#pragma once

#include "anedya_commons.h"

typedef struct {
    anedya_uuid_t cmdId;
    char command[50];
    unsigned int command_len;
    char data[1000];
    unsigned int data_len;
    unsigned int cmd_data_type;
    unsigned long long exp;
} anedya_command_obj_t;

anedya_err_t _anedya_parse_inbound_command(char *payload, unsigned int payload_len, anedya_command_obj_t *obj);