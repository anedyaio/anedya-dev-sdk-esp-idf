#include "anedya_operations.h"
#include "anedya_sdk_config.h"
#include "anedya_json_builder.h"
#include "anedya_json_parse.h"
#include "anedya_op_commands.h"
#include <string.h>
#include <stdio.h>

anedya_err_t anedya_device_bind_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_bind_device_t *req_config)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_BIND_DEVICE;
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
    p = anedya_json_nstr(p, "deviceid", client->config->_device_id_str, strlen((char *)client->config->_device_id_str), &marker);
    p = anedya_json_nstr(p, "bindingsecret", req_config->binding_secret, req_config->binding_secret_len, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);
    char topic[100];

    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/bindDevice/json");
    // Body is ready
    // printf("BODY : %s", txbuffer);
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

void _anedya_device_handle_generic_resp(anedya_client_t *client, anedya_txn_t *txn)
{
    // Parse JSON and check for error
    json_t mem[32];
    // Parse the json and get the txn id
    json_t const *json = json_create(txn->_rxbody, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        printf("Error while parsing for txn: %d\n", txn->desc);
        _anedya_interface_std_out("Error while parsing JSON body : Generic Response");
        return;
    }
    //printf("parsed: txn: %d", txn->desc);
    // Check if success
    json_t const *success = json_getProperty(json, "success");
    if (!success || JSON_BOOLEAN != json_getType(success))
    {
        _anedya_interface_std_out("Error, the success property is not found.");
    }
    bool s = json_getBoolean(success);
    if (s == true)
    {
        txn->is_success = true;
    }
    else
    {
        txn->is_success = false;
        json_t const *error = json_getProperty(json, "errCode");
        if (!error || JSON_INTEGER != json_getType(error))
        {
            _anedya_interface_std_out("Error, the error property is not found.");
        }
        int err = json_getInteger(error);
        txn->_op_err = err;
    }
    return;
}