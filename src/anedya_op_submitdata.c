#include "anedya_operations.h"
#include "anedya_op_submitdata.h"

anedya_err_t anedya_op_submit_float_req(anedya_client_t *client, anedya_txn_t *txn, const char *variable_identifier, float value, uint64_t timestamp_ms)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_SUBMIT_DATA;
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
    p = anedya_json_arrOpen(p, "data", &marker);
    p = anedya_json_objOpen(p, NULL, &marker);
    p = anedya_json_str(p, "variable", variable_identifier, &marker);
    p = anedya_json_double(p, "value", value, &marker);
    p = anedya_json_verylong(p, "timestamp", timestamp_ms, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_arrClose(p, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/submitdata/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_op_submit_geo_req(anedya_client_t *client, anedya_txn_t *txn, const char *variable_identifier, anedya_geo_data_t *value, uint64_t timestamp_ms) {
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_SUBMIT_DATA;
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
    p = anedya_json_arrOpen(p, "data", &marker);
    p = anedya_json_objOpen(p, NULL, &marker);
    p = anedya_json_str(p, "variable", variable_identifier, &marker);
    p = anedya_json_objOpen(p, "value", &marker);
    p = anedya_json_double(p, "lat", value->lat, &marker);
    p = anedya_json_double(p, "long",value->lon, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_verylong(p, "timestamp", timestamp_ms, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_arrClose(p, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);
    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/submitdata/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}