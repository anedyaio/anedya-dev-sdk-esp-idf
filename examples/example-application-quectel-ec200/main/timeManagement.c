#include "timeManagement.h"
#include "sync.h"
#include "esp_log.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"

static sync_data_t *sync_data;
static TaskHandle_t current_task;

static const char *TAG = "TimeManagement";

// Task to monitor and adjust time as required.
void syncTime_task(void *pvParameters)
{
    // Device just booted up. System time is out of sync.
    sync_data = (sync_data_t *)pvParameters;
    current_task = xTaskGetCurrentTaskHandle();
    // bool device_tos = true;
    // int seconds_remain = 0;
    xEventGroupClearBits( sync_data->DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT);
    
    ESP_LOGI(TAG, "Starting the time sync task.");
    char strftime_buf[60];

    // Wait for modem to be ready
    while (1)
    {
        anedya_err_t err = anedya_ext_connectivity_check(&anedya_client, 5000);
        if (err == ANEDYA_OK)
            break;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Main loop
    while (1)
    {

        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        anedya_ext_get_modem_time(&anedya_client, 1, strftime_buf);
        ESP_LOGI(TAG, "Fetched Modem Time: %s", strftime_buf);

        struct tm tm;
        memset(&tm, 0, sizeof(struct tm));

        // Parse format: "2025/05/16,09:29:00+22"
        // Ignore the +22 part
        if (sscanf(strftime_buf, "%d/%d/%d,%d:%d:%d",
                   &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                   &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6)
        {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;

            // Abstract the time to GMT
            time_t t = mktime(&tm);
            if (t != -1)
            {
                // Set the system time to GMT
                struct timeval tv = {
                    .tv_sec = t,
                    .tv_usec = 0,
                };
                // printf("Epoch time: %lld\n", tv.tv_sec);
                settimeofday(&tv, NULL);
                xEventGroupSetBits(sync_data->DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT);
                ESP_LOGI(TAG,"-----------------------------------");
                ESP_LOGI(TAG, "System time set to GMT from modem.");
                ESP_LOGI(TAG,"-----------------------------------");
            }
            else
            {
                ESP_LOGE("TIMEKEEPER", "Failed to convert modem time to epoch.");
            }
        }
        else
        {
            ESP_LOGE("TIMEKEEPER", "Failed to parse modem time string: %s", strftime_buf);
        }

        vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);  // Sync time every 60 seconds
    }
}


