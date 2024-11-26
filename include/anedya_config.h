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

    /**
     * @brief Parses a device ID string into a binary array format.
     *
     * This function validates and parses a 36-character device ID string, formatted as a UUID
     * with hyphens at positions 8, 13, 18, and 23 (e.g., "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx").
     * The function converts the UUID string into a 16-byte binary format and stores it in the
     * provided `devID` array of type `anedya_device_id_t`.
     *
     * @param[in] deviceID The input device ID string in UUID format (36 characters including hyphens, plus the null-terminator).
     *                     It must contain 32 hexadecimal digits separated by hyphens.
     * @param[out] devID   An `anedya_device_id_t` array (16 bytes) that receives the parsed binary device ID.
     *
     * @retval `ANEDYA_OK` on successful parsing.
     * @retval `ANEDYA_ERR_INVALID_DEVICE_ID` if the input `deviceID` is incorrectly formatted.
     *
     * @note This function requires the device ID string to be exactly 36 characters in length, including
     *       hyphens. Each hexadecimal character is validated, and invalid characters result in an error.
     *
     * @warning The output array `devID` must be large enough to hold 16 bytes.
     */
    anedya_err_t anedya_parse_device_id(const char deviceID[37], anedya_device_id_t devID);

    /**
     * @brief Initializes the configuration structure for an Anedya client.
     *
     * This function sets up the `anedya_config_t` structure by assigning the connection key,
     * connection key length, and device ID. It also converts the binary device ID (`devId`)
     * into a UUID string format and stores it in the `_device_id_str` field of the configuration.
     * Additionally, all event handlers are initialized to `NULL`.
     *
     * @param[out] config            Pointer to the `anedya_config_t` structure to initialize.
     * @param[in]  devId             The binary device ID in the form of `anedya_device_id_t`.
     * @param[in]  connection_key    Pointer to a string containing the connection key.
     * @param[in]  connection_key_len Length of the connection key string.
     *
     * @return
     *      - `ANEDYA_OK` if initialization is successful.
     *
     * @note This function sets the event handler, on_connect, and on_disconnect callbacks to `NULL`.
     *       Ensure to set these callbacks if needed after initialization.
     *
     * @warning The `config` pointer must point to valid memory, as this function will write data
     *          directly to the fields of `config`.
     */
    anedya_err_t anedya_config_init(anedya_config_t *config, anedya_device_id_t devId, const char *connection_key, size_t connection_key_len);

    /**
     * @brief: Set the region of the device.
     *
     * The function sets the region of the device.
     *
     * @param[out] config    Pointer to the `anedya_config_t` structure.
     * @param[in] region    Pointer to the region string.(Possible values: ANEDYA_REGION_AP_IN_1)
     * @retval - `ANEDYA_OK` if successful.
     * @retval - `ANEDYA_ERR_INVALID_REGION` if the region is not supported.
     * @note        - This function sets the region of the device.
     * @note        - Please refer to the documentation for the supported regions (https://docs.anedya.io/device/#region).
     * @warning     - Ensure to set the region before connecting.
     */
    anedya_err_t anedya_config_set_region(anedya_config_t *config, const char *region);

    /**
     * @brief: Set the timeout to wait for the response from the device.
     *
     * The function sets the timeout to wait for the response from the Connectivity Server.
     *
     * @param[out] config    Pointer to the `anedya_config_t` structure.
     * @param[in] timeout   Timeout in seconds.
     * @retval - `ANEDYA_OK` if successful.
     * @note     - Ensure to set the timeout before connecting.
     */
    anedya_err_t anedya_config_set_timeout(anedya_config_t *config, size_t timeout_sec);

#ifdef ANEDYA_CONNECTION_METHOD_MQTT

    /**
     * @brief Sets the MQTT connection callback for the Anedya configuration.
     *
     * This function assigns a user-defined callback function to be called
     * when the client successfully connects to the Anedya platform via
     * MQTT. It also allows the user to set a context that will be passed
     * to the callback.
     *
     * @param[in] config Pointer to the `anedya_config_t` structure in which
     *                   the connection callback will be set.
     * @param[in] on_connect Pointer to the callback function to be invoked
     *                       upon successful connection. If this is NULL,
     *                       the function returns an error.
     * @param[in] ctx User-defined context that will be passed to the
     *                connection callback function. This can be used to
     *                store additional information needed during the callback.
     *
     * @retval - `ANEDYA_OK` on successful setting of the connection callback.
     * @retval - `ANEDYA_ERR` if the provided callback function is NULL.
     *
     * @note Ensure that the `config` structure is properly initialized before
     *       calling this function. The callback function should not be NULL;
     *       otherwise, an error will be returned.
     */
    anedya_err_t anedya_config_set_connect_cb(anedya_config_t *config, anedya_on_connect_cb_t on_connect, anedya_context_t ctx);

    /**
     * @brief Sets the disconnection callback for the Anedya configuration.
     *
     * This function assigns a user-defined callback function to be called
     * when the client disconnects from the Anedya platform. It also allows
     * the user to set a context that will be passed to the callback.
     *
     * @param[in] config Pointer to the `anedya_config_t` structure in which
     *                   the disconnection callback will be set.
     * @param[in] on_disconnect Pointer to the callback function to be invoked
     *                          upon disconnection. If this is NULL, the
     *                          function returns an error.
     * @param[in] ctx User-defined context that will be passed to the
     *                disconnection callback function. This can be used to
     *                store additional information needed during the callback.
     *
     * @retval - `ANEDYA_OK` on successful setting of the disconnection callback.
     * @retval - `ANEDYA_ERR` if the provided callback function is NULL.
     *
     * @note Ensure that the `config` structure is properly initialized before
     *       calling this function. The callback function should not be NULL;
     *       otherwise, an error will be returned.
     */
    anedya_err_t anedya_config_set_disconnect_cb(anedya_config_t *config, anedya_on_disconnect_cb_t on_disconnect, anedya_context_t ctx);

    /**
     * @brief Registers an event handler for the Anedya configuration.
     *
     * This function assigns a user-defined event handler that will be called
     * to process various events from the Anedya platform. It also allows the
     * user to set a context that will be passed to the event handler.
     *
     * @param[in] config Pointer to the `anedya_config_t` structure in which
     *                   the event handler will be registered.
     * @param[in] event_handler Pointer to the event handler function that will
     *                         be invoked when an event occurs. If this is
     *                         NULL, the function returns an error.
     * @param[in] ctx User-defined context that will be passed to the
     *                event handler function. This can be used to store
     *                additional information needed during event handling.
     *
     * @retval ANEDYA_OK on successful registration of the event handler.
     * @retval ANEDYA_ERR if the provided event handler function is NULL.
     *
     * @note Ensure that the `config` structure is properly initialized before
     *       calling this function. The event handler function should not be
     *       NULL; otherwise, an error will be returned.
     */
    anedya_err_t anedya_config_register_event_handler(anedya_config_t *config, anedya_event_handler_t event_handler, anedya_context_t ctx);

#endif

#ifdef __cplusplus
}
#endif