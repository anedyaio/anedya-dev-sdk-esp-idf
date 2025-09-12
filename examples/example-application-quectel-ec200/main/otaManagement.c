#include "otaManagement.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "anedya.h"

static sync_data_t *sync_data;
static TaskHandle_t current_task;

static const char *TAG = "OTA";

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

static void EscapeQuestionMarks(char *input, char *output);

void ota_management_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting OTA Task");
    sync_data = (sync_data_t *)pvParameters;
    current_task = xTaskGetCurrentTaskHandle();
    uint32_t ulNotifiedValue;
    xEventGroupSetBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);
    while (!anedya_client.is_connected)
    {
        // ESP_LOGI(TAG, "Waiting for client connection");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Now start the task
    ESP_LOGI(TAG, "Client check: %s", anedya_client.config->_device_id_str);
    for (;;)
    {
        while (!anedya_client.is_connected)
        {
            // ESP_LOGI(TAG, "Waiting for client connection");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        // Check for update on Anedya
        ESP_LOGI(TAG, "Proceeding with the call!");
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
            ESP_LOGI(TAG, "%s", anedya_err_to_name(aerr));
        }
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
        if (ulNotifiedValue == 0x01)
        {
            ESP_LOGI(TAG, "TXN Complete");
        }
        else
        {
            ESP_LOGI(TAG, "TXN Timeout");
            // TODO: Handle error
        }
        ulNotifiedValue = 0x00;
        // OTA Txn completed.
        if (ota_txn.is_success)
        {
            // Success full transaction
            ESP_LOGI(TAG, "OTA Transaction Successful");
            if (resp.deployment_available)
            {
                char depID[37];
                char URL[1000];
                uuid_unparse(resp.deployment_id, depID);
                ESP_LOGI(TAG, "A deployment is available!");
                ESP_LOGI(TAG, "Deployment ID: %s", depID);
                ESP_LOGI(TAG, "Asset Identifier: %s", resp.asset.asset_identifier);
                ESP_LOGI(TAG, "Asset Version: %s", resp.asset.asset_version);
                ESP_LOGI(TAG, "Asset Size: %d", resp.asset.asset_size);
                ESP_LOGI(TAG, "Asset URL: %s", resp.asset.asset_url);
                EscapeQuestionMarks(resp.asset.asset_url, URL);
                strcpy(resp.asset.asset_url, URL);

                short check_update_status = 0;
                while (check_update_status < 5)
                {

                    // TODO: Anedya update OTA status to start
                    anedya_req_ota_update_status_t update_status = {
                        .deployment_id = resp.deployment_id,
                        .status = ANEDYA_OTA_STATUS_START,
                    };
                    anedya_txn_t update_txn;
                    anedya_txn_register_callback(&update_txn, TXN_COMPLETE, &current_task);
                    anedya_err_t uerr = anedya_op_ota_update_status_req(&anedya_client, &update_txn, &update_status);
                    if (aerr != ANEDYA_OK)
                    {
                        ESP_LOGI(TAG, "%s", anedya_err_to_name(aerr));
                    }
                    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
                    if (ulNotifiedValue == 0x01)
                    {
                        ESP_LOGI(TAG, "TXN Complete");
                        check_update_status = 5;
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
                            check_update_status = 0;
                            while (check_update_status < 5)
                            {

                                anedya_err_t cerr = anedya_op_ota_update_status_req(&anedya_client, &conclude_txn, &conclude_status);
                                if (cerr != ANEDYA_OK)
                                {
                                    ESP_LOGI(TAG, "%s", anedya_err_to_name(cerr));
                                }
                                xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
                                if (ulNotifiedValue == 0x01)
                                {
                                    ESP_LOGI(TAG, "TXN Complete");
                                    check_update_status = 5;
                                }
                                else
                                {
                                    ESP_LOGI(TAG, "TXN Timeout");
                                    // TODO: Handle error
                                }
                                check_update_status++;
                                vTaskDelay(500 / portTICK_PERIOD_MS);
                            }

                            if (otaerr == ESP_OK)
                            {
                                ESP_LOGI(TAG, "Firmware has been updated. Waiting for shutdown process to complete.");
                                // while(xEventGroupGetBits(DeviceEvents) & DEVICE_POWER_CYCLE) {
                                //  Device still processing the storage request. Wait for some time
                                // vTaskDelay(100/portTICK_PERIOD_MS);
                                //}
                                ESP_LOGI(TAG, "Shutdown Process complete.");
                                for (int i = 0; i < 5; i++)
                                {
                                    ESP_LOGI(TAG, "Device will restart in: %d seconds", (5 - i));
                                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                                }
                                ESP_LOGI(TAG, "Restarting device!");
                                esp_restart();
                            }
                            else
                            {
                                ESP_ERROR_CHECK_WITHOUT_ABORT(otaerr);
                                ESP_LOGI(TAG, "Firmware update failed. Restarting device for continued operation!");
                                for (int i = 0; i < 5; i++)
                                {
                                    ESP_LOGI(TAG, "Device will restart in: %d seconds", (5 - i));
                                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                                }
                                ESP_LOGI(TAG, "Restarting device!");
                                esp_restart();
                            }
                        }
                    }
                    else
                    {
                        ESP_LOGI(TAG, "TXN Timeout");
                        // TODO: Handle error
                    }
                    check_update_status++;
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
            }
            else
            {
                ESP_LOGI(TAG, "No deployment available");
            }
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

static esp_err_t do_firmware_update(anedya_op_next_ota_resp_t *resp)
{
    ESP_LOGI(TAG, "Setting up device for new firmware update!");

    xEventGroupClearBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);
    vTaskDelay(20000 / portTICK_PERIOD_MS); // Allow tasks to finish

    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_partition == NULL)
    {
        ESP_LOGE(TAG, "No OTA partition found");
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(ota_partition, resp->asset.asset_size, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }

    int start_position = 0;
    time_t t = time(NULL);
    int chunk_size = 50000;  // call for the next chunk
    size_t read_len = 20480; // uart read len
    char *data = malloc(read_len * sizeof(char));
    while (start_position < resp->asset.asset_size)
    {
        anedya_ext_net_reader_t reader = {0};

        if ((int)resp->asset.asset_size - (int)start_position < chunk_size)
        {
            chunk_size = (int)resp->asset.asset_size - (int)start_position;
        }

        anedya_err_t oerr = anedya_ext_http_get_range_request(&anedya_client, &reader, resp->asset.asset_url, resp->asset.asset_url_len, start_position, chunk_size, 10000);
        if (oerr != ANEDYA_OK)
            continue;
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if (reader.content_length > 0)
        {
            while (reader.bytes_read != reader.content_length)
            {
                int def = reader.content_length - reader.bytes_read;
                if (def < read_len)
                {
                    read_len = (size_t)def;
                }
                size_t read = anedya_ext_ota_read_next(&anedya_client, &reader, read_len, data, 2000);
                if (read > 0)
                {
                    uint32_t offset = start_position + reader.bytes_read - read;
                    err = esp_ota_write_with_offset(ota_handle, data, read, (uint32_t)offset);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
                        esp_ota_end(ota_handle);
                        return err;
                    }
                    else
                    {
                        ESP_LOGI(TAG, "%d bytes written sucessfully", read);
                    }
                    // ESP_LOGI(TAG, "Reading %d bytes", read);
                    // for (int i = 0; i < read; i++)
                    // {
                    //     printf("%c", data[i]);
                    // }
                    ESP_LOGI(TAG, "Bytes read %d, Read Asset size %d, Percentage: %f %%  \n", start_position + reader.bytes_read, resp->asset.asset_size, ((float)(start_position + reader.bytes_read) / (float)resp->asset.asset_size) * 100);
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            // printf("\n");
            anedya_ext_ota_reader_close(&anedya_client, &reader);
        }
        start_position = start_position + chunk_size;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(data);
    time_t endTime = time(NULL);

    float time_taken = difftime(endTime, t);
    printf("Total time taken: %f seconds\n", time_taken);

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

static void EscapeQuestionMarks(char *input, char *output)
{
    bool firstChar = 0;
    for (int i = 0; i < strlen(input) + 1; i++)
    {
        char c;
        if (input[i] == '?')
        {
            if (firstChar)
            {
                c = '&';
            }
            else
            {
                firstChar = 1;
                c = input[i];
            }
        }
        else
        {
            c = input[i];
        }
        output[i] = c;
    }
}
