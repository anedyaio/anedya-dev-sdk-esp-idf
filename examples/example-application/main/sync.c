#include "sync.h"
#include "anedya.h"
#include "esp_log.h"

EventGroupHandle_t ConnectionEvents; // Definition
EventGroupHandle_t OtaEvents;


void TXN_COMPLETE(anedya_txn_t *txn, anedya_context_t ctx)
{
    //ESP_LOGI("CLIENT", "On TXN handler");
    TaskHandle_t *handle = (TaskHandle_t *)ctx;
    xTaskNotify(*handle, 0x01, eSetValueWithOverwrite);
}