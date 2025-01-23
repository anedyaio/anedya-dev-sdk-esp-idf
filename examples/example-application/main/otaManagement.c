#include "otaManagement.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static sync_data_t *sync_data;
static TaskHandle_t current_task;
static char asset_version[50]={0};

static esp_err_t do_firmware_update(anedya_op_next_ota_resp_t *resp);

#define UUID_STR_LEN 37
static void uuid_unparse(const anedya_uuid_t uu, char *out)
{
    snprintf(out, UUID_STR_LEN,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
             "%02x%02x-%02x%02x%02x%02x%02x%02x",
             uu[0], uu[1], uu[2], uu[3], uu[4], uu[5], uu[6], uu[7], uu[8], uu[9], uu[10], uu[11],
             uu[12], uu[13], uu[14], uu[15]);
}

void ota_management_task(void *pvParameters)
{

    ESP_LOGI("OTA", "Starting OTA Task");
    sync_data = (sync_data_t *)pvParameters;
    current_task = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;
    // Wait for device to connect to WiFi
    xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    printf("Waiting for client connection\n");
    while (!anedya_client.is_connected)
    {
        ESP_LOGI("OTA", "Waiting for client connection");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Now start the task
    ESP_LOGI("OTA", "Client check: %s", anedya_client.config->_device_id_str);
    for (;;)
    {
        while (!anedya_client.is_connected)
        {
            ESP_LOGI("OTA", "Waiting for client connection");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        // Check for update on Anedya
        ESP_LOGI("OTA", "Proceeding with the call!");
        anedya_txn_t ota_txn;
        anedya_asset_metadata_t meta[3];
        anedya_op_next_ota_resp_t resp;
        resp.asset.asset_metadata = meta;
        resp.asset.asset_metadata_len = 3;
        ota_txn.response = &resp;
        anedya_txn_register_callback(&ota_txn, TXN_COMPLETE, &current_task);
        anedya_err_t aerr = anedya_op_ota_next_req(&anedya_client, &ota_txn);
        if (aerr != ANEDYA_OK)
        {
            ESP_LOGI("OTA", "%s", anedya_err_to_name(aerr));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            ESP_LOGI("OTA", "TXN Complete");
        }
        else
        {
            ESP_LOGI("OTA", "TXN Timeout");
            // TODO: Handle error
        }
        ulNotifiedValue = 0x00;
        // OTA Txn completed.
        if (ota_txn.is_success)
        {
            // Success full transaction
            ESP_LOGI("OTA", "OTA Transaction Successful");
            if (resp.deployment_available)
            {
                char depID[37];
                char URL[1000];
                uuid_unparse(resp.deployment_id, depID);
                ESP_LOGI("OTA", "A deployment is available!");
                ESP_LOGI("OTA", "Deployment ID: %s", depID);
                ESP_LOGI("OTA", "Asset Identifier: %s", resp.asset.asset_identifier);
                ESP_LOGI("OTA", "Asset Version: %s", resp.asset.asset_version);
                ESP_LOGI("OTA", "Asset Size: %d", resp.asset.asset_size);
                ESP_LOGI("OTA", "Asset URL: %s", resp.asset.asset_url);

                for(int i = 0; i < resp.asset.asset_version_len; i++){
                    asset_version[i] = resp.asset.asset_version[i];
                }

                // TODO: Anedya update OTA status to start
                anedya_req_ota_update_status_t update_status = {
                    .deployment_id = resp.deployment_id,
                    .status = ANEDYA_OTA_STATUS_START,
                };
                anedya_txn_t update_txn;
                anedya_txn_register_callback(&update_txn, TXN_COMPLETE, &current_task);
                anedya_err_t uerr = anedya_op_ota_update_status_req(&anedya_client, &update_txn, &update_status);
                if (uerr != ANEDYA_OK)
                {
                    ESP_LOGI("OTA", "%s", anedya_err_to_name(uerr));
                }
                xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
                if (ulNotifiedValue == 0x01)
                {
                    ESP_LOGI("OTA", "TXN Complete");
                    if (update_txn.is_success)
                    {
                        anedya_req_ota_update_status_t conclude_status = {
                            .deployment_id = resp.deployment_id,
                        };
                        anedya_txn_t conclude_txn;
                        anedya_txn_register_callback(&conclude_txn, TXN_COMPLETE, &current_task);
                        // Start the firmware update
                        esp_err_t otaerr = do_firmware_update(&resp);
                        if (otaerr != ESP_OK)
                        {
                            conclude_status.status = ANEDYA_OTA_STATUS_FAILURE;
                        }
                        else
                        {
                            conclude_status.status = ANEDYA_OTA_STATUS_SUCCESS;
                        }
                        anedya_err_t cerr = anedya_op_ota_update_status_req(&anedya_client, &conclude_txn, &conclude_status);
                        if (cerr != ANEDYA_OK)
                        {
                            ESP_LOGI("OTA", "%s", anedya_err_to_name(cerr));
                        }
                        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
                        if (ulNotifiedValue == 0x01)
                        {
                            ESP_LOGI("OTA", "TXN Complete");
                        }
                        else
                        {
                            ESP_LOGI("OTA", "TXN Timeout");
                            // TODO: Handle error
                        }

                        if (otaerr == ESP_OK)
                        {
                            ESP_LOGI("OTA", "Firmware has been updated. Waiting for shutdown process to complete.");
                            // while(xEventGroupGetBits(DeviceEvents) & DEVICE_POWER_CYCLE) {
                            //  Device still processing the storage request. Wait for some time
                            // vTaskDelay(100/portTICK_PERIOD_MS);
                            //}
                            ESP_LOGI("OTA", "Shutdown Process complete.");
                            for (int i = 0; i < 5; i++)
                            {
                                ESP_LOGI("OTA", "Device will restart in: %d seconds", (5 - i));
                                vTaskDelay(1000 / portTICK_PERIOD_MS);
                            }
                            ESP_LOGI("OTA", "Restarting device!");
                            esp_restart();
                        }
                        else
                        {
                            ESP_ERROR_CHECK_WITHOUT_ABORT(otaerr);
                            esp_restart();
                        }
                    }
                }
                else
                {
                    ESP_LOGI("OTA", "TXN Timeout");
                    // TODO: Handle error
                }
            }
            else
            {
                ESP_LOGI("OTA", "No deployment available");
                // printf("Current Asset Version: %s\n", asset_version);
            }
        }
        vTaskDelay(11000 / portTICK_PERIOD_MS);
    }
}


static esp_err_t do_firmware_update(anedya_op_next_ota_resp_t *resp)
{
    int read_len = 0;
    ESP_LOGI("OTA", "Setting up device for new firmware update!");
    xEventGroupClearBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);
    // Wait for all tasks to complete a loop.
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    // Carry out OTA update
    esp_http_client_config_t config = {
        .url = resp->asset.asset_url,
        .crt_bundle_attach = esp_crt_bundle_attach
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (https_ota_handle == NULL)
    {
        return ESP_FAIL;
    }
    // Get the total image size in bytes.
    // int tsize = esp_https_ota_get_image_size(https_ota_handle);

    // Start online update
    ESP_LOGI("OTA", "Performing firmware update");
    while (1)
    {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            if (err == ESP_OK)
            {
                break;
            }
            else
            {
                return err;
            }
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
        // Code reached here that means OTA is still in progress.
    }
    read_len = esp_https_ota_get_image_len_read(https_ota_handle);
    ESP_LOGI("OTA", "New firmware downloaded: %.1f Kb downloaded", (float)read_len / 1024.0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(err);
    // Check if everything is ok.
    if (err != ESP_OK)
    {
        // There is some error. Abort the firmware upgrade.
        esp_https_ota_abort(https_ota_handle);
        return err;
    }
    if (!esp_https_ota_is_complete_data_received(https_ota_handle))
    {
        // Complete data has not beeen recieved. There can be multiple issue.
        // Abort the firmware upgrade.
        esp_https_ota_abort(https_ota_handle);
        return ESP_FAIL;
    }
    // Everything is ok. Finish the update.
    esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
    if (ota_finish_err != ESP_OK)
    {
        return ota_finish_err;
    }
    return ESP_OK;
}
