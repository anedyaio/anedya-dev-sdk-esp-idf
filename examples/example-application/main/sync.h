
#ifndef _SYNC_H_
#define _SYNC_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "uuid.h"
#include "anedya.h"

typedef struct {
    EventGroupHandle_t ConnectionEvents;
    EventGroupHandle_t DeviceEvents;
    uuid_t device_uuid;
    bool device_bound;
    char ID[15];
} sync_data_t;

extern sync_data_t gatewaystate;
extern anedya_client_t anedya_client;

//============================================================================
// Runtime EventGroups
//============================================================================
extern EventGroupHandle_t ConnectionEvents; // Declaration
extern EventGroupHandle_t OtaEvents;

#define WIFI_FAIL_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define CONNECTION_FAIL BIT2
#define CONNECTION_PUSH_FAIL BIT3
#define MQTT_CONNECTED BIT5
#define OTA_NOT_IN_PROGRESS BIT6

void TXN_COMPLETE(anedya_txn_t *txn, anedya_context_t ctx);

#endif