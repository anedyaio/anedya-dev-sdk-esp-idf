#include "anedya_operations.h"
#include "anedya_op_commands.h"
#include "anedya_commons.h"

anedya_err_t _anedya_parse_inbound_command(char *payload, unsigned int payload_len, anedya_command_obj_t *obj)
{
    json_t mem[32];
    char temp[ANEDYA_RX_BUFFER_SIZE];
    strcpy(temp, payload);
    // Parse the json and get the txn id
    json_t const *json = json_create(temp, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body: Valuestore type");
    }
    json_t const *cmdID = json_getProperty(json, "commandId");
    if (!cmdID || JSON_TEXT != json_getType(cmdID))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *cmd_id = json_getValue(cmdID);
    anedya_err_t err = _anedya_uuid_parse(cmd_id, obj->cmdId);
    if (err != ANEDYA_OK)
    {
        return err;
    }

    json_t const *cmd = json_getProperty(json, "command");
    if (!cmd || JSON_TEXT != json_getType(cmd))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *cmd_val = json_getValue(cmd);
    strcpy(obj->command, cmd_val);
    obj->command_len = strlen(cmd_val);
    json_t const *dt = json_getProperty(json, "datatype");
    if (!dt || JSON_TEXT != json_getType(dt))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    const char *dt_val = json_getValue(dt);
    if (_anedya_strcmp(dt_val, "string") == 0)
    {
        // Copy data as it is
        json_t const *data = json_getProperty(json, "data");
        if (!data || JSON_TEXT != json_getType(data))
        {
            _anedya_interface_std_out("Error, the deploymentId property is not found.");
            return ANEDYA_ERR_PARSE_ERROR;
        }
        const char *data_val = json_getValue(data);
        strcpy(obj->data, data_val);
        obj->data_len = strlen(data_val);
        obj->cmd_data_type = ANEDYA_DATATYPE_STRING;
    }

    if (_anedya_strcmp(dt_val, "binary") == 0)
    {
        // Copy data as it is
        json_t const *data = json_getProperty(json, "data");
        if (!data || JSON_TEXT != json_getType(data))
        {
            _anedya_interface_std_out("Error, the deploymentId property is not found.");
            return ANEDYA_ERR_PARSE_ERROR;
        }
        const char *data_content = json_getValue(data);
        // Decode binary data from
        obj->data_len = _anedya_base64_decode((unsigned char *)data_content, obj->data);
        obj->cmd_data_type = (unsigned int)ANEDYA_DATATYPE_BINARY;
    }

    json_t const *exp = json_getProperty(json, "exp");
    if (!exp || JSON_INTEGER != json_getType(exp))
    {
        _anedya_interface_std_out("Error, the deploymentId property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }
    obj->exp = json_getInteger(exp);
    return ANEDYA_OK;
}

anedya_err_t anedya_op_cmd_status_update(anedya_client_t *client, anedya_txn_t *txn, anedya_req_cmd_status_update_t *req_config)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }
    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_CMD_UPDATE_STATUS;
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
    char uuid[37];
    _anedya_uuid_marshal(req_config->cmdId, uuid);
    p = anedya_json_str(p, "commandId", uuid, &marker);
    p = anedya_json_str(p, "status", req_config->status, &marker);
    if (req_config->data_len > 0 && req_config->data != NULL)
    {
        switch (req_config->data_type)
        {
        case ANEDYA_DATATYPE_STRING:
            p = anedya_json_str(p, "ackdata", (char *)req_config->data, &marker);
            p = anedya_json_str(p, "ackdatatype", "string", &marker);
            break;
        case ANEDYA_DATATYPE_BINARY:
            unsigned char encoded[1024];
            int encoded_len = _anedya_base64_encode(req_config->data, encoded);
            p = anedya_json_nstr(p, "ackdata", (char *)encoded, encoded_len, &marker);
            p = anedya_json_str(p, "ackdatatype", "binary", &marker);
            break;
        default:
            return ANEDYA_ERR_INVALID_DATATYPE;
            break;
        }
    }
    if (req_config->data_len != 0 && req_config->data == NULL) {
        return ANEDYA_ERR_INVALID_DATA;
    }
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);
    // Body is ready now publish it to the MQTT
    char topic[100];
    //printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/commands/updateStatus/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}