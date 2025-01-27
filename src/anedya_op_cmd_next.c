#include "anedya_operations.h"

anedya_err_t anedya_op_cmd_next(anedya_client_t *client, anedya_txn_t *txn)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_CMD_NEXT;
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
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    // printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/commands/next/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

void _anedya_op_cmd_handle_next_resp(anedya_client_t *client, anedya_txn_t *txn)
{

    anedya_op_cmd_next_resp_t *resp = (anedya_op_cmd_next_resp_t *)txn->response;
    // printf("Resp: %s\n", txn->_rxbody);
    json_t mem[32];
    // Parse the json and get the txn id
    json_t const *json = json_create(txn->_rxbody, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body:response handler Get list value store");
        return;
    }
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
        json_t const *error = json_getProperty(json, "errorcode");
        if (!error || JSON_INTEGER != json_getType(error))
        {
            _anedya_interface_std_out("Error, the error property is not found.");
        }
        int err = (anedya_err_t)json_getInteger(error);
        txn->_op_err = err;
        return;
    }

    // Flow reaches here means, request was successful.
    json_t const *available = json_getProperty(json, "available");
    if (!available || JSON_BOOLEAN != json_getType(available))
    {
        _anedya_interface_std_out("Error, the available property is not found.");
    }
    resp->available = json_getBoolean(available);
    if (!resp->available)
    {
        printf("No more commands available\n");
        return;
    }

    json_t const *command_id_prop = json_getProperty(json, "commandId");
    if (!command_id_prop || JSON_TEXT != json_getType(command_id_prop))
    {
        _anedya_interface_std_out("Error, the key property is not found or is not text.");
        return;
    }

    const char *cmd_id = json_getValue(command_id_prop);
    anedya_err_t err = _anedya_uuid_parse(cmd_id, resp->cmdId);
    if (err != ANEDYA_OK)
    {
        _anedya_interface_std_out("Error, failed to parse the command id.");
        // return;
    }

    json_t const *cmd_prop = json_getProperty(json, "command");
    if (!cmd_prop || JSON_TEXT != json_getType(cmd_prop))
    {
        _anedya_interface_std_out("Error, the type property is not found or is not text.");
        return;
    }
    const char *cmd = json_getValue(cmd_prop);
    snprintf(resp->command, sizeof(resp->command), "%s", cmd);
    resp->command_len = strlen(cmd);

    json_t const *status_prop = json_getProperty(json, "status");
    if (!status_prop || JSON_TEXT != json_getType(status_prop))
    {
        _anedya_interface_std_out("Error, the status property is not found or is not text.");
        return;
    }
    const char *status = json_getValue(status_prop);
    snprintf(resp->status, sizeof(resp->status), "%s", status);

    json_t const *data_prop = json_getProperty(json, "data");
    if (!data_prop || JSON_TEXT != json_getType(data_prop))
    {
        _anedya_interface_std_out("Error, the data property is not found or is not text.");
        return;
    }
    const char *data = json_getValue(data_prop);
    if (data)
    {
        resp->data = (uint8_t *)data;
        resp->data_len = strlen(data);

    }

    json_t const *datatype_prop = json_getProperty(json, "datatype");
    if (!datatype_prop || JSON_TEXT != json_getType(datatype_prop))
    {
        _anedya_interface_std_out("Error, the datatype property is not found or is not text.");
        return;
    }
    const char *datatype = (char *)json_getValue(datatype_prop);
    if (datatype)
    {
        if (strcmp(datatype, "string") == 0)
        {
            resp->data_type = ANEDYA_DATATYPE_STRING;
        }
        else if (strcmp(datatype, "binary") == 0)
        {
            resp->data_type = ANEDYA_DATATYPE_BINARY;
        }
        else
        {
            resp->data_type = ANEDYA_DATATYPE_UNKNOWN;
        }
    }

    json_t const *issued_at_prop = json_getProperty(json, "issuedAt");
    if (!issued_at_prop || JSON_INTEGER != json_getType(issued_at_prop))
    {
        _anedya_interface_std_out("Error, the issued at property is not found or is not text.");
        return;
    }
    resp->issued_at = (unsigned long long)json_getInteger(issued_at_prop);

    json_t const *updated_prop = json_getProperty(json, "updated");
    if (!updated_prop || JSON_INTEGER != json_getType(updated_prop))
    {
        _anedya_interface_std_out("Error, the updated property is not found or is not text.");
        return;
    }
    resp->updated = (unsigned long long)json_getInteger(updated_prop);

    json_t const *nextavailable_prop = json_getProperty(json, "nextavailable");
    if (!nextavailable_prop || JSON_BOOLEAN != json_getType(nextavailable_prop))
    {
        _anedya_interface_std_out("Error, the next available property is not found or is not text.");
        return;
    }
    resp->nextavailable = json_getBoolean(nextavailable_prop);

    return;
}
