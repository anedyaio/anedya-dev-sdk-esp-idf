#include <stdio.h>
#include "anedya.h"
#include "anedya_op_commands.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "sync.h"
#include "timeManagement.h"
#include "otaManagement.h"
#include "submitData.h"
#include "valueStore.h"
#include "commandHandler.h"
#include "submitLog.h"

sync_data_t gatewaystate;
anedya_config_t anedya_client_config;
anedya_client_t anedya_client;

static const char *TAG = "MAIN";

static uint32_t ulNotifiedValue;
static TaskHandle_t current_task;
static EventGroupHandle_t event_group;

anedya_command_obj_t *command_obj = NULL;

static void MQTT_ON_Connect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On connect handler");
    EventGroupHandle_t *handle = (EventGroupHandle_t *)ctx;
    xEventGroupSetBits(*handle, BIT3);
    xEventGroupSetBits(ConnectionEvents, MQTT_CONNECTED_BIT);
}

static void MQTT_ON_Disconnect(anedya_context_t ctx)
{
    ESP_LOGI("CLIENT", "On disconnect handler");
    xEventGroupClearBits(ConnectionEvents, MQTT_CONNECTED_BIT);
}

void cl_event_handler(anedya_client_t *client, anedya_event_t event, void *event_data)
{
    switch (event)
    {
    //=============================================== Commands Handler ======================================================
    // For more info visit: https://docs.anedya.io/commands/intro/
    case ANEDYA_EVENT_COMMAND:
        static anedya_command_obj_t local_command_obj; // Persistent memory
        memcpy(&local_command_obj, (anedya_command_obj_t *)event_data, sizeof(anedya_command_obj_t));
        command_obj = &local_command_obj;
        ESP_LOGI("COMMAND_EVENT", " Received Commands: %s\n", command_obj->cmdId);
        xEventGroupSetBits(gatewaystate.COMMANDEVENTS, COMMAND_AVAILABLE_BIT);
        break;

    // ============================================== Valuestore Handler =====================================================
    //  It Will be called when valuestore is updated
    // For more info visit: https://docs.anedya.io/valuestore/
    case ANEDYA_EVENT_VS_UPDATE_FLOAT:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: float");
        anedya_valuestore_obj_float_t *data = (anedya_valuestore_obj_float_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%f", data->key, data->value);
        break;
    case ANEDYA_EVENT_VS_UPDATE_BOOL:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: bool");
        anedya_valuestore_obj_bool_t *data_bool = (anedya_valuestore_obj_bool_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%d", data_bool->key, data_bool->value);
        break;
    case ANEDYA_EVENT_VS_UPDATE_STRING:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: string");
        anedya_valuestore_obj_string_t *data_str = (anedya_valuestore_obj_string_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%s", data_str->key, data_str->value);
        break;
    case ANEDYA_EVENT_VS_UPDATE_BIN:
        printf(" Received Events \n");
        ESP_LOGI("CLIENT", "Valuestore update notified: bin");
        anedya_valuestore_obj_bin_t *data_bin = (anedya_valuestore_obj_bin_t *)event_data;
        ESP_LOGI("CLIENT", "Key Updated: %s Value:%s", data_bin->key, data_bin->value);
        break;

        // ======================================================================================================================
    }
}

void app_main(void)
{
    char connkey[64] = CONFIG_CONNECTION_KEY; // Connection Key to connect to anedya
    anedya_device_id_t devid;
    anedya_parse_device_id(CONFIG_PHYSICAL_DEVICE_ID, devid); // UUID of the device

    current_task = xTaskGetCurrentTaskHandle();
    event_group = xEventGroupCreate();
    ConnectionEvents = xEventGroupCreate();
    OtaEvents = xEventGroupCreate();
    gatewaystate.DeviceTimeEvents = xEventGroupCreate();
    gatewaystate.COMMANDEVENTS = xEventGroupCreate();

    xEventGroupSetBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);

    // ============================================= Anedya Config =============================================================
    anedya_config_init(&anedya_client_config, devid, connkey, strlen(connkey));          // Initialize config
    anedya_config_set_connect_cb(&anedya_client_config, MQTT_ON_Connect, &event_group);  // Set connect callback
    anedya_config_set_disconnect_cb(&anedya_client_config, MQTT_ON_Disconnect, NULL);    // Set disconnect callback
    anedya_config_register_event_handler(&anedya_client_config, cl_event_handler, NULL); // Set event handler
    anedya_config_set_region(&anedya_client_config, ANEDYA_REGION_AP_IN_1);              // Set region
    anedya_config_set_timeout(&anedya_client_config, 30000);                             // Set timeout

#define APN_COUNT 8
    anedya_ext_apn_config_t apn_config[APN_COUNT] = {
        {.cid = 1, .ip_ver = "IPV4V6", .apn = "airteliot.com"},
        {.cid = 3, .ip_ver = "IPV4V6", .apn = "Jionet"},
        {.cid = 3, .ip_ver = "IP", .apn = "airtelgprs.com"},
        {.cid = 4, .ip_ver = "IP", .apn = "www"},
        {.cid = 5, .ip_ver = "IPV4V6", .apn = "ntnet"},
        {.cid = 5, .ip_ver = "IP", .apn = "ntnet"},
        {.cid = 6, .ip_ver = "IPV4V6", .apn = "web"},
        {.cid = 6, .ip_ver = "IP", .apn = "web"},
    };

    anedya_ext_config_t anedya_ext_config = {
        .uart_port_num = UART_NUM_2,
        .tx_pin = CONFIG_MCU_TX_PIN,
        .rx_pin = CONFIG_MCU_RX_PIN,
        .rts_pin = CONFIG_MCU_RTS_PIN,
        .cts_pin = CONFIG_MCU_CTS_PIN,
        .uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
        },
        .apn_count = APN_COUNT,
        .apn_configs = apn_config,
    };

    anedya_client_config.interface_config = &anedya_ext_config;

    // Restart the modem, in case it gets hold
    gpio_set_direction(CONFIG_MCU_QUECTEL_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_MCU_QUECTEL_RST, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    gpio_set_level(CONFIG_MCU_QUECTEL_RST, 0);

    // Start the modem
    gpio_set_direction(CONFIG_MCU_QUECTEL_PWR, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_MCU_QUECTEL_PWR, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    gpio_set_level(CONFIG_MCU_QUECTEL_PWR, 0);

    ESP_LOGI(TAG, "Waiting for 10s to get modem start...");
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    // Initialize client
    anedya_client_init(&anedya_client_config, &anedya_client);
    anedya_err_t aerr = anedya_client_connect(&anedya_client);
    if (aerr != ANEDYA_OK)
    {
        ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
    }
    ESP_LOGI("CLIENT", "Waiting for MQTT Connection");
    xEventGroupWaitBits(event_group, BIT3, pdFALSE, pdFALSE, 30000 / portTICK_PERIOD_MS);
    ESP_LOGI("Prov", "MQTT Connected");
    xTaskCreate(&syncTime_task, "syncTime", 4096, &gatewaystate, 4, NULL);
    xEventGroupWaitBits(gatewaystate.DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

// =============================================== Operations ================================================================
// =================== Provisioning ========================
#ifdef CONFIG_EN_PROVISIONING
    anedya_txn_t bind_txn;
    anedya_txn_register_callback(&bind_txn, TXN_COMPLETE, &current_task);
    anedya_req_bind_device_t req = {
        .binding_secret = CONFIG_BINDING_KEY,
        .binding_secret_len = strlen(CONFIG_BINDING_KEY)};
    ESP_LOGI("Prov", "Sending Binding Details");
    aerr = anedya_device_bind_req(&anedya_client, &bind_txn, &req);
    if (aerr != ANEDYA_OK)
    {
        ESP_LOGI("Prov", "%d", aerr);
    }

    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    ESP_LOGI("Prov", "Waiting for TXN to complete!");

    if (ulNotifiedValue == 0x01)
    {
        if (bind_txn.is_complete)
        {
            if (bind_txn.is_success)
            {
                ESP_LOGI("Prov", "Binding Successful");
            }
            else
            {
                ESP_LOGI("Prov", "Binding Failed");
            }
        }
    }
    else
    {
        ESP_LOGI("Binding", "TXN Timeout");
    }
    ulNotifiedValue = 0x00;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif

    xTaskCreate(ota_management_task, "OTA", 20240, &gatewaystate, 1, NULL);    // Start OTA Task
    xTaskCreate(submitData_task, "SUBMITDATA", 6096, NULL, 2, NULL);           // Start Submit Data Task
    xTaskCreate(valueStore_task, "VALUESTORE", 10240, NULL, 4, NULL);          // Start Valuestore Task
    xTaskCreate(commandHandling_task, "COMMANDHANDLER", 10240, NULL, 1, NULL); // Start Command Handler
    xTaskCreate(submitLog_task, "SUBMITLOG", 6096, NULL, 4, NULL);             // Start Submit Log

    for (;;)
    {
        // ================================================ Send Heartbeat to Anedya ================================================
        // For more info visit: https://docs.anedya.io/getting-started/quickstart/

        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        anedya_txn_t hb_txn;
        anedya_txn_register_callback(&hb_txn, TXN_COMPLETE, &current_task);

        anedya_err_t aerr = anedya_device_send_heartbeat(&anedya_client, &hb_txn);
        if (aerr != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(aerr));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            ESP_LOGI(TAG,"--------------------------------------");
            ESP_LOGI(TAG, "Heartbeat sent to Anedya successfully");
            ESP_LOGI(TAG,"--------------------------------------");
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to sent heartbeat");
        }

        vTaskDelay(30000 / portTICK_PERIOD_MS);
        // ==========================================================================================================================
    }
}
