#include "anedya_operations.h"




anedya_err_t anedya_op_valuestore_get_key(anedya_client_t *client, anedya_txn_t *txn, anedya_valuestore_get_key_t obj)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_VALUESTORE_GET;
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
    p = anedya_json_objOpen(p, "namespace", &marker);
    p = anedya_json_str(p, "scope", obj.ns.scope, &marker);
    p = anedya_json_str(p, "id", obj.ns.id, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_str(p, "key", obj.key, &marker);
    p = anedya_json_objClose(p, &marker);
    p = anedya_json_end(p, &marker);

    // Body is ready now publish it to the MQTT
    char topic[100];
    // printf("Req: %s", txbuffer);
    strcpy(topic, "$anedya/device/");
    strcat(topic, client->config->_device_id_str);
    strcat(topic, "/valuestore/getValue/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}


void _anedya_op_valuestore_get_resp(anedya_client_t *client, anedya_txn_t *txn)
{
    // Parse JSON and check for error
    anedya_op_get_valuestore_resp_t *resp = (anedya_op_get_valuestore_resp_t *)txn->response;
    json_t mem[32];
    // Parse the json and get the txn id
    json_t const *json = json_create(txn->_rxbody, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body:response handler OTA Next");
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
        json_t const *error = json_getProperty(json, "errCode");
        if (!error || JSON_INTEGER != json_getType(error))
        {
            _anedya_interface_std_out("Error, the error property is not found.");
        }
        int err = json_getInteger(error);
        txn->_op_err = err;
        return;
    }
    // Flow reaches here means, request was successful.
    json_t const *namespace = json_getProperty(json, "namespace");
    if (!namespace || JSON_OBJ != json_getType(namespace))
    {
        _anedya_interface_std_out("Error, the namespace property is not found.");
        return;
    }
    json_t const *scope = json_getProperty(namespace, "scope");
    if (!scope || JSON_TEXT != json_getType(scope))
    {
        _anedya_interface_std_out("Error, the scope property is not found.");
        return;
    }
    resp->scope = json_getValue(scope);

    json_t const *key = json_getProperty(json, "key");
    if (!key || JSON_TEXT != json_getType(key))
    {
        _anedya_interface_std_out("Error, the key property is not found.");
        return;
    }
    resp->key = json_getValue(key);

    json_t const *type = json_getProperty(json, "type");
    if (!type || JSON_TEXT != json_getType(type))
    {
        _anedya_interface_std_out("Error, the type property is not found.");
        return;
    }
    resp->type = json_getValue(type);

    if (strcmp(resp->type, "string") == 0){
        json_t const *value = json_getProperty(json, "value");
        if (!value || JSON_TEXT != json_getType(value))
        {
            _anedya_interface_std_out("Error, the value property is not found.");
            return;
        }
        resp->value_str = json_getValue(value);
        resp->str_value_len = strlen(resp->value_str);
    }else if (strcmp(resp->type, "float") == 0){
        json_t const *value = json_getProperty(json, "value");
        if (!value || JSON_INTEGER != json_getType(value))
        {
            _anedya_interface_std_out("Error, the value property is not found.");
            return;
        }
        resp->value_float = json_getInteger(value);
    }else if (strcmp(resp->type, "boolean") == 0){
        json_t const *value = json_getProperty(json, "value");
        if (!value || JSON_BOOLEAN != json_getType(value))
        {
            _anedya_interface_std_out("Error, the value property is not found.");
            return;
        }
        resp->value_bool = json_getBoolean(value);
    }else if (strcmp(resp->type, "binary") == 0){
        json_t const *value = json_getProperty(json, "value");
        if (!value || JSON_TEXT != json_getType(value))
        {
            _anedya_interface_std_out("Error, the value property is not found.");
            return;
        }
        resp->value_bin = json_getValue(value);
        resp->bin_value_len = strlen(resp->value_bin);
    }else{
        _anedya_interface_std_out("Error, the type property is not found.");
        return;
    }
    json_t const *modified= json_getProperty(json, "modified");
    if (!modified || JSON_INTEGER != json_getType(modified))
    {
        _anedya_interface_std_out("Error, the modified property is not found.");
        return;
    }
    resp->modified = json_getInteger(modified);
    return;
}
