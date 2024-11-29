#pragma once

#include "anedya_commons.h"

#define ANEDYA_CMD_STATUS_RECEIVED "received"
#define ANEDYA_CMD_STATUS_PROCESSING "processing"
#define ANEDYA_CMD_STATUS_SUCCESS "success"
#define ANEDYA_CMD_STATUS_FAILED "failure"

typedef const char * anedya_cmd_status_t;

typedef struct {
    anedya_uuid_t cmdId;
    char command[50];
    unsigned int command_len;
    char data[1000];
    unsigned int data_len;
    unsigned int cmd_data_type;
    unsigned long long exp;
} anedya_command_obj_t;

typedef struct {
    anedya_uuid_t cmdId;
    anedya_cmd_status_t status;
    unsigned char *data;
    unsigned int data_len;
    unsigned int data_type;
} anedya_req_cmd_status_update_t;

anedya_err_t _anedya_parse_inbound_command(char *payload, unsigned int payload_len, anedya_command_obj_t *obj);