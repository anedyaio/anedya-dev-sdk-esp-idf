
#include "valueStore.h"
#include "sync.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"

const char *TAG = "VALUE_STORE";
static TaskHandle_t current_task;

void valueStore_task(void *pvParameters)
{
    current_task = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;
    while (1)
    {
        xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        anedya_txn_t vs_txn;
        anedya_err_t v_err;
        anedya_txn_register_callback(&vs_txn, TXN_COMPLETE, &current_task);
        // ======================= Set float Value ================================
        // For more info visit: https://docs.anedya.io/valuestore

        const char *key = "FLOAT_KEY";
        float value = 1.00;

        v_err = anedya_op_valuestore_set_float(&anedya_client, &vs_txn, key, value);
        if (v_err != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%f'  Key Value Set\n", TAG, key, value);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        //============================= Set bool Value ================================
        const char *boolKey = "BOOL_KEY";
        bool boolValue = true;

        v_err = anedya_op_valuestore_set_bool(&anedya_client, &vs_txn, boolKey, boolValue);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGI("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%d'  Key Value Set\n", TAG, boolKey, boolValue);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        //============================ Set String Value ================================
        const char *strKey = "STR_KEY";
        const char *strValue = "OK";
        size_t strValueLen = strlen(strValue);
        v_err = anedya_op_valuestore_set_string(&anedya_client, &vs_txn, strKey, strValue, strValueLen);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGE("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%s'  Key Value Set\n", TAG, strKey, strValue);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        // ======================= Set Binary Value ================================
        const char *binKey = "BIN_KEY";
        const char *binValue = "aGVsbG8=";

        size_t binValueLen = strlen(binValue);

        v_err = anedya_op_valuestore_set_bin(&anedya_client, &vs_txn, binKey, binValue, binValueLen);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGE("CLIENT", "%s", anedya_err_to_name(v_err));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Key:'%s', Value: '%s'  Key Value Set\n", TAG, binKey, binValue);
                printf("--------------------------------\n");
            }
            else{
                ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        // ======================= Get String Value ================================
        anedya_valuestore_get_key_t vs_get_key;
        vs_get_key.key = "STR_KEY";
        vs_get_key.ns.scope = ANEDYA_SCOPE_SELF;

        anedya_op_get_valuestore_resp_t resp;
        vs_txn.response = &resp;
        v_err = anedya_op_valuestore_get_key(&anedya_client, &vs_txn, vs_get_key);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGE("CLIENT", "%s", anedya_err_to_name(v_err));
        }

        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("--------------------------------\n");
                printf("%s: Got Key:'%s', Type: '%s' \n", TAG, vs_get_key.key, resp.value_str);
                printf("--------------------------------\n");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to get key value from Anedya");
            }
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}
