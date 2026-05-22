#include "anedya_sdk_config.h"

#ifdef CONFIG_AN_INTERFACE_ESP32_WIFI_QUETEL
#include "anedya_interface.h"

// Interface for the ESP32  Wifi-Quectel

#include "anedya_esp_wifi_quectel_interface.h"
#include "anedya_certs.h"
#include "anedya_client.h"
#include "anedya_commons.h"
#include "time.h"
#include <sys/time.h>
#include <math.h>

static const char *TAG = "Anedya";

static short network = Anedya_Network_Interface_UNDEFINED;

anedya_err_t anedya_choose_network_interface(short network_interface)
{
    switch (network_interface)
    {
    case Anedya_Network_Interface_WIFI:
        ESP_LOGI(TAG, "Anedya_Network_Interface_WIFI");
        network = Anedya_Network_Interface_WIFI;
        break;
    case Anedya_Network_Interface_QUECTEL:
        network = Anedya_Network_Interface_QUECTEL;
        break;
    default:
        network = Anedya_Network_Interface_UNDEFINED;
        return ANEDYA_ERR;
        break;
    }
    return ANEDYA_ERR;
}

anedya_err_t _anedya_interface_init(anedya_client_t *client)
{
    if (network == Anedya_Network_Interface_WIFI)
    {
        return ANEDYA_OK;
    }
    else if (network == Anedya_Network_Interface_QUECTEL)
        return _anedya_quectel_interface_init(client);
    return ANEDYA_ERR;
}

uint64_t _anedya_interface_get_time_ms()
{
    if (network == Anedya_Network_Interface_WIFI)
        return _anedya_wifi_interface_get_time_ms();
    else if (network == Anedya_Network_Interface_QUECTEL)
        return _anedya_quectel_interface_get_time_ms();

    return 0;
}

void _anedya_interface_std_out(const char *str)
{
    if (network == Anedya_Network_Interface_WIFI)
        _anedya_wifi_interface_std_out(str);
    else if (network == Anedya_Network_Interface_QUECTEL)
        _anedya_quectel_interface_std_out(str);
}

static void anedya_espi_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    if (network == Anedya_Network_Interface_WIFI)
        anedya_wifi_espi_mqtt_event_handler(handler_args, base, event_id, event_data);
}

#ifdef ANEDYA_CONNECTION_METHOD_MQTT

anedya_mqtt_client_handle_t _anedya_interface_mqtt_init(anedya_client_t *parent, char *broker, const char *devid, const char *secret)
{
    esp_mqtt_client_handle_t client;
    if (network == Anedya_Network_Interface_WIFI)
        return _anedya_wifi_interface_mqtt_init(parent, broker, devid, secret);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return _anedya_quectel_interface_mqtt_init(parent, broker, devid, secret);
    return (void *)&client;
}

anedya_err_t anedya_interface_mqtt_connect(anedya_mqtt_client_handle_t anclient)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_connect(anclient);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_connect(anclient);
    return ANEDYA_ERR;
}

anedya_err_t anedya_interface_mqtt_disconnect(anedya_mqtt_client_handle_t anclient)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_disconnect(anclient);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_disconnect(anclient);
    return ANEDYA_ERR;
}

anedya_err_t anedya_interface_mqtt_destroy(anedya_mqtt_client_handle_t anclient)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_destroy(anclient);
    if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_destroy(anclient);
    return ANEDYA_ERR;
}

size_t anedya_interface_mqtt_status(anedya_mqtt_client_handle_t anclient)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_status(anclient);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_status(anclient);

    return 0;
}

anedya_err_t anedya_interface_mqtt_subscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topilc_len, int qos)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_subscribe(anclient, topic, topilc_len, qos);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_subscribe(anclient, topic, topilc_len, qos);
    return ANEDYA_ERR;
}

anedya_err_t anedya_interface_mqtt_unsubscribe(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_unsubscribe(anclient, topic, topic_len);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_unsubscribe(anclient, topic, topic_len);
    return ANEDYA_ERR;
}

anedya_err_t anedya_interface_mqtt_publish(anedya_mqtt_client_handle_t anclient, char *topic, int topic_len, char *payload, int payload_len, int qos, int retain)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_interface_mqtt_publish(anclient, topic, topic_len, payload, payload_len, qos, retain);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_interface_mqtt_publish(anclient, topic, topic_len, payload, payload_len, qos, retain);
    return ANEDYA_ERR;
}

anedya_err_t anedya_set_message_callback(anedya_mqtt_client_handle_t anclient, anedya_client_t *client)
{
    if (network == Anedya_Network_Interface_WIFI)
        return anedya_wifi_set_message_callback(anclient, client);
    else if (network == Anedya_Network_Interface_QUECTEL)
        return anedya_quectel_set_message_callback(anclient, client);
    return ANEDYA_ERR;
}

#endif

#endif // AN_INTERFACE_ESP32_WIFI_QUECTEL