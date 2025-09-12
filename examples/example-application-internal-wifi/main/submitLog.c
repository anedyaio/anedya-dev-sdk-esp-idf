#include "submitLog.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "time.h"

#include "sync.h"

static const char *TAG = "SUBMIT_LOG";
static TaskHandle_t current_task;

void submitLog_task(void *vParameters)
{
    uint32_t ulNotifiedValue;
    current_task = xTaskGetCurrentTaskHandle();

    while (1)
    {
        xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        anedya_txn_t lg_txn;
        anedya_txn_register_callback(&lg_txn, TXN_COMPLETE, &current_task);
        uint64_t timestamp_ms = (uint64_t)time(NULL) * 1000;
        printf("Time Stamp: %llu\n", timestamp_ms);
        char message[13] = "DeviceBooted";
        // =================================================== Submit Log to Anedya ==================================================
        anedya_err_t err = anedya_op_submit_log(&anedya_client, &lg_txn, message, strlen(message), timestamp_ms);
        // ===========================================================================================================================
        if (err != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (lg_txn.is_success && lg_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: '%s' Log sent\n", TAG, message);
                printf("--------------------------------\n");

                // Log sent to Anedya, Hold the task
                for (;;)
                {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to send Log to Anedya");
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to send Log to Anedya");
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}