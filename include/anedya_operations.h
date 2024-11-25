#pragma once

#ifndef _ANEDYA_OPERATIONS_H_
#define _ANEDYA_OPERATIONS_H_

#include <stdio.h>
#include "anedya.h"
#include "anedya_json_builder.h"
#include "anedya_json_parse.h"
#include "anedya_ota.h"
#include "anedya_op_vs.h"
#include "anedya_op_submitevent.h"
#include "string.h"

// Anedya Operation codes

#define ANEDYA_OP_BIND_DEVICE 1
#define ANEDYA_OP_HEARTBEAT 2
#define ANEDYA_OP_OTA_NEXT 3
#define ANEDYA_OP_SUBMIT_DATA 4
#define ANEDYA_OP_OTA_UPDATE_STATUS 5
#define ANEDYA_OP_VALUESTORE_SET 6
#define ANEDYA_OP_SUBMIT_EVENT 7

// Anedya Events
#define ANEDYA_EVENT_VS_UPDATE_FLOAT 1
#define ANEDYA_EVENT_VS_UPDATE_BOOL 2

// Anedya Operations Error Codes
#define ANEDYA_OP_ERR_RESP_BUFFER_OVERFLOW -1

typedef struct
{
    bool success;
    char *error_msg;
    size_t error;
} anedya_generic_resp_t;

// Anedya Operations and requests
anedya_err_t anedya_device_bind_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_bind_device_t *req_config);
anedya_err_t anedya_device_send_heartbeat(anedya_client_t *client, anedya_txn_t *txn);
anedya_err_t anedya_op_ota_next_req(anedya_client_t *client, anedya_txn_t *txn);
anedya_err_t anedya_op_ota_update_status_req(anedya_client_t *client, anedya_txn_t *txn, anedya_req_ota_update_status_t *req);
anedya_err_t anedya_op_submit_float_req(anedya_client_t *client, anedya_txn_t *txn, const char *variable_identifier, float value, uint64_t timestamp_ms);
anedya_err_t anedya_op_valuestore_set_float(anedya_client_t *client, anedya_txn_t *txn, const char *key, float value);
anedya_err_t anedya_op_valuestore_set_bool(anedya_client_t *client, anedya_txn_t *txn, const char *key, bool value);
anedya_err_t anedya_op_submit_event(anedya_client_t *client, anedya_txn_t *txn, anedya_req_submit_event_t *req_config);

// Reponse handlers
void _anedya_device_handle_generic_resp(anedya_client_t *client, anedya_txn_t *txn);
void _anedya_op_ota_next_resp(anedya_client_t *client, anedya_txn_t *txn);

#endif