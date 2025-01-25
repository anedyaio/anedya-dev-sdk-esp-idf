
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

        //============================ Set String Value ================================
        // For more info visit: https://docs.anedya.io/valuestore
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
                printf("%s: Key:%s, Value: %s  Key Value Set\n", TAG, strKey, strValue);
                printf("--------------------------------\n");
            }
        }
        else
        {
            // ESP_LOGI("CLIENT", "TXN Timeout");
            ESP_LOGE(TAG, "Failed to set Key Value to Anedya");
        }

        // ======================= Get String Value ================================
        anedya_req_valuestore_get_key_t req_str_key = {
            .key = "STR_KEY",
            .ns = {
                .scope = ANEDYA_SCOPE_SELF,
            }};

        anedya_valuestore_obj_string_t resp;
        vs_txn.response = &resp;

        v_err = anedya_op_valuestore_get_key(&anedya_client, &vs_txn, req_str_key);

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
                printf("%s: Got Key:%s, Value: %s \n", TAG, resp.key, resp.value);
                printf("--------------------------------\n");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to get key value from Anedya");
            }
        }

        // ====================== Get VS Obj List ================================
        anedya_req_valuestore_list_obj_t req_key_list = {
            .limit = 2,
            .offset = 0,
        };
        anedya_op_valuestore_list_obj_resp_t resp_obj_list;
        anedya_valuestore_obj_key_t resp_keys[req_key_list.limit];
        resp_obj_list.keys = resp_keys;
        vs_txn.response = &resp_obj_list;

        v_err = anedya_op_valuestore_list_obj(&anedya_client, &vs_txn, req_key_list);

        if (v_err != ANEDYA_OK)
        {
            ESP_LOGE("CLIENT", "%s", anedya_err_to_name(v_err));
        }

        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            if (vs_txn.is_success && vs_txn.is_complete)
            {
                printf("------------------------------------------------------------\n");
                printf("%s: Total Keys: %d\n", TAG, resp_obj_list.totalcount);
                printf("%s: Count %d\n", TAG, resp_obj_list.count);
                printf("%s: Next %d\n", TAG, resp_obj_list.next);
                for (int i = 0; i < resp_obj_list.count; i++)
                {
                    printf( "%s: Namespace: scope: %s, Id: %s\n", TAG, resp_obj_list.keys[i].ns.scope, resp_obj_list.keys[i].ns.id);
                    printf("%s: Key :%s, Type: %s, Modified: %lld\n", TAG, resp_obj_list.keys[i].key, resp_obj_list.keys[i].type, resp_obj_list.keys[i].modified); 
                }
                printf("-------------------------------------------------------------\n");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to get key list from Anedya");
            }
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
}
