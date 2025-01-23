#pragma once

#include "stdint.h"
#include "stddef.h"

#define ANEDYA_SCOPE_GLOBAL "global"
#define ANEDYA_SCOPE_SELF "self"

#define ANEDYA_VALUESTORE_TYPE_NONE 0
#define ANEDYA_VALUESTORE_TYPE_FLOAT 1
#define ANEDYA_VALUESTORE_TYPE_BOOL 2
#define ANEDYA_VALUESTORE_TYPE_STRING 3
#define ANEDYA_VALUESTORE_TYPE_BIN 4

typedef struct
{
    char *scope;
    char id[50];
} anedya_valuestore_ns_t;

typedef struct
{
    anedya_valuestore_ns_t ns;
    char key[50];
    float value;
    int64_t modified;
} anedya_valuestore_obj_float_t;

typedef struct
{
    anedya_valuestore_ns_t ns;
    const char key[50];
    bool value;
    int64_t modified;
} anedya_valuestore_obj_bool_t;

typedef struct
{
    anedya_valuestore_ns_t ns;
    const char key[50];
    char *value;
    size_t value_len;
    int64_t modified;
} anedya_valuestore_obj_string_t;

typedef struct
{
    anedya_valuestore_ns_t ns;
    const char key[50];
    char *value;
    size_t value_len;
    int64_t modified;
} anedya_valuestore_obj_bin_t;

uint8_t _anedya_parse_valuestore_type(char *payload, size_t payload_len);
anedya_err_t _anedya_parse_valuestore_float(char *payload, size_t payload_len, anedya_valuestore_obj_float_t *obj);
anedya_err_t _anedya_parse_valuestore_string(char *payload, size_t payload_len, anedya_valuestore_obj_string_t *obj);
anedya_err_t _anedya_parse_valuestore_bool(char *payload, size_t payload_len, anedya_valuestore_obj_bool_t *obj);
anedya_err_t _anedya_parse_valuestore_bin(char *payload, size_t payload_len, anedya_valuestore_obj_bin_t *obj);

typedef struct
{
    anedya_valuestore_ns_t ns;
    char *key;
} anedya_req_valuestore_get_key_t;

void _anedya_op_valuestore_get_resp(anedya_client_t *client, anedya_txn_t *txn);
