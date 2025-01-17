#include "anedya_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

anedya_err_t anedya_op_valuestore_set_float(anedya_client_t *client, anedya_txn_t *txn, const char *key, float value)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_VALUESTORE_SET;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK)
    {
        return err;
    }
// Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif
    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_str(p, "key", key, &marker);
    p = anedya_json_double(p, "value", value, &marker);
    p = anedya_json_str(p, "type", "float", &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/valuestore/setValue/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}
anedya_err_t anedya_op_valuestore_set_string(anedya_client_t *client, anedya_txn_t *txn, const char *key, const char *value, size_t value_len)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // Check the length of the value
    size_t value_length = strlen(value);  //todo : have to remove strlen
    if (value_length != value_len){
        return ANEDYA_ERR_VALUE_MISMATCH_LEN;
    }
    if (value_length > 1000)
    {
        return ANEDYA_ERR_VALUE_TOO_LONG;
    }

    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_VALUESTORE_SET;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK)
    {
        return err;
    }

    // Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
    // TODO: Implement dynamic allocation
#endif
    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_str(p, "key", key, &marker);
    p = anedya_json_str(p, "value", value, &marker);
    p = anedya_json_str(p, "type", "string", &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/valuestore/setValue/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}


anedya_err_t anedya_op_valuestore_set_bool(anedya_client_t *client, anedya_txn_t *txn, const char *key, bool value)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_VALUESTORE_SET;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK)
    {
        return err;
    }
// Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
// TODO: Implement dynamic allocation
#endif
    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_str(p, "key", key, &marker);
    p = anedya_json_bool(p, "value", value, &marker);
    p = anedya_json_str(p, "type", "boolean", &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/valuestore/setValue/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}



anedya_err_t anedya_op_valuestore_set_bin(anedya_client_t *client, anedya_txn_t *txn, const char *key, const char *base64_value, size_t base64_value_len) {
    // Check if the client is connected
    if (client->is_connected == 0) {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // Check the length of the Base64 value
    if (base64_value_len > 1000) {
        return ANEDYA_ERR_VALUE_TOO_LONG;
    }

        // Check the length of the value
    size_t value_length = strlen(base64_value);  //todo : have to remove strlen
    if (value_length != base64_value_len){
        return ANEDYA_ERR_VALUE_MISMATCH_LEN;
    }

    // If connected, create a transaction
    txn->_op = ANEDYA_OP_VALUESTORE_SET;
    anedya_err_t err = _anedya_txn_register(client, txn);
    if (err != ANEDYA_OK) {
        return err;
    }

    // Generate the JSON body
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char txbuffer[ANEDYA_TX_BUFFER_SIZE];
    size_t marker = sizeof(txbuffer);
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
    // TODO: Implement dynamic allocation
#endif
    char slot_number[4];
    int digitLen = snprintf(slot_number, sizeof(slot_number), "%d", txn->desc);
    char *p = anedya_json_objOpen(txbuffer, NULL, &marker);
    // Get the reqId based on slot.
    p = anedya_json_nstr(p, "reqId", slot_number, digitLen, &marker);
    p = anedya_json_str(p, "key", key, &marker);
    p = anedya_json_str(p, "value", base64_value, &marker); // Use the provided Base64 value
    p = anedya_json_str(p, "type", "binary", &marker); // Indicate that the value is Base64-encoded
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);


    // Body is ready; now publish it to MQTT
    char topic[100];
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/valuestore/setValue/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK) {
        return err;
    }

    return ANEDYA_OK;
}
