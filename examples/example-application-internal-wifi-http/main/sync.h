
#ifndef _SYNC_H_
#define _SYNC_H_

#include "anedya.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

typedef struct {
  EventGroupHandle_t ConnectionEvents;
  EventGroupHandle_t DeviceEvents;
  EventGroupHandle_t DeviceTimeEvents;
  EventGroupHandle_t COMMANDEVENTS;
  anedya_uuid_t device_uuid;
  bool device_bound;
  char ID[15];
} sync_data_t;

extern sync_data_t gatewaystate;
extern anedya_client_t anedya_client;
extern anedya_command_obj_t *command_obj;

//============================================================================
// Runtime EventGroups
//============================================================================
extern EventGroupHandle_t ConnectionEvents;
extern EventGroupHandle_t OtaEvents;

// Connection event bits (used by both WiFi and Quectel paths)
#define WIFI_FAIL_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1
#define MQTT_CONNECTED_BIT BIT2
#define ANEDYA_CONNECTED_BIT BIT3

#define OTA_NOT_IN_PROGRESS_BIT BIT4
#define SYNCED_DEVICE_TIME_BIT BIT5
#define COMMAND_AVAILABLE_BIT BIT6

void TXN_COMPLETE(anedya_txn_t *txn, anedya_context_t ctx);

#endif /* _SYNC_H_ */