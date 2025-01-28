#pragma once

#ifndef _ANEDYA_OPERATIONS_H_
#define _ANEDYA_OPERATIONS_H_

#include <stdio.h>
#include "anedya.h"
#include "anedya_json_builder.h"
#include "anedya_json_parse.h"
#include "anedya_ota.h"
#include "anedya_op_vs.h"
#include "anedya_op_submitevent.h"
#include "anedya_op_submitdata.h"
#include "anedya_op_commands.h"
#include "string.h"

// Anedya Operation codes

#define ANEDYA_OP_BIND_DEVICE 1
#define ANEDYA_OP_HEARTBEAT 2
#define ANEDYA_OP_OTA_NEXT 3
#define ANEDYA_OP_SUBMIT_DATA 4
#define ANEDYA_OP_OTA_UPDATE_STATUS 5
#define ANEDYA_OP_VALUESTORE_SET 6
#define ANEDYA_OP_SUBMIT_EVENT 7
#define ANEDYA_OP_CMD_UPDATE_STATUS 8
#define ANEDYA_OP_SUBMIT_LOG 9
#define ANEDYA_OP_VALUESTORE_GET 10
#define ANEDYA_OP_VALUESTORE_GET_LIST 11
#define ANEDYA_OP_VALUESTORE_DELETE 12
#define ANEDYA_OP_CMD_QUEUED_OBJ 13
#define ANEDYA_OP_CMD_NEXT 14

// Anedya Events
#define ANEDYA_EVENT_VS_UPDATE_FLOAT 1
#define ANEDYA_EVENT_VS_UPDATE_BOOL 2
#define ANEDYA_EVENT_COMMAND 3
#define ANEDYA_EVENT_VS_UPDATE_STRING 4
#define ANEDYA_EVENT_VS_UPDATE_BIN 5

// Anedya Operations Error Codes
#define ANEDYA_OP_ERR_RESP_BUFFER_OVERFLOW -1

typedef struct
{
    bool success;
    char *error_msg;
    size_t error;
} anedya_generic_resp_t;

// =========================== Anedya Operations and Requests =========================

/**
 * @brief Send a device bind request to the server.
 *
 * This function initiates a binding request for the device, creating a transaction and generating
 * the required JSON payload for binding. The request is published to the MQTT server if the client
 * is connected.
 *
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the bind request transaction.
 * @param[in] req_config Pointer to the `anedya_req_bind_device_t` structure containing binding configuration details.
 *
 * @retval - `ANEDYA_OK` if the bind request is successfully sent.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_device_bind_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_bind_device_t *req_config);

/**
 * @brief Send a heartbeat signal to the server.
 *
 * This function sends a heartbeat signal from the device to the server to maintain an active
 * connection status.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the heartbeat transaction.
 *
 * @retval - `ANEDYA_OK` if the heartbeat request is successfully sent.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_device_send_heartbeat(anedya_client_t *client, anedya_txn_t *txn);

/**
 * @brief Request the next OTA (Over-the-Air) deployment details from the server.
 *
 * This function sends a request for the next available OTA deployment. It initializes the
 * response structure to indicate no deployment available initially and creates a transaction
 * with the required JSON payload if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the OTA request transaction.
 *                 The response (`txn->response`) will be populated with OTA deployment details.
 *
 * @retval - `ANEDYA_OK` if the OTA request is successfully sent.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_ota_next_req(anedya_client_t *client, anedya_txn_t *txn);

/**
 * @brief Send an OTA update status to the server.
 *
 * This function sends the status of an OTA update for a specific deployment to the server. It
 * creates a transaction, generates the JSON payload with deployment details, and publishes it
 * to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the OTA update status transaction.
 * @param[in] req Pointer to the `anedya_req_ota_update_status_t` structure containing the deployment
 *                ID and status to be sent.
 *
 * @retval - `ANEDYA_OK` if the OTA update status is successfully sent.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_ota_update_status_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_ota_update_status_t *req);

/**
 * @brief Submit a floating-point data value to the server.
 *
 * This function sends a floating-point value with a specific variable identifier and timestamp
 * to the server. It initializes a transaction, generates a JSON payload for the data submission,
 * and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the data submission transaction.
 * @param[in] variable_identifier A string representing the unique identifier for the variable.
 * @param[in] value Floating-point value to be submitted.
 * @param[in] timestamp_ms Timestamp of the data point in milliseconds.
 *
 * @retval - `ANEDYA_OK` if the data is successfully submitted.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_submit_float_req(anedya_client_t *client, anedya_txn_t *txn, const char *variable_identifier, float value, uint64_t timestamp_ms);

/**
 * @brief Submit a geo-coordinate data value to the server.
 *
 * This function sends a floating-point value with a specific variable identifier and timestamp
 * to the server. It initializes a transaction, generates a JSON payload for the data submission,
 * and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the data submission transaction.
 * @param[in] variable_identifier A string representing the unique identifier for the variable.
 * @param[in] value Geo-coordinate value to be submited
 * @param[in] timestamp_ms Timestamp of the data point in milliseconds.
 *
 * @retval - `ANEDYA_OK` if the data is successfully submitted.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_submit_geo_req(anedya_client_t *client, anedya_txn_t *txn, const char *variable_identifier, anedya_geo_data_t *value, uint64_t timestamp_ms);

/**
 * @brief Set a string value in the valuestore at anedya.
 *
 * This function allows setting a string value associated with a specific key in the
 * device's valuestore. It initializes a transaction, creates a JSON payload with the provided
 * key and value, and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] key A string representing the unique identifier (key) for the value in the valuestore.
 * @param[in] value The string value to be stored in the valuestore.
 * @param[in] value_len The length of the string value.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - `ANEDYA_ERR_VALUE_TOO_LONG` if the string value is longer than 1000 bytes.
 * @retval - `ANEDYA_ERR_VALUE_MISMATCH_LEN` if the string value length does not match the provided length.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_set_string(anedya_client_t *client, anedya_txn_t *txn, const char *key, const char *value, size_t value_len);

/**
 * @brief Set a floating-point value in the valuestore at anedya.
 *
 * This function allows setting a floating-point value associated with a specific key in the
 * device's valuestore. It initializes a transaction, creates a JSON payload with the provided
 * key and value, and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] key A string representing the unique identifier (key) for the value in the valuestore.
 * @param[in] value The floating-point value to be stored in the valuestore.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_set_float(anedya_client_t *client, anedya_txn_t *txn, const char *key, float value);

/**
 * @brief Set a boolean-point value in the valuestore at anedya.
 *
 * This function allows setting a boolean-point value associated with a specific key in the
 * device's valuestore. It initializes a transaction, creates a JSON payload with the provided
 * key and value, and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] key A string representing the unique identifier (key) for the value in the valuestore.
 * @param[in] value The floating-point value to be stored in the valuestore.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_set_bool(anedya_client_t *client, anedya_txn_t *txn, const char *key, bool value);

/**
 * @brief Set a binary value in the valuestore at anedya.
 *
 * This function allows setting a binary value encoded in Base64 associated with a specific key
 * in the device's valuestore. It initializes a transaction, creates a JSON payload with the
 * provided key and value, and publishes it to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] key A string representing the unique identifier (key) for the value in the valuestore.
 * @param[in] base64_value The binary value, encoded in Base64, to be stored in the valuestore.
 * @param[in] base64_value_len The length of the Base64 encoded value.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - `ANEDYA_ERR_VALUE_TOO_LONG` if the Base64 encoded value is longer than 1000 bytes.
 * @retval - `ANEDYA_ERR_VALUE_MISMATCH_LEN` if the length of the Base64 encoded value does not match the provided length.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */

anedya_err_t anedya_op_valuestore_set_bin(anedya_client_t *client, anedya_txn_t *txn, const char *key, const char *base64_value, size_t base64_value_len);

/**
 * @brief Get the value associated with a key from the valuestore at anedya.
 *
 * This function retrieves a value associated with a specific key from the valuestore.
 * It initializes a transaction, creates a JSON payload with the provided key, and publishes it
 * to the server if the client is connected.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] obj Pointer to an `anedya_valuestore_get_key_t` structure containing the key to retrieve.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully retrieved.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_get_key(anedya_client_t *client, anedya_txn_t *txn, anedya_req_valuestore_get_key_t obj );

/**
 * @brief Get the list of the keys from the valuestore at anedya.
 *
 * This function retrieves a list of keys-value associated with a list of keys from the valuestore.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] obj Pointer to an `anedya_req_valuestore_list_obj_t` structure containing the list of keys to retrieve.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully retrieved.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_list_obj(anedya_client_t *client, anedya_txn_t *txn, anedya_req_valuestore_list_obj_t obj );

/**
 * @brief Delete a value from the valuestore at anedya.
 *
 * This function deletes a value associated with a specific key from the valuestore.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] key A string representing the unique identifier (key) for the value in the valuestore.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully deleted.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_valuestore_delete(anedya_client_t *client, anedya_txn_t *txn, const char *key);

/**
 * @brief Send an event to Anedya
 *
 * This function sends an event to Anedya.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[inout] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] req_config Pointer to the request config
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_submit_event(anedya_client_t *client, anedya_txn_t *txn, anedya_req_submit_event_t *req_config);

/**
 * @brief Update command status
 *
 * This function updates status of a received command to Anedya.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[inout] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] req_config Pointer to the request config
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_cmd_status_update(anedya_client_t *client, anedya_txn_t *txn, anedya_req_cmd_status_update_t *req_config);

/**
 * @brief List commands
 *
 * This function lists queued commands of a specific device
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[inout] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] obj Pointer to an `anedya_req_cmd_list_obj_t` structure containing the list of commands to retrieve.
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully retrieved.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_cmd_queued_obj(anedya_client_t *client, anedya_txn_t *txn, anedya_req_cmd_list_obj_t obj);
/**
 * @brief Retrieve the next command for execution from Anedya.
 *
 * This function retrieves the next available command for execution from the server.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[out] txn Pointer to an `anedya_txn_t` structure for the command transaction.
 *
 * @retval - `ANEDYA_OK` if the next command is successfully retrieved.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_cmd_next(anedya_client_t *client, anedya_txn_t *txn);

/**
 * @brief Send a log to Anedya
 *
 * This function sends a log to Anedya.
 *
 * @param[in] client Pointer to the `anedya_client_t` structure representing the client.
 * @param[inout] txn Pointer to an `anedya_txn_t` structure for the valuestore transaction.
 * @param[in] log Pointer to the log
 * @param[in] log_len Length of the log
 * @param[in] timestamp_ms Timestamp of the log
 *
 * @retval - `ANEDYA_OK` if the valuestore entry is successfully set.
 * @retval - `ANEDYA_ERR_NOT_CONNECTED` if the client is not connected to the server.
 * @retval - Error code if transaction registration or message publishing fails.
 *
 * @note Ensure the client is connected before calling this function.
 * @warning This function uses static or dynamic allocation based on configuration macros.
 *          Ensure the appropriate allocation macros are defined.
 */
anedya_err_t anedya_op_submit_log(anedya_client_t *client, anedya_txn_t *txn, char *log, unsigned int log_len, unsigned long long timestamp_ms);





//========================== Reponse handlers ===================================
void _anedya_device_handle_generic_resp(anedya_client_t *client, anedya_txn_t *txn);
void _anedya_op_ota_next_resp(anedya_client_t *client, anedya_txn_t *txn);

#endif