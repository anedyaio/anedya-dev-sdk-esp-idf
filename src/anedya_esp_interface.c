#include "anedya_esp_interface.h"
#include "anedya_interface.h"
#include "anedya_certs.h"
#include "anedya_client.h"
#include "anedya_commons.h"
#include "time.h"
#include <sys/time.h>

static const char *TAG = "ANEDYA_ESPI";

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

esp_mqtt_client_handle_t client;
bool anedya_espi_mqtt_connected = false;

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

static void anedya_espi_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;
    anedya_client_t *cl = (anedya_client_t *)handler_args;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        //ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Issue callback
        if (cl->_anedya_on_connect_handler != NULL)
        {
            cl->_anedya_on_connect_handler(cl);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        //ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        if (cl->_anedya_on_disconnect_handler != NULL)
        {
            cl->_anedya_on_disconnect_handler(cl);
        }
        break;

    case MQTT_EVENT_SUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        //printf("DATA=%.*s\r\n", event->data_len, event->data);
        cl->_message_handler(cl, event->topic, event->topic_len, event->data, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        }
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        }
        else
        {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

#ifdef ANEDYA_CONNECTION_METHOD_MQTT

anedya_mqtt_client_handle_t _anedya_interface_mqtt_init(anedya_client_t *parent, char *broker, const char *devid, const char *secret)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .network.disable_auto_reconnect = false,
        .outbox.limit = 1024,
        .broker = {
            .address.port = 8883,
            .address.hostname = broker,
            .address.transport = MQTT_TRANSPORT_OVER_SSL,
            .verification.certificate = (const char *)anedya_tls_root_ca,
            .verification.certificate_len = anedya_tls_root_ca_len,
        },
        .credentials = {
            .username = devid,
            .authentication.password = secret,
            .client_id = devid,
        },
    };
    //ESP_LOGI("ANEDYA_ESPI", "Connecting to MQTT secret: %s  Length:%d", secret, parent->config->connection_key_len);
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, anedya_espi_mqtt_event_handler, (void *)parent);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    return (void *)&client;
}

anedya_err_t anedya_interface_mqtt_connect(anedya_mqtt_client_handle_t anclient)
{
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    esp_err_t err = esp_mqtt_client_start(*c);
    if (err != ESP_OK)
    {
        return ANEDYA_ERR;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_disconnect(anedya_mqtt_client_handle_t anclient)
{
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    esp_err_t err = esp_mqtt_client_stop(*c);
    if (err != ESP_OK)
    {
        return ANEDYA_ERR;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_destroy(anedya_mqtt_client_handle_t anclient) {
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    esp_err_t err = esp_mqtt_client_destroy(*c);
    if (err != ESP_OK)
    {
        return ANEDYA_ERR;
    }
    return ANEDYA_OK;
}

size_t anedya_interface_mqtt_status(anedya_mqtt_client_handle_t anclient)
{
    if (anedya_espi_mqtt_connected)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

anedya_err_t anedya_interface_mqtt_subscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topilc_len, int qos)
{
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    int id = esp_mqtt_client_subscribe_single(*c, topic, qos);
    if (id == -1)
    {
        return ANEDYA_ERR;
    }
    if (id == -2)
    {
        return ANEDYA_INTERFACE_BUSY;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_unsubscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len)
{
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    int id = esp_mqtt_client_unsubscribe(*c, topic);
    if (id == -1)
    {
        return ANEDYA_ERR;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_interface_mqtt_publish(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len, char *payload, int payload_len, int qos, int retain)
{
    esp_mqtt_client_handle_t *c = (esp_mqtt_client_handle_t *)anclient;
    int id = esp_mqtt_client_publish(*c, topic, payload, payload_len, qos, retain);
    if (id == -1)
    {
        return ANEDYA_ERR;
    }
    if (id == -2)
    {
        return ANEDYA_INTERFACE_BUSY;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_set_message_callback(anedya_mqtt_client_handle_t anclient, anedya_client_t *client)
{
    return ANEDYA_OK;
}

void _anedya_interface_std_out(const char *str)
{
    ESP_LOGI("ANEDYA_ESPI", "%s", str);
}

#endif
