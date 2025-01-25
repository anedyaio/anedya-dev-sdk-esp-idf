#include "anedya_operations.h"

anedya_err_t anedya_op_cmd_list_obj_to_anedya(anedya_client_t *client, anedya_txn_t *txn, anedya_req_cmd_list_obj_t obj)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_CMD_LIST_OBJ;
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
    p = anedya_json_int(p, "limit", obj.limit, &marker);
    p = anedya_json_int(p, "offset", obj.offset, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    // printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/commands/list/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

void _anedya_op_command_handle_list_obj_resp(anedya_client_t *client, anedya_txn_t *txn)
{

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
    anedya_op_cmd_list_obj_resp_t *resp = (anedya_op_cmd_list_obj_resp_t *)txn->response;

    json_t const *totalcount_prop = json_getProperty(json, "totalcount");
    if (!totalcount_prop || JSON_INTEGER != json_getType(totalcount_prop))
    {
        _anedya_interface_std_out("Error, the totalcount property is not found.");
    }
    else
    {
        resp->totalcount = (int)json_getInteger(totalcount_prop);
    }

    json_t const *count_prop = json_getProperty(json, "count");
    if (!count_prop || JSON_INTEGER != json_getType(count_prop))
    {
        _anedya_interface_std_out("Error, the count property is not found.");
        return;
    }
    resp->count = (int)json_getInteger(count_prop);

    json_t const *next_prop = json_getProperty(json, "next");
    if (!next_prop || JSON_INTEGER != json_getType(next_prop))
    {
        _anedya_interface_std_out("Error, the next property is not found.");
        return;
    }
    resp->next = (int)json_getInteger(next_prop);
    // printf("Next: %d\n", resp->next);

    json_t const *list_prop = json_getProperty(json, "commands");
    if (!list_prop || JSON_ARRAY != json_getType(list_prop))
    {
        _anedya_interface_std_out("Error, the keys property is not found.");
        return;
    }

    json_t const *cmds_prop = json_getChild(list_prop);
    int i = 0;
    while (cmds_prop && i < resp->count)
    {
        if (JSON_OBJ != json_getType(cmds_prop))
        {
            _anedya_interface_std_out("Error, the key property is not an object.");
            return;
        }

        json_t const *command_id_prop = json_getProperty(cmds_prop, "commandId");
        if (!command_id_prop || JSON_TEXT != json_getType(command_id_prop))
        {
            _anedya_interface_std_out("Error, the key property is not found or is not text.");
            return;
        }

        const char *cmd_id = json_getValue(command_id_prop);
        anedya_err_t err = _anedya_uuid_parse(cmd_id, resp->commands[i].cmdId);
        if (err != ANEDYA_OK)
        {
            _anedya_interface_std_out("Error, failed to parse the command id.");
            return;
        }

        json_t const *cmd_prop = json_getProperty(cmds_prop, "command");
        if (!cmd_prop || JSON_TEXT != json_getType(cmd_prop))
        {
            _anedya_interface_std_out("Error, the type property is not found or is not text.");
            return;
        }
        const char *cmd = (char *)json_getValue(cmd_prop);
        snprintf(resp->commands[i].command, sizeof(resp->commands[i].command), "%s", cmd);

        json_t const *status_prop = json_getProperty(cmds_prop, "status");
        if (!status_prop || JSON_TEXT != json_getType(status_prop))
        {
            _anedya_interface_std_out("Error, the type property is not found or is not text.");
            return;
        }
        resp->commands[i].status = (char *)json_getValue(status_prop);

        json_t const *issued_at_prop = json_getProperty(cmds_prop, "issuedAt");
        if (!issued_at_prop || JSON_INTEGER != json_getType(issued_at_prop))
        {
            _anedya_interface_std_out("Error, the modified property is not found or is not text.");
            return;
        }
        resp->commands[i].issued_at = (unsigned long long)json_getInteger(issued_at_prop);

        json_t const *updated_prop = json_getProperty(cmds_prop, "updated");
        if (!updated_prop || JSON_INTEGER != json_getType(updated_prop))
        {
            _anedya_interface_std_out("Error, the modified property is not found or is not text.");
            return;
        }
        resp->commands[i].updated = (unsigned long long)json_getInteger(updated_prop);

        cmds_prop = json_getSibling(cmds_prop); // Move to the next sibling in the array
        i++;
    }

    return;
}
