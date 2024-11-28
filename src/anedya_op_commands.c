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
        obj->data_len = _anedya_base64_decode((unsigned char*)data_content, obj->data);
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