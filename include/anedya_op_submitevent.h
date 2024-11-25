#pragma once

#include <stdint.h>

typedef struct {
    char key[50];
    char value[50];
} anedya_event_data_t;

typedef struct {
    const char event_type[50];
    long long timestamp;
    anedya_event_data_t *data;
    unsigned int data_count;
} anedya_req_submit_event_t;