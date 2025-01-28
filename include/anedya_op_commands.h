#pragma once

#include "anedya_commons.h"

#define ANEDYA_CMD_STATUS_RECEIVED "received"
#define ANEDYA_CMD_STATUS_PROCESSING "processing"
#define ANEDYA_CMD_STATUS_SUCCESS "success"
#define ANEDYA_CMD_STATUS_FAILED "failure"
#define ANEDYA_CMD_STATUS_PENDING "pending"

typedef const char *anedya_cmd_status_t;

typedef struct {
    anedya_uuid_t cmdId;
    char command[50];
    unsigned int command_len;
    char data[1000];
    unsigned int data_len;
    unsigned int cmd_data_type;
    unsigned long long exp;
} anedya_command_obj_t;

typedef struct
{
    anedya_uuid_t cmdId;
    anedya_cmd_status_t status;
    unsigned char *data;
    unsigned int data_len;
    unsigned int data_type;
} anedya_req_cmd_status_update_t;

anedya_err_t _anedya_parse_inbound_command(char *payload, unsigned int payload_len, anedya_command_obj_t *obj);

// ============= Command List ================
typedef struct{
    unsigned short limit;
    unsigned short offset;
} anedya_req_cmd_list_obj_t;

typedef struct
{
    anedya_uuid_t cmdId;
    char command[50];
    unsigned int command_len;
    char *status;
    unsigned long long issued_at;
    unsigned long long updated;
} anedya_command_queued_obj_t;
typedef struct
{
    anedya_command_queued_obj_t *commands;
    unsigned short int totalcount;
    unsigned short int count;
    unsigned short int next;
} anedya_op_cmd_queued_obj_resp_t;

void _anedya_op_command_handle_queued_obj_resp(anedya_client_t *client, anedya_txn_t *txn);

// ============== Next Command ==================
typedef struct
{
    bool available;
    anedya_uuid_t cmdId;
    char command[50];
    unsigned int command_len;
    char status[20];
    unsigned char *data;
    unsigned int data_len;
    unsigned int data_type;
    unsigned long long issued_at;
    unsigned long long updated;
    bool nextavailable;
} anedya_op_cmd_next_resp_t;

void _anedya_op_cmd_handle_next_resp(anedya_client_t *client, anedya_txn_t *txn);