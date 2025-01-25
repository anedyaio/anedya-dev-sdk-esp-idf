#include "anedya_operations.h"

anedya_err_t anedya_op_valuestore_list_obj(anedya_client_t *client, anedya_txn_t *txn, anedya_req_valuestore_list_obj_t obj)
{
    // First check if client is already connected or not
    if (client->is_connected == 0)
    {
        return ANEDYA_ERR_NOT_CONNECTED;
    }

    // If it is connected, then create a txn
    txn->_op = ANEDYA_OP_VALUESTORE_GET_LIST;
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
    strcat(topic, "/valuestore/scan/json");
    err = anedya_interface_mqtt_publish(client->mqtt_client, topic, strlen(topic), txbuffer, strlen(txbuffer), 0, 0);
    if (err != ANEDYA_OK)
    {
        return err;
    }
    return ANEDYA_OK;
}

void _anedya_op_valuestore_handle_list_obj_resp(anedya_client_t *client, anedya_txn_t *txn)
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
    anedya_op_valuestore_list_obj_resp_t *resp = (anedya_op_valuestore_list_obj_resp_t *)txn->response;

    json_t const *totalcount_prop = json_getProperty(json, "count");
    if (!totalcount_prop || JSON_INTEGER != json_getType(totalcount_prop))
    {
        _anedya_interface_std_out("Error, the totalcount property is not found.");
        return;
    }
    resp->totalcount = (int)json_getInteger(totalcount_prop);

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

    json_t const *list_prop = json_getProperty(json, "keys");
    if (!list_prop || JSON_ARRAY != json_getType(list_prop))
    {
        _anedya_interface_std_out("Error, the keys property is not found.");
        return;
    }

    json_t const *keys_prop = json_getChild(list_prop);
    int i = 0;
    while (keys_prop && i < resp->count)
    {
        if (JSON_OBJ != json_getType(keys_prop))
        {
            _anedya_interface_std_out("Error, the key property is not an object.");
            return;
        }

        json_t const *namespace_prop = json_getProperty(keys_prop, "namespace");
        if (!namespace_prop || JSON_OBJ != json_getType(namespace_prop))
        {
            _anedya_interface_std_out("Error, the namespace property is not found or is not text.");
            return;
        }
        json_t const *scope_prop = json_getProperty(namespace_prop, "scope");
        if (!scope_prop || JSON_TEXT != json_getType(scope_prop))
        {
            _anedya_interface_std_out("Error, the scope property is not found or is not text.");
            return;
        }
        resp->keys[i].ns.scope = (char *)json_getValue(scope_prop);

        json_t const *id_prop = json_getProperty(namespace_prop, "id");
        if (!id_prop || JSON_TEXT != json_getType(id_prop))
        {
            _anedya_interface_std_out("Error, the id property is not found or is not text.");
            const char *id = "";
            snprintf(resp->keys[i].ns.id, sizeof(resp->keys[i].ns.id), "%s", id);
        }
        else
        {
            const char *id = (const char *)json_getValue(id_prop);
            snprintf(resp->keys[i].ns.id, sizeof(resp->keys[i].ns.id), "%s", id);
        }

        json_t const *key_prop = json_getProperty(keys_prop, "key");
        if (!key_prop || JSON_TEXT != json_getType(key_prop))
        {
            _anedya_interface_std_out("Error, the key property is not found or is not text.");
            return;
        }

        const char *key = (const char *)json_getValue(key_prop);
        snprintf(resp->keys[i].key, sizeof(resp->keys[i].key), "%s", key);

        json_t const *type_prop = json_getProperty(keys_prop, "type");
        if (!type_prop || JSON_TEXT != json_getType(type_prop))
        {
            _anedya_interface_std_out("Error, the type property is not found or is not text.");
            return;
        }
        const char *type = (char *)json_getValue(type_prop);
        snprintf(resp->keys[i].type, sizeof(resp->keys[i].type), "%s", type);

        json_t const *modified_prop = json_getProperty(keys_prop, "modified");
        if (!modified_prop || JSON_INTEGER != json_getType(modified_prop))
        {
            _anedya_interface_std_out("Error, the modified property is not found or is not text.");
            return;
        }
        resp->keys[i].modified = (int64_t)json_getInteger(modified_prop);

        keys_prop = json_getSibling(keys_prop); // Move to the next sibling in the array
        i++;
    }

    return;
}
