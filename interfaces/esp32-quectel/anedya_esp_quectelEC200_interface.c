#include "sdkconfig.h"

#ifdef CONFIG_AN_INTERFACE_ESP32_QUETEL
#include "anedya_interface.h"

// Network Interface is Sim Network Quectel

#include "anedya_esp_quectelEC200_interface.h"
#include "anedya_certs.h"
#include "anedya_client.h"
#include "anedya_commons.h"
#include "time.h"
#include <sys/time.h>
#include <math.h>

static const char *TAG = "Anedya";
static short debug_level = 1;

static esp_mqtt_client_config_t mqtt_cfg;

static short UART_PORT_NUMBER = -1;
static SemaphoreHandle_t uart_port_mutex;

#define UNKONWN_CMD -1
#define MODEM_CMD_EX_ASAP 0
#define MODEM_RESP_WAIT 1
static unsigned int CMD_TYPE = MODEM_CMD_EX_ASAP;

#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
static uint8_t response_topic[150];
static uint8_t modem_response[ANEDYA_RX_BUFFER_SIZE];
static uint8_t dtmp[200] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

static unsigned int modem_response_len = 0;
static bool more_data_available = false;

anedya_ext_mqtt_event_t mqtt_event;
#define MQTT_EVENT_RECEIVE_DATA (BIT0)

static EventGroupHandle_t ModemEvents;
static EventGroupHandle_t MqttEvents;

#define MODEM_EVENT_RECEIVE_DATA (BIT0)
#define MODEM_EVENT_OTA_NOT_IN_PROGRESS (BIT1)

#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
static uint8_t pat[PATTERN_CHR_NUM + 1];

static void _uart_event_task(void *pvParameters);
static anedya_err_t _anedya_ext_clear_uart_buffer(anedya_client_t *anedya_client);
static void _anedya_ext_mqtt_event_task(void *pvParameters);
static void _mqtt_message_parser(anedya_client_t *anedya_client, uart_event_t event);
static anedya_err_t _anedya_ext_send_AT_command(char *cmd, unsigned int cmd_type, char *resp, char *expected_resp, size_t timeout);

void _anedya_interface_sleep_ms(size_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

uint64_t _anedya_interface_get_time_ms()
{
    struct timeval ts;
    gettimeofday(&ts, NULL);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_usec / 1000;
}

void _anedya_interface_std_out(const char *str)
{
    ESP_LOGI("ANEDYA", "%s", str);
}

static void _anedya_ext_mqtt_event_task(void *pvParameters)
{
    anedya_client_t *anedya_client = (anedya_client_t *)pvParameters;

    xEventGroupClearBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
    while (1)
    {
        xEventGroupWaitBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupClearBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);

        switch (mqtt_event.event_id)
        {
        case EXT_MQTT_EVENT_CONNECTED:
            if (anedya_client->_anedya_on_connect_handler != NULL)
            {
                anedya_client->_anedya_on_connect_handler(anedya_client);
            }
            continue;
        case EXT_MQTT_EVENT_DISCONNECTED:
            if (anedya_client->_anedya_on_disconnect_handler != NULL)
            {
                anedya_client->_anedya_on_disconnect_handler(anedya_client);
            }
            continue;
        case EXT_MQTT_EVENT_DATA:
            if (anedya_client->_message_handler != NULL)
            {
                anedya_client->_message_handler(anedya_client, mqtt_event.topic, mqtt_event.topic_len, mqtt_event.data, mqtt_event.data_len);
            }
            continue;
        default:
            continue;
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

static void _mqtt_message_parser(anedya_client_t *anedya_client, uart_event_t event)
{
    int dummy1, dummy2;
    // check for mqtt connect URC
    char *conn_ptr = strstr((char *)dtmp, "+QMTCONN:");
    if (conn_ptr != NULL)
    {
        int ret_code = 5;
        int matched = sscanf((char *)conn_ptr, "+QMTCONN: %*d,%*d,%d", &ret_code);
        if (matched)
        {
            if (ret_code == 0)
            {
                mqtt_event.event_id = EXT_MQTT_EVENT_CONNECTED;
                xEventGroupSetBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
            }
            else
            {
                ESP_LOGE(TAG, "Invalid uart response!");
            }
        }
    }
    // check for mqtt disconnect URC
    char *stat_ptr = strstr((char *)dtmp, "+QMTSTAT:");
    if (stat_ptr != NULL)
    {
        ESP_LOGE(TAG, "Mqtt Disconnected!");
        mqtt_event.event_id = EXT_MQTT_EVENT_DISCONNECTED;
        xEventGroupSetBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
    }

    // check for the mqtt message URC
    char *recv_ptr = strstr((char *)dtmp, "+QMTRECV:");
    if (recv_ptr != NULL)
    {
        modem_response_len = 0;
        int matched = sscanf(recv_ptr, "+QMTRECV: %d,%d,\"%[^\"]\",%d", &dummy1, &dummy2, response_topic, &modem_response_len);
        if (matched)
        {
            if (modem_response_len > 9)
            {
                char *start_ptr = strchr((char *)dtmp, '{');
                if (start_ptr)
                {
                    strncpy((char *)modem_response, (char *)start_ptr, modem_response_len);
                }
            }
            more_data_available = true;
        }
        else
        {
            ESP_LOGE("UART EVENT HANDLER", "Invalid uart response");
        }
    }
    else if (more_data_available)
    {
        if (modem_response_len < 10)
        {
            int a = modem_response_len;
            int b;

            int matched = sscanf((char *)dtmp, "%d,\"{", &b);
            if (matched != 1)
            {
                ESP_LOGE(TAG, "Failed to extract b from response");
                more_data_available = false;
                return;
            }
            // Find number of digits in b
            int temp = b, digits = 0;
            while (temp != 0)
            {
                temp /= 10;
                digits++;
            }

            // Concatenate
            modem_response_len = a * pow(10, digits) + b;
            char *start_ptr = strchr((char *)dtmp, '{');
            if (start_ptr)
            {
                strncpy((char *)modem_response, (char *)start_ptr, modem_response_len);
            }
            if (strlen((char *)modem_response) == modem_response_len)
            {
                if (debug_level > 1)
                    ESP_LOGW("Anedya Response", "%s", (char *)modem_response);

                if (strlen((char *)response_topic) == 0)
                {
                    ESP_LOGE(TAG, "No topic found in response!");
                    more_data_available = false;
                    return;
                }

                if (modem_response[modem_response_len - 1] == '}')
                {
                    mqtt_event.event_id = EXT_MQTT_EVENT_DATA;
                    mqtt_event.data = (char *)modem_response;
                    mqtt_event.topic = (char *)response_topic;
                    mqtt_event.data_len = strlen((char *)modem_response);
                    mqtt_event.topic_len = strlen((char *)response_topic);
                    xEventGroupSetBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
                    xEventGroupSetBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
                }
                more_data_available = false;
                return;
            }
        }
        else if (strlen((char *)modem_response) < modem_response_len)
        {
            int i = 0;
            while (event.size != i)
            {
                strncat((char *)modem_response, (char *)dtmp + i, 1);
                if (strlen((char *)modem_response) == modem_response_len)
                {
                    if (debug_level > 1)
                        ESP_LOGW("Anedya Response", "%s", (char *)modem_response);

                    if (strlen((char *)response_topic) == 0)
                    {
                        ESP_LOGE(TAG, "No topic found in response!");
                        more_data_available = false;
                        break;
                    }

                    if (modem_response[modem_response_len - 1] == '}')
                    {
                        mqtt_event.event_id = EXT_MQTT_EVENT_DATA;
                        mqtt_event.data = (char *)modem_response;
                        mqtt_event.topic = (char *)response_topic;
                        mqtt_event.data_len = strlen((char *)modem_response);
                        mqtt_event.topic_len = strlen((char *)response_topic);
                        xEventGroupSetBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
                        xEventGroupSetBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
                    }
                    more_data_available = false;
                    break;
                }
                i++;
            }
        }
        else
        {
            if (debug_level > 1)
                ESP_LOGW("Anedya Response", "%s", (char *)modem_response);

            if (strlen((char *)response_topic) == 0)
            {
                ESP_LOGE(TAG, "No topic found in response!");
                more_data_available = false;
                return;
            }

            if (modem_response[modem_response_len - 1] == '}')
            {
                mqtt_event.event_id = EXT_MQTT_EVENT_DATA;
                mqtt_event.data = (char *)modem_response;
                mqtt_event.topic = (char *)response_topic;
                mqtt_event.data_len = strlen((char *)modem_response);
                mqtt_event.topic_len = strlen((char *)response_topic);
                xEventGroupSetBits(MqttEvents, MQTT_EVENT_RECEIVE_DATA);
                xEventGroupSetBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
            }
            more_data_available = false;
        }
    }
    else
    {
        if (debug_level > 1)
            ESP_LOGI(TAG, "Modem response: %s", (char *)dtmp);
        xEventGroupSetBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
    }
    return;
}

static void _uart_event_task(void *pvParameters)
{
    anedya_client_t *anedya_client = (anedya_client_t *)pvParameters;
    QueueHandle_t uart_queue = ((anedya_ext_config_t *)anedya_client->config->interface_config)->uart_queue_handle;

    uart_event_t event;
    size_t buffered_size;
    _anedya_ext_clear_uart_buffer(anedya_client); // Clear the UART buffer

    for (;;)
    {
        xEventGroupWaitBits(ModemEvents, MODEM_EVENT_OTA_NOT_IN_PROGRESS, pdFALSE, pdFALSE, portMAX_DELAY);

        // Waiting for UART event
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                // ESP_LOGI("UART EVENT HANDLER", "[UART DATA]: %d", event.size);
                for (int i = 0; i < 200; ++i)
                    dtmp[i] = 0;

                if (!more_data_available)
                {
                    // empty the buffers
                    for (int i = 0; i < ANEDYA_RX_BUFFER_SIZE; ++i)
                        modem_response[i] = 0;
                    for (int j = 0; j < 150; ++j)
                        response_topic[j] = 0;
                }
                // Remove unwanted characters
                int len = uart_read_bytes(UART_PORT_NUMBER, dtmp, event.size, pdMS_TO_TICKS(2000));
                if (len <= 0)
                    break;
                int j = 0;
                for (int i = 0; i < len; ++i)
                {
                    if (dtmp[i] != '\r' && dtmp[i] != '\n')
                        dtmp[j++] = dtmp[i];
                }
                dtmp[j] = '\0';

                // ESP_LOGI("UartEventHandler", "%s", dtmp);
                _mqtt_message_parser(anedya_client, event);
                break;

            case UART_FIFO_OVF:
                ESP_LOGI("UART EVENT HANDLER", "hw fifo overflow");
                uart_flush_input(UART_PORT_NUMBER);
                xQueueReset(uart_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGI("UART EVENT HANDLER", "ring buffer full");
                uart_flush_input(UART_PORT_NUMBER);
                xQueueReset(uart_queue);
                break;

            case UART_BREAK:
                ESP_LOGI("UART EVENT HANDLER", "uart rx break");
                break;

            case UART_PARITY_ERR:
                ESP_LOGI("UART EVENT HANDLER", "uart parity error");
                break;

            case UART_FRAME_ERR:
                ESP_LOGI("UART EVENT HANDLER", "uart frame error");
                break;

            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART_PORT_NUMBER, &buffered_size);
                int pos = uart_pattern_pop_pos(UART_PORT_NUMBER);
                ESP_LOGI("UART EVENT HANDLER", "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1)
                {
                    uart_flush_input(UART_PORT_NUMBER);
                }
                else
                {
                    uart_read_bytes(UART_PORT_NUMBER, dtmp, pos, 100 / portTICK_PERIOD_MS);

                    // Clear pat manually
                    for (int i = 0; i < (PATTERN_CHR_NUM + 1); ++i)
                        pat[i] = 0;

                    uart_read_bytes(UART_PORT_NUMBER, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);

                    ESP_LOGI("UART EVENT HANDLER", "read data: %s", dtmp);
                    ESP_LOGI("UART EVENT HANDLER", "read pat : %s", pat);
                }
                break;

            default:
                ESP_LOGI("UART EVENT HANDLER", "uart event type: %d", event.type);
                break;
            }
        }

        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

static anedya_err_t _anedya_ext_send_AT_command(char *cmd, unsigned int cmd_type, char *resp, char *expected_resp, size_t timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    CMD_TYPE = cmd_type;
    if (debug_level > 1)
        ESP_LOGI(TAG, "TX Commnad: %s", cmd);
    uart_write_bytes(UART_PORT_NUMBER, cmd, strlen(cmd));

    if (cmd_type == MODEM_RESP_WAIT)
    {
        int start_time = xTaskGetTickCount();
        if (expected_resp != NULL)
        {
            while (xTaskGetTickCount() - start_time < timeout / portTICK_PERIOD_MS)
            {
                if (!xEventGroupWaitBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA, pdFALSE, pdFALSE, timeout / portTICK_PERIOD_MS))
                {
                    ESP_LOGW(TAG, "Timed out waiting for response from modem");
                    return ANEDYA_ERR_EXT_TIMEOUT;
                }
                char *check_ptr = strstr((char *)dtmp, expected_resp);
                // if (strncmp((char *)dtmp, expected_resp, strlen((char *)expected_resp)) == 0)
                if (check_ptr != NULL)
                {
                    // ESP_LOGI("RX Commnad:", "%s", dtmp);
                    if (resp != NULL)
                    {
                        strcpy((char *)resp, (char *)dtmp);
                    }
                    CMD_TYPE = UNKONWN_CMD;
                    xEventGroupClearBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
                    return ANEDYA_OK;
                }
                xEventGroupClearBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
            }
            return ANEDYA_ERR_EXT_TIMEOUT;
        }
        else
        {
            // ESP_LOGI(TAG, "Waiting for response from modem with timeout: %d ms", timeout);
            vTaskDelay(timeout / portTICK_PERIOD_MS);
            if (strlen((char *)dtmp) == 0)
                return ANEDYA_ERR_EXT_TIMEOUT;
        }
    }
    else
    {
        if (!xEventGroupWaitBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA, pdFALSE, pdFALSE, timeout / portTICK_PERIOD_MS))
        {
            ESP_LOGW(TAG, "Timed out waiting for response from modem");
            return ANEDYA_ERR_EXT_TIMEOUT;
        }
    }
    // ESP_LOGI("RX Commnad:", "%s", dtmp);
    if (resp != NULL)
    {
        strcpy((char *)resp, (char *)dtmp);
    }
    for (int i = 0; i < strlen((char *)dtmp); i++)
        dtmp[i] = 0;
    CMD_TYPE = UNKONWN_CMD;
    xEventGroupClearBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);
    return ANEDYA_OK;
}

anedya_err_t _anedya_interface_init(anedya_client_t *client)
{
    anedya_ext_config_t *ext_config = (anedya_ext_config_t *)client->config->interface_config;
    UART_PORT_NUMBER = ext_config->uart_port_num;

    if (ext_config->uart_port_num == -1)
    {
        ESP_LOGE(TAG, "Invalid port number passed to uart init");
        return ANEDYA_EXT_ERR;
    }
    if(debug_level > 0)
    ESP_LOGI(TAG, "Initializing the Anedya Quectel EC200 interface");
    ModemEvents = xEventGroupCreate();
    MqttEvents = xEventGroupCreate();
    uart_port_mutex = xSemaphoreCreateMutex();

    // Install UART driver, and get the queue.
    uart_driver_install(ext_config->uart_port_num, ANEDYA_RX_BUFFER_SIZE, ANEDYA_TX_BUFFER_SIZE, 20, &ext_config->uart_queue_handle, 0);
    uart_param_config(ext_config->uart_port_num, &ext_config->uart_config);

    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(ext_config->uart_port_num, ext_config->tx_pin, ext_config->rx_pin, ext_config->rts_pin, ext_config->cts_pin); // Mcu RTS PIN, Mcu CTS PIN

    // Enable pattern detection
    if (uart_enable_pattern_det_baud_intr(ext_config->uart_port_num, '+', PATTERN_CHR_NUM, 9, 0, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not enable pattern detection");
        return ANEDYA_EXT_ERR;
    }

    if (uart_pattern_queue_reset(ext_config->uart_port_num, 20) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not reset pattern queue");
        return ANEDYA_EXT_ERR;
    }

    // Create the task
    if (xTaskCreate(_uart_event_task, "uart_event_task", 8192, client, 1, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Could not create uart event task");
        return ANEDYA_EXT_ERR;
    }

    xEventGroupSetBits(ModemEvents, MODEM_EVENT_OTA_NOT_IN_PROGRESS);
    anedya_err_t err;

    err = _anedya_ext_send_AT_command("ATE0\r\n", MODEM_CMD_EX_ASAP, NULL, "OK", 2000); // disable echo

    if (xTaskCreate(_anedya_ext_mqtt_event_task, "MQTT_EVENT_TASK", 8192, client, 2, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create MQTT Event task");
    }
    err = _anedya_ext_send_AT_command("AT+QMTDISC=0\r\n", MODEM_CMD_EX_ASAP, NULL, NULL, 2000); // disconnect from MQTT broker
    err = _anedya_ext_send_AT_command("AT\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    if (err == ANEDYA_OK)
    {
        if (ext_config->apn_count > 0)
        {
            int check = 0;
            while (1)
            {
                if (debug_level > 0)
                    ESP_LOGI(TAG, "Checking Modem Internet Connectivity...");
                int status = -1, rssi = 0, mode = -1;
                err = anedya_ext_network_reg_status(client, &status, 2000);
                err = anedya_ext_network_operator(client, &mode, 2000);
                err = anedya_ext_signal_quality(client, &rssi, NULL, 2000);
                if (debug_level > 0)
                    ESP_LOGI(TAG, "Status: %d, Mode: %d, RSSI: %d", status, mode, rssi);
                char ping_url[100] = {0};
                snprintf(ping_url, 100, "https://device.%s.anedya.io/v1/check", client->config->region);
                err = anedya_ext_net_check(client, ping_url, strlen(ping_url), 30000);
                if (err == ANEDYA_OK)
                {
                    if (debug_level > 0)
                    {
                        ESP_LOGI(TAG, "Modem is connected to internet.");
                    }
                    return ANEDYA_OK;
                }
                else
                {
                    if (debug_level > 0)
                        ESP_LOGI(TAG, "Setting APN...");

                    err = anedya_ext_deactivate_pdp_context(client, 1, 2000);
                    anedya_ext_apn_config_t *apn_config = (anedya_ext_apn_config_t *)ext_config->apn_configs;
                    for (int i = 0; i < ext_config->apn_count; i++)
                    {
                        if (apn_config[i].apn == NULL)
                        {
                            ESP_LOGE(TAG, "APN is NULL, plz check the config");
                            return ANEDYA_EXT_ERR;
                        }
                        anedya_ext_set_apn(client, apn_config[i].cid, apn_config[i].ip_ver, apn_config[i].apn, apn_config[i].username, apn_config[i].password);
                        vTaskDelay(50 / portTICK_PERIOD_MS);
                    }
                    err = anedya_ext_set_fun_mode(client, 0, NULL, 2000);
                    err = anedya_ext_set_fun_mode(client, 1, NULL, 10000);
                }
                check++;
                if (check > 4)
                    return ANEDYA_EXT_ERR;
                vTaskDelay(10000 / portTICK_PERIOD_MS);
            }
        }
    }
    return err;
}

anedya_err_t anedya_ext_restore_settings_to_factory_defaults(anedya_client_t *client, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    // if (value < 0)
    //     return ANEDYA_EXT_ERR;
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[20];

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    sprintf(AT_cmd, "AT&F\r\n");
    anedya_err_t err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "OK", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}
anedya_err_t anedya_ext_set_fun_mode(anedya_client_t *client, int fun, int rst, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (fun < 0 || rst < 0)
    {
        ESP_LOGE(TAG, "Invalid fun or rst value!");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[30];

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif
    if (fun >= 0 && rst >= 0)
    {
        sprintf(AT_cmd, "AT+CFUN=%d,%d\r\n", (short)fun, (short)rst);
    }
    else if (rst < 0)
    {
        sprintf(AT_cmd, "AT+CFUN=%d\r\n", fun);
    }
    anedya_err_t err = ANEDYA_EXT_ERR;
    if (rst == 1)
    {
        err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "RDY", timeout);
        _anedya_ext_send_AT_command("ATE0\r\n", MODEM_CMD_EX_ASAP, NULL, NULL, 2000);
    }
    else
    {
        err = _anedya_ext_send_AT_command(AT_cmd, MODEM_CMD_EX_ASAP, NULL, NULL, timeout);
    }
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_ext_connectivity_check(anedya_client_t *client, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
    anedya_err_t err = _anedya_ext_send_AT_command("AT\r\n", MODEM_RESP_WAIT, NULL, "OK", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_ext_network_reg_status(anedya_client_t *client, int *stat, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (stat == NULL)
    {
        ESP_LOGE(TAG, "stat is null!");
        return ANEDYA_EXT_ERR;
    }
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char s_response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
    err = _anedya_ext_send_AT_command("AT+CEREG?\r\n", MODEM_RESP_WAIT, s_response, "+CEREG:", timeout);
    xSemaphoreGive(uart_port_mutex);

    int check = sscanf(s_response, "+CEREG: %*d,%d", stat);
    if (check != 1)
    {
        return ANEDYA_EXT_ERR;
    }

    return err;
}
anedya_err_t anedya_ext_network_operator(anedya_client_t *client, int *mode, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (mode == NULL)
    {
        ESP_LOGE(TAG, "mode is null!");
        return ANEDYA_EXT_ERR;
    }
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char s_response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
    err = _anedya_ext_send_AT_command("AT+COPS?\r\n", MODEM_RESP_WAIT, s_response, "+COPS:", timeout);
    xSemaphoreGive(uart_port_mutex);

    int check = sscanf(s_response, "+COPS: %d", mode);
    if (check != 1)
    {
        return ANEDYA_EXT_ERR;
    }

    return err;
}
anedya_err_t anedya_ext_signal_quality(anedya_client_t *client, int *rssi, int *ber, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char s_response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    if (rssi == NULL)
    {
        ESP_LOGE(TAG, "rssi is null");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
    err = _anedya_ext_send_AT_command("AT+CSQ\r\n", MODEM_RESP_WAIT, s_response, "+CSQ:", timeout);
    xSemaphoreGive(uart_port_mutex);
    if (ber != NULL)
    {
        int check = sscanf(s_response, "+CSQ: %d,%d", rssi, ber);
        if (check != 2)
        {
            return ANEDYA_EXT_ERR;
        }
    }
    else
    {
        int check = sscanf(s_response, "+CSQ: %d,", rssi);
        if (check != 1)
        {
            return ANEDYA_EXT_ERR;
        }
    }
    return err;
}

anedya_err_t anedya_ext_pdp_context_status(anedya_client_t *client, char *pdp_context_out, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
    err = _anedya_ext_send_AT_command("AT+CGDCONT?\r\n", MODEM_RESP_WAIT, pdp_context_out, "+CGDCONT:", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}
anedya_err_t anedya_ext_activate_pdp_context(anedya_client_t *client, int cid, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (cid < 0)
    {
        ESP_LOGE(TAG, "cid is invalid!");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[30] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    sprintf(AT_cmd, "AT+QIACT=%d\r\n", cid);
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "OK", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}
anedya_err_t anedya_ext_deactivate_pdp_context(anedya_client_t *client, int cid, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (cid == NULL || cid < 0)
    {
        ESP_LOGE(TAG, "cid is invalid!");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[30] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    sprintf(AT_cmd, "AT+QIDEACT=%d\r\n", cid);
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "OK", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_ext_read_pdp_context(anedya_client_t *client, char *pdp_context_out, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, (timeout + 5000) / portTICK_PERIOD_MS);
    anedya_err_t err = ANEDYA_EXT_ERR;
    err = _anedya_ext_send_AT_command("AT+QIACT?\r\n", MODEM_RESP_WAIT, pdp_context_out, "+QIACT:", timeout);
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_ext_net_check(anedya_client_t *client, char *url, int url_len, int timeout)
{
    if (url_len > 100)
    {
        ESP_LOGE(TAG, "URL too long, exceeds 100 characters");
        return ANEDYA_EXT_ERR;
    }
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }

    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);

    // Configure HTTP and SSL
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"contextid\",1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"responseheader\",1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"requestheader\",0\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"sslctxid\",3\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"sslversion\",3,3\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"seclevel\",3,0\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"sni\",3,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"cacert\",3,\"UFS:anedya_tls_root_ca.pem\"\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[100] = {0};
    char cmd_response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QHTTPURL=%d,80\r\n", url_len);
    if (_anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "CONNECT", 80000) != ANEDYA_OK)
    {
        xSemaphoreGive(uart_port_mutex);
        return ANEDYA_EXT_ERR;
    }

    uart_write_bytes(UART_PORT_NUMBER, (const char *)url, url_len);
    vTaskDelay(pdMS_TO_TICKS(1000));

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QHTTPGET=%d\r\n", timeout / 1000);
    if (_anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, cmd_response, "+QHTTPGET:", timeout + 1000) != ANEDYA_OK)
    {
        xSemaphoreGive(uart_port_mutex);
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreGive(uart_port_mutex);
    if (strlen(cmd_response) > 0)
    {
        if (debug_level > 2)
            ESP_LOGI(TAG, "Ping response: %s", cmd_response);
        int err = 0;
        short check = sscanf(cmd_response, "+QHTTPGET: %d", &err);
        if (check == 1)
        {
            if (err == 0)
            {
                if (debug_level > 2)
                    ESP_LOGI(TAG, "Ping success: %d", err);
                return ANEDYA_OK;
            }
            else if (err == 702)
            {
                ESP_LOGI(TAG, "Ping failed: %d", err);
                return ANEDYA_EXT_ERR;
            }
        }
        else
        {
            ESP_LOGW(TAG, "Invalid Ping Response: %s", cmd_response);
        }
    }
    else
    {
        ESP_LOGE(TAG, "Ping failed: %s", cmd_response);
        return ANEDYA_EXT_ERR;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_ext_set_apn(anedya_client_t *client, int cid, char *ip_ver, char *apn, char *user, char *pass)
{
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char at_cmd[100] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    sprintf(at_cmd, "AT+CGDCONT=%d,\"%s\",\"%s\"\r\n", cid, ip_ver, apn);
    anedya_err_t err = _anedya_ext_send_AT_command(at_cmd, MODEM_RESP_WAIT, NULL, "OK", 2000);
    xSemaphoreGive(uart_port_mutex);
    return err;
}

static anedya_err_t _anedya_ext_clear_uart_buffer(anedya_client_t *anedya_client)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    size_t buffered_len = 0;
    QueueHandle_t uart_queue = ((anedya_ext_config_t *)anedya_client->config->interface_config)->uart_queue_handle;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_PORT_NUMBER, &buffered_len));
    for (int i = 0; i < buffered_len; i++)
    {
        uint8_t data;
        uart_read_bytes(UART_PORT_NUMBER, &data, 1, pdMS_TO_TICKS(500));
    }
    xQueueReset(uart_queue);
    return ANEDYA_OK;
}

anedya_err_t anedya_ext_get_modem_time(anedya_client_t *client, int mode, char *output_dateTime)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char temp[50];
    char AT_cmd[20];

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    sprintf(AT_cmd, "AT+QLTS=%d\r\n", mode);
    anedya_err_t err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, temp, "+QLTS:", 10000);
    if (err == ANEDYA_OK)
    {
        sscanf(temp, "+QLTS: \"%[^\"]\"", output_dateTime);
        // ESP_LOGI(TAG, "Modem Time: %s", output_dateTime);
    }
    xSemaphoreGive(uart_port_mutex);
    return err;
}

#ifdef ANEDYA_CONNECTION_METHOD_MQTT

anedya_mqtt_client_handle_t _anedya_interface_mqtt_init(anedya_client_t *parent, char *broker, const char *devid, const char *secret)
{
    mqtt_cfg = (esp_mqtt_client_config_t){
        .network.disable_auto_reconnect = false,
        .outbox.limit = 1024,
        .broker = {
            .address.port = 8883,
            .address.hostname = broker,
            .address.transport = MQTT_TRANSPORT_OVER_SSL,
            // .verification.certificate = (const char *)anedya_tls_root_ca,
            // .verification.certificate_len = anedya_tls_root_ca_len,
        },
        .credentials = {
            .username = devid,
            .authentication.password = secret,
            .client_id = devid,
        },
    };
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[300];
    char response[20] = {0};
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    anedya_err_t err = ANEDYA_EXT_ERR;
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
    err = _anedya_ext_send_AT_command("AT+IFC=2,2\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000); // Enable RTS/CTS flow control
    err = _anedya_ext_send_AT_command("AT+IFC?\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);

    // uploade tls root ca
    err = _anedya_ext_send_AT_command("AT+QFOPEN=\"anedya_tls_root_ca.pem\",1\r\n", MODEM_RESP_WAIT, response, "+QFOPEN:", 2000);
    if (err != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to open `anedya_tls_root_ca.pem` file");
        xSemaphoreGive(uart_port_mutex);
        return NULL;
    }
    int f_handle;
    int matched = sscanf((char *)response, "+QFOPEN: %d", &f_handle);
    if (matched != 1)
    {
        ESP_LOGE(TAG, "Failed to get file handle");
        xSemaphoreGive(uart_port_mutex);
        return NULL;
    }
    // ESP_LOGI(TAG, "File handle: %d", f_handle);

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QFWRITE=%d,%d,20\r\n", (int)f_handle, anedya_tls_root_ca_len);
    // Send QFWRITE command to modem
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, response, "CONNECT", 10000);
    if (err != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to initiate QFWRITE");
        xSemaphoreGive(uart_port_mutex);
        return (void *)NULL;
    }
    for (int i = 0; i < anedya_tls_root_ca_len; i++)
    {
        char data[2];
        data[0] = anedya_tls_root_ca[i];
        data[1] = '\0';
        int bytes_written = uart_write_bytes(UART_PORT_NUMBER, data, strlen(data));
        if (bytes_written != strlen(data))
        {
            ESP_LOGE(TAG, "Failed to write %d bytes to UART. Only %d bytes were written.", strlen(data), bytes_written);
            xSemaphoreGive(uart_port_mutex);
            return NULL;
        }
    }
    if (!xEventGroupWaitBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA, pdFALSE, pdFALSE, 10000 / portTICK_PERIOD_MS))
    {
        ESP_LOGE(TAG, "Failed to upload anedya tls root ca!");
        xSemaphoreGive(uart_port_mutex);
        return NULL;
    }
    if (strncmp((char *)dtmp, "+QFWRITE: 769,769", strlen("+QFWRITE: 769,769")) == 0)
    {
        if (debug_level > 2)
            ESP_LOGI(TAG, "set anedya tls root ca success!");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to upload anedya tls root ca!");
        xSemaphoreGive(uart_port_mutex);
        return NULL;
    }
    xEventGroupClearBits(ModemEvents, MODEM_EVENT_RECEIVE_DATA);

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QFCLOSE=%d\r\n", f_handle);
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "OK", 5000);

    // Config broker credentials
    // NOTE: ssl context id is 2 for the mqtt client, will set it 3 for the http client
    err = _anedya_ext_send_AT_command("AT+QMTCFG=\"recv/mode\",0,0,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QMTCFG=\"SSL\",0,1,2\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"cacert\",2,\"UFS:anedya_tls_root_ca.pem\"\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"seclevel\",2,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"sni\",2,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"sslversion\",2,4\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"ciphersuite\",2,0xFFFF\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    err = _anedya_ext_send_AT_command("AT+QSSLCFG=\"ignorelocaltime\",2,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 2000);
    xSemaphoreGive(uart_port_mutex);
    return (void *)&mqtt_cfg;
}

anedya_err_t anedya_interface_mqtt_connect(anedya_mqtt_client_handle_t anclient)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    // xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[300];
    char response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    anedya_err_t err = ANEDYA_EXT_ERR;
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
    sprintf(AT_cmd, "AT+QMTOPEN=0,\"%s\",8883\r\n", mqtt_cfg.broker.address.hostname);
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, response, "+QMTOPEN:", 40000);
    if (err != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to open MQTT connection");
        xSemaphoreGive(uart_port_mutex);
        return ANEDYA_EXT_ERR;
    }
    if (debug_level > 3)
        ESP_LOGI(TAG, "OPEN RESPONSE: %s", (char *)response);
    int result_code = -1;
    int matched = sscanf((char *)response, "+QMTOPEN: %*d,%d", &result_code);
    if (matched)
    {
        if (result_code == 0)
        {
            if (debug_level > 0)
                ESP_LOGI(TAG, "MQTT connection opened!");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to open MQTT connection, response: %s", (char *)response);
            xSemaphoreGive(uart_port_mutex);
            return ANEDYA_EXT_ERR;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to open MQTT connection, modem err");
    }

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", mqtt_cfg.credentials.client_id, mqtt_cfg.credentials.username, mqtt_cfg.credentials.authentication.password);
    err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, response, "+QMTCONN:", 30000);
    if (err == ANEDYA_OK)
    {
        int ret_code = 5;
        int matched = sscanf((char *)response, "+QMTCONN: %*d,%*d,%d", &ret_code);
        if (matched)
        {
            if (ret_code == 0)
            {
                if (debug_level > 0)
                    ESP_LOGI(TAG, "MQTT connection established!");
                xSemaphoreGive(uart_port_mutex);
                return ANEDYA_OK;
            }
            else
            {
                switch (ret_code)
                {
                case 1:
                    ESP_LOGE(TAG, " Connection Refused: Unacceptable Protocol Version ");
                    break;
                case 2:
                    ESP_LOGE(TAG, "Connection Refused: Identifier Rejected ");
                    break;
                case 3:
                    ESP_LOGE(TAG, "Connection Refused: Server Unavailable ");
                    break;
                case 4:
                    ESP_LOGE(TAG, "Connection Refused: Bad User Name or Password ");
                    break;
                case 5:
                    ESP_LOGE(TAG, "Connection Refused: Not Authorized ");
                    break;
                default:
                    ESP_LOGE(TAG, "Connection Refused! ");
                    ESP_LOGE(TAG, "Modem Response: %s", (char *)response);
                    break;
                }
                xSemaphoreGive(uart_port_mutex);
                return ANEDYA_EXT_ERR;
            }
        }
        else
        {
            ESP_LOGE(TAG, "Invalid uart response!");
        }
    }
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_interface_mqtt_disconnect(anedya_mqtt_client_handle_t anclient)
{

    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_destroy(anedya_mqtt_client_handle_t anclient)
{

    return ANEDYA_OK;
}

size_t anedya_interface_mqtt_status(anedya_mqtt_client_handle_t anclient)
{

    return 0;
}

anedya_err_t anedya_interface_mqtt_subscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topilc_len, int qos)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[300];
    char response[20] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif
    if (debug_level > 2)
        ESP_LOGI(TAG, "Subscribing to topic: %s", (char *)topic);
    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QMTSUB=0,1,\"%s\",%d\r\n", (char *)topic, qos);
    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);
    anedya_err_t err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, response, "+QMTSUB:", 5000);
    if (err == ANEDYA_OK)
    {
        if (debug_level > 3)
            ESP_LOGI(TAG, "MQTT subscribed! %s", (char *)response);
        int result = 2;
        int matched = sscanf((char *)response, "+QMTSUB: %*d,%*d,%d", &result);
        if (matched)
        {
            if (result == 0)
            {
                xSemaphoreGive(uart_port_mutex);
                return ANEDYA_OK;
            }
            else
            {
                ESP_LOGE(TAG, "MQTT subscribe failed!");
                switch (result)
                {
                case 1:
                    ESP_LOGE(TAG, "Packet retransmission ");
                    break;
                case 2:
                    ESP_LOGE(TAG, "Failed to send packet");
                    break;
                default:
                    ESP_LOGE(TAG, "Unknown error | res: %s", (char *)response);
                    break;
                }
                ESP_LOGE(TAG, "MQTT subscribe failed! %s", (char *)response);
                xSemaphoreGive(uart_port_mutex);
                return ANEDYA_EXT_ERR;
            }
        }
        else
        {
            ESP_LOGE(TAG, "MQTT subscribe failed! %s", (char *)response);
            xSemaphoreGive(uart_port_mutex);
            return ANEDYA_EXT_ERR;
        }
    }
    xSemaphoreGive(uart_port_mutex);
    return err;
}

anedya_err_t anedya_interface_mqtt_unsubscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len)
{

    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_publish(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len, char *payload, int payload_len, int qos, int retain)
{

    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    if (xSemaphoreTake(uart_port_mutex, 60000 / portTICK_PERIOD_MS) == pdTRUE)
    {
        char AT_cmd[topic_len + 40];
        snprintf(AT_cmd, sizeof(AT_cmd), "AT+QMTPUBEX=0,1,%d,%d,\"%s\",%d\r\n", qos, retain, (char *)topic, payload_len);
        anedya_err_t err = _anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, ">", 10000);
        if (err == ANEDYA_OK)
        {

            err = _anedya_ext_send_AT_command((char *)payload, MODEM_RESP_WAIT, NULL, "}", 5000);
        }

        xSemaphoreGive(uart_port_mutex);
        return err;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take semaphore in publish.");
        return ANEDYA_EXT_ERR;
    }
}

anedya_err_t anedya_set_message_callback(anedya_mqtt_client_handle_t anclient, anedya_client_t *client)
{
    return ANEDYA_OK;
}
#endif // end of ANEDYA_CONNECTION_METHOD_MQTT

anedya_err_t anedya_ext_http_get_range_request(anedya_client_t *client, anedya_ext_net_reader_t *reader, char *url, int url_len, int starting_position, int readlen, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }

    xSemaphoreTake(uart_port_mutex, portMAX_DELAY);

    // Configure HTTP and SSL
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"contextid\",1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"responseheader\",1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"requestheader\",0\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QHTTPCFG=\"sslctxid\",3\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"sslversion\",3,3\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"seclevel\",3,0\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"sni\",3,1\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
    _anedya_ext_send_AT_command("AT+QSSLCFG=\"cacert\",3,\"UFS:anedya_tls_root_ca.pem\"\r\n", MODEM_RESP_WAIT, NULL, "OK", 5000);
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char AT_cmd[100] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QHTTPURL=%d,80\r\n", url_len);
    if (_anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "CONNECT", 80000) != ANEDYA_OK)
        return ANEDYA_EXT_ERR;

    uart_write_bytes(UART_PORT_NUMBER, (const char *)url, url_len);
    vTaskDelay(pdMS_TO_TICKS(1000));

    snprintf(AT_cmd, sizeof(AT_cmd), "AT+QHTTPGETEX=80,%d,%d\r\n", starting_position, readlen);
    if (_anedya_ext_send_AT_command(AT_cmd, MODEM_RESP_WAIT, NULL, "+QHTTPGET:", 80000) != ANEDYA_OK)
        return ANEDYA_EXT_ERR;

    xEventGroupClearBits(ModemEvents, MODEM_EVENT_OTA_NOT_IN_PROGRESS);
    _anedya_ext_clear_uart_buffer(client); // Clear the UART buffer

    ESP_LOGI("TX Command", "AT+QHTTPREAD=300\r\n");
    uart_write_bytes(UART_PORT_NUMBER, "AT+QHTTPREAD=300\r\n", strlen("AT+QHTTPREAD=300\r\n"));

// --- Header Parsing Section ---
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char header_buf[2048] = {0};

#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif

    int header_pos = 0;
    bool headers_done = false;
    bool content_length_found = false;
    int content_length = -1;
    uint8_t byte;

    TickType_t start = xTaskGetTickCount();
    while (!headers_done && (xTaskGetTickCount() - start < pdMS_TO_TICKS(timeout)))
    {
        int read = uart_read_bytes(UART_PORT_NUMBER, &byte, 1, pdMS_TO_TICKS(500));
        if (read == 1)
        {
            if (header_pos < sizeof(header_buf) - 1)
            {
                header_buf[header_pos++] = byte;
                header_buf[header_pos] = '\0';
            }

            // Detect end of headers
            if (strstr(header_buf, "\r\n\r\n") != NULL)
            {
                headers_done = true;
                break;
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (!headers_done)
    {
        ESP_LOGE(TAG, "Failed to read HTTP headers");
        return ANEDYA_EXT_ERR;
    }

    // --- Parse Content-Length ---
    char *line = strtok(header_buf, "\r\n");
    while (line != NULL)
    {
        if (strstr(line, "Content-Length:") != NULL && !content_length_found)
        {
            int parsed_len = -1;
            if (sscanf(line, "Content-Length: %d", &parsed_len) == 1)
            {
                content_length = parsed_len;
                content_length_found = true;
                reader->content_length = content_length;
                ESP_LOGI(TAG, "Parsed Content-Length: %d", content_length);
            }
        }
        line = strtok(NULL, "\r\n");
    }

    if (!content_length_found)
    {
        ESP_LOGE(TAG, "Content-Length header not found");
        return ANEDYA_EXT_ERR;
    }

    // You can now read binary asset content from UART here if needed

    return ANEDYA_OK;
}

size_t anedya_ext_ota_read_next(anedya_client_t *client, anedya_ext_net_reader_t *reader, int read_len, char *output, int timeout)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return 0;
    }

    size_t read = uart_read_bytes(UART_PORT_NUMBER, output, read_len, pdMS_TO_TICKS(timeout));
    if (read == 0)
    {
        ESP_LOGI(TAG, "Read error %d", read);
        return read;
    }
    // ESP_LOGI(TAG, "Read %d bytes from uARt", read);
    reader->bytes_read += read;
    return read;
}

anedya_err_t anedya_ext_ota_reader_close(anedya_client_t *client, anedya_ext_net_reader_t *reader)
{
    if (UART_PORT_NUMBER == -1)
    {
        ESP_LOGE(TAG, "Invalid port number");
        return ANEDYA_EXT_ERR;
    }
    _anedya_ext_clear_uart_buffer(client); // Clear the UART buffer
    xEventGroupSetBits(ModemEvents, MODEM_EVENT_OTA_NOT_IN_PROGRESS);
    xSemaphoreGive(uart_port_mutex);
    // ESP_LOGI(TAG, "Reset the queue");
    return ANEDYA_OK;
}

#endif // AN_INTERFACE_ESP32_QUETEL