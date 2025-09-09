#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_AN_INTERFACE_ESP32_QUETEL

#include "anedya_sdk_config.h"

#ifdef ___cplusplus
extern "C"
{
#endif

#include "esp_err.h"
#include "anedya_commons.h"
#include "anedya_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include <sys/param.h>
#include "mqtt_client.h"
#include "esp_tls.h"
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"


  typedef struct
  {
    int cid;
    const char *ip_ver;
    const char *apn;
    const char *username;
    const char *password;
  } anedya_ext_apn_config_t;

  typedef struct
  {
    unsigned int uart_port_num;
    unsigned int tx_pin;
    unsigned int rx_pin;
    unsigned int rts_pin;
    unsigned int cts_pin;
    uart_config_t uart_config;
    QueueHandle_t uart_queue_handle;
    anedya_ext_apn_config_t *apn_configs;
    int apn_count;
  } anedya_ext_config_t;

#define EXT_MQTT_EVENT_CONNECTED 1
#define EXT_MQTT_EVENT_DISCONNECTED 2
#define EXT_MQTT_EVENT_DATA 3

  typedef struct
  {
    int event_id;
    char *data;
    int data_len;
    char *topic;
    int topic_len;
  } anedya_ext_mqtt_event_t;

  typedef struct
  {
    int content_length;
    int bytes_read;
  } anedya_ext_net_reader_t;

  /**
   * @brief Checks if the modem is connected via UART
   *
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the modem is connected
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the modem is not connected
   *
   * @warning Must uart initialized before calling this function.
   */
  anedya_err_t anedya_ext_connectivity_check(anedya_client_t *client, int timeout);

  /**
   * @brief Restores the modem settings to factory defaults
   *
   * @param client Pointer to the Anedya client structure containing configuration
   * @param timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if fail to set
   *
   * @warning Must uart initialized before calling this function.
   */
  anedya_err_t anedya_ext_restore_settings_to_factory_defaults(anedya_client_t *client, int timeout);

  /**
   * @brief Sets the modem's functionality mode
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] fun The new functionality mode
   * @param[in] rst The new reset mode
   * @param[in] wait_for_rdy Whether to wait for the RDY response after setting the mode
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if fail to set
   *
   * @warning Must uart initialized before calling this function.
   */
  anedya_err_t anedya_ext_set_fun_mode(anedya_client_t *client, int fun, int rst, int timeout);

  /**
   * @brief Gets the current network registration status
   *
   * @param client[in] Pointer to the Anedya client structure containing configuration
   * @param stat[out] Pointer to an integer to store the current network registration status
   * @param timeout[in] The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if fail to get
   *
   * @warning Must uart initialized before calling this function.
   */
  anedya_err_t anedya_ext_network_reg_status(anedya_client_t *client, int *stat, int timeout);

  /**
   * @brief Retrieves the current network operator mode
   *
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[out] mode Pointer to an integer to store the current network operator mode
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   *  @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `mode` pointer is not NULL.
   */
  anedya_err_t anedya_ext_network_operator(anedya_client_t *client, int *mode, int timeout);

  /**
   * @brief Retrieves the current signal quality
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[out] rssi Pointer to an integer to store the signal strength in dBm
   * @param[out] ber Pointer to an integer to store the signal quality in error rate
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `rssi` pointer is not NULL.
   * @note If `ber` is NULL, only the signal strength is retrieved.
   */
  anedya_err_t anedya_ext_signal_quality(anedya_client_t *client, int *rssi, int *ber, int timeout);

  /**
   * @brief Retrieves the current PDP context status
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[out] pdp_context_out Buffer to store the PDP context information
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `pdp_context_out` buffer is adequately sized to hold the response.
   */
  anedya_err_t anedya_ext_pdp_context_status(anedya_client_t *client, char *pdp_context_out, int timeout);

  /**
   * @brief Activates the specified PDP context
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] cid The identifier of the PDP context to activate
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `cid` is a valid PDP context identifier.
   */
  anedya_err_t anedya_ext_activate_pdp_context(anedya_client_t *client, int cid, int timeout);

  /**
   * @brief Deactivates the specified PDP context
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] cid The identifier of the PDP context to deactivate
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `cid` is a valid PDP context identifier.
   */
  anedya_err_t anedya_ext_deactivate_pdp_context(anedya_client_t *client, int cid, int timeout);

  /**
   * @brief Reads the PDP context from the modem
   *
   * This function retrieves the current PDP context from the modem and stores it in the provided buffer.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[out] pdp_context_out Buffer to store the PDP context response
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `pdp_context_out` buffer is adequately sized to hold the response.
   */
  anedya_err_t anedya_ext_read_pdp_context(anedya_client_t *client, char *pdp_context_out, int timeout);

  /**
   * @brief Checks network connectivity
   *
   * This function attempts to ping the specified URL to check if it is reachable.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] url The URL to ping (e.g., www.google.com)
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `url` is a valid URL.
   */
  anedya_err_t anedya_ext_net_check(anedya_client_t *client, char *url, int url_len, int timeout);

  /**
   * @brief Sets the Access Point Name (APN) for a specified PDP context
   *
   * This function configures the APN settings for a given PDP context using the provided parameters.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] cid The identifier of the PDP context to configure
   * @param[in] ip_ver The IP version to use (e.g., "IP", "IPV6", or "IPV4V6")
   * @param[in] apn The Access Point Name to set for the PDP context
   * @param[in] user Optional username for APN authentication (can be NULL if not required)
   * @param[in] pass Optional password for APN authentication (can be NULL if not required)
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `apn` pointer is not NULL.
   */
  anedya_err_t anedya_ext_set_apn(anedya_client_t *client, int cid, char *ip_ver, char *apn, char *user, char *pass);

  /**
   * @brief Gets the current date and time from the modem
   *
   * This function sends an AT command to the modem to retrieve the current date and time.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] mode The AT command mode to use (e.g., 0 for current time, 1 for local time)
   * @param[out] output_dateTime The retrieved date and time in the format "YYYY-MM-DD HH:MM:SS"
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `output_dateTime` pointer is not NULL.
   */
  anedya_err_t anedya_ext_get_modem_time(anedya_client_t *client, int mode, char *output_dateTime);

  // OTA related functions
  /**
   * @brief Function to send a partial HTTP GET request
   *
   * This function sends a partial HTTP GET request to the specified URL.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] reader Pointer to the Anedya HTTP reader structure
   * @param[in] url The URL to download from
   * @param[in] url_len The length of the URL
   * @param[in] starting_position The starting byte position to read from
   * @param[in] readlen The number of bytes to read
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `url` is a valid URL and that the `starting_position` and `readlen` parameters are valid.
   */
  anedya_err_t anedya_ext_http_get_range_request(anedya_client_t *client, anedya_ext_net_reader_t *reader, char *url, int url_len, int starting_position, int readlen, int timeout);

  /**
   * @brief Reads from the UART interface for the specified number of bytes
   *
   * This function reads from the UART interface for the specified number of bytes.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] reader Pointer to the Anedya HTTP reader structure
   * @param[in] read_len The number of bytes to read
   * @param[out] output The buffer to store the read data
   * @param[in] timeout The maximum time to wait for the operation to complete
   *
   * @return size_t
   * @retval - The number of bytes read
   * @retval - 0 if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `read_len` parameter is valid and that the `output` pointer is not NULL.
   */
  size_t anedya_ext_ota_read_next(anedya_client_t *client, anedya_ext_net_reader_t *reader, int read_len, char *output, int timeout);

  /**
   * @brief Closes the OTA reader
   *
   * This function closes the OTA reader and releases the UART lock.
   *
   * @param[in] client Pointer to the Anedya client structure containing configuration
   * @param[in] reader Pointer to the Anedya HTTP reader structure
   *
   * @return anedya_err_t
   * @retval - `ANEDYA_OK` if the operation is successful
   * @retval - `ANEDYA_ERR_EXT_TIMEOUT` if the operation times out
   * @retval - `ANEDYA_EXT_ERR` if there is an error in communication or invalid parameters
   *
   * @warning The UART must be initialized before calling this function.
   * @note Ensure that the `reader` pointer is not NULL.
   */
  anedya_err_t anedya_ext_ota_reader_close(anedya_client_t *client, anedya_ext_net_reader_t *reader);

#ifdef __cplusplus
}
#endif

#endif // AN_INTERFACE_ESP32_QUETEL