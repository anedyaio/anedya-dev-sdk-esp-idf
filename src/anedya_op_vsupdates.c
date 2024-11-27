#include "anedya_operations.h"
#include "anedya_op_vs.h"

uint8_t _anedya_parse_valuestore_type(char *payload, size_t payload_len)
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
    json_t const *type = json_getProperty(json, "type");
    if (!type || JSON_TEXT != json_getType(type))
    {
        _anedya_interface_std_out("Error, the type property is not found.");
    }
    const char *t = json_getValue(type);

    if (strcmp(t, "float") == 0)
    {
        return ANEDYA_VALUESTORE_TYPE_FLOAT;
    }

    if (strcmp(t, "string") == 0)
    {
        return ANEDYA_VALUESTORE_TYPE_STR;
    }

    if (strcmp(t, "binary") == 0)
    {
        return ANEDYA_VALUESTORE_TYPE_BIN;
    }

    if (strcmp(t, "boolean") == 0)
    {
        return ANEDYA_VALUESTORE_TYPE_BOOL;
    }
    return ANEDYA_VALUESTORE_TYPE_NONE;
}

anedya_err_t _anedya_parse_valuestore_float(char *payload, size_t payload_len, anedya_valustore_obj_float_t *obj)
{
    json_t mem[32];
    //printf("Buffer processed: %s\n", payload);
    // Parse the json and get the txn id
    json_t const *json = json_create(payload, mem, sizeof mem / sizeof *mem);
    if (!json)
    {
        _anedya_interface_std_out("Error while parsing JSON body: Valuestore float");
        return ANEDYA_ERR;
    }

    //
    // Parse Namespace
    //
    json_t const *ns = json_getProperty(json, "namespace");
    if (!ns || JSON_OBJ != json_getType(ns))
    {
        _anedya_interface_std_out("Error, the namespace property is not found.");
        return ANEDYA_ERR_PARSE_ERROR;
    }

    // Parse scope
    json_t const *scope = json_getProperty(ns, "scope");
    if (!scope || JSON_TEXT != json_getType(scope))
    {
        _anedya_interface_std_out("Error, the scope property is not found.");
    }
    const char *s = json_getValue(scope);

    if (strcmp(s, ANEDYA_SCOPE_SELF) == 0)
    {
        obj->ns.scope = ANEDYA_SCOPE_SELF;
    }
    // TODO: Do global scope handling

    // Parse Key
    json_t const *key = json_getProperty(json, "key");
    if (!key || JSON_TEXT != json_getType(key))
    {
        _anedya_interface_std_out("Error, the key property is not found.");
    }
    const char *k = json_getValue(key);
    strcpy(obj->key, k);

    // Parse Value
    json_t const *value = json_getProperty(json, "value");
    jsonType_t val_type = json_getType(value);
    //printf("Property: %d", val_type);
    if (!value || (JSON_REAL != val_type && JSON_INTEGER != val_type)) 
    {
        _anedya_interface_std_out("Error, the value property is not found.");
        return ANEDYA_ERR;
    }
    if(val_type == JSON_REAL)
    {
        obj->value = json_getReal(value);
    }
    if(val_type == JSON_INTEGER)
    {
        obj->value = (float)json_getInteger(value);
    }

    // Prase modified
    json_t const *modified = json_getProperty(json, "modified");
    if (!modified || JSON_INTEGER != json_getType(modified))
    {
        _anedya_interface_std_out("Error, the modified property is not found.");
    }
    obj->modified = json_getInteger(modified);
    return ANEDYA_OK;
}