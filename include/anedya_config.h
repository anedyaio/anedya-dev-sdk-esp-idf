#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#define UUID_STRING_LEN 37

    typedef struct
    {
        size_t timeout;
        const char *region;
        const char *connection_key;
        unsigned int connection_key_len;
        anedya_device_id_t device_id;
        anedya_device_id_str_t _device_id_str;
// MQTT Callbacks
#ifdef ANEDYA_CONNECTION_METHOD_MQTT
        anedya_on_connect_cb_t on_connect;
        anedya_context_t on_connect_ctx;
        anedya_on_disconnect_cb_t on_disconnect;
        anedya_context_t on_disconnect_ctx;
        anedya_event_handler_t event_handler;
        anedya_context_t event_handler_ctx;
#endif
// In case of dynamic memory allocation, set the initial sizes of buffers
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
        size_t tx_buffer_size;
        size_t rx_buffer_size;
#ifdef ANEDYA_ENABLE_DEVICE_LOGS
        size_t log_buffer_size;
        size_t log_batch_size;
#endif
        size_t datapoints_batch_size;
#endif
    } anedya_config_t;

    anedya_err_t anedya_parse_device_id(const char deviceID[37], anedya_device_id_t devID);

    // Config management APIs
    /** @brief: Initialize a config with default values*/
    anedya_err_t anedya_config_init(anedya_config_t *config, anedya_device_id_t devId, const char *connection_key, size_t connection_key_len);

    anedya_err_t anedya_config_set_region(anedya_config_t *config, const char *region);

    anedya_err_t anedya_config_set_timeout(anedya_config_t *config, size_t timeout_sec);

#ifdef ANEDYA_CONNECTION_METHOD_MQTT

    anedya_err_t anedya_config_set_connect_cb(anedya_config_t *config, anedya_on_connect_cb_t on_connect, anedya_context_t ctx);

    anedya_err_t anedya_config_set_disconnect_cb(anedya_config_t *config, anedya_on_disconnect_cb_t on_disconnect, anedya_context_t ctx);

    anedya_err_t anedya_config_register_event_handler(anedya_config_t *config, anedya_event_handler_t event_handler, anedya_context_t ctx);

#endif

#ifdef __cplusplus
}
#endif