#include "commandHandler.h"
#include "sync.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/gpio.h"

// Static task handle and notified value
static TaskHandle_t current_task;
static uint32_t ulNotifiedValue;

// Log tag
static const char *TAG = "COMMAND_HANDLER";

// Declare the status update structure
static anedya_req_cmd_status_update_t command_status_update;

static void update_command_status(anedya_req_cmd_status_update_t *command_status_update);

void commandHandling_task(void *pvParameters)
{
    current_task = xTaskGetCurrentTaskHandle();
    // Set GPIO 2 as output
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

    anedya_txn_t cmd_list_txn;
    anedya_txn_register_callback(&cmd_list_txn, TXN_COMPLETE, &current_task);
    anedya_req_cmd_list_obj_t cmd_list_req = {
        .limit = 1,
        .offset = 0,
    };
    anedya_op_cmd_list_obj_resp_t cmd_list_resp;
    anedya_command_obj_list_t cmd_obj_list[cmd_list_req.limit];
    cmd_list_resp.commands = cmd_obj_list;
    cmd_list_txn.response = &cmd_list_resp;
    anedya_err_t err = anedya_op_cmd_list_obj(&anedya_client, &cmd_list_txn, cmd_list_req);
    if (err != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to list commands: %d", err);
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (cmd_list_txn.is_success && cmd_list_txn.is_complete)
        {
            printf("================================================================\n");
            ESP_LOGI(TAG, "Command List:");
            for (int i = 0; i < cmd_list_req.limit; i++)
            {
                ESP_LOGI(TAG, "Command ID: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                        cmd_list_resp.commands[i].cmdId[0], cmd_list_resp.commands[i].cmdId[1],
                        cmd_list_resp.commands[i].cmdId[2], cmd_list_resp.commands[i].cmdId[3],
                        cmd_list_resp.commands[i].cmdId[4], cmd_list_resp.commands[i].cmdId[5],
                        cmd_list_resp.commands[i].cmdId[6], cmd_list_resp.commands[i].cmdId[7],
                        cmd_list_resp.commands[i].cmdId[8], cmd_list_resp.commands[i].cmdId[9],
                        cmd_list_resp.commands[i].cmdId[10], cmd_list_resp.commands[i].cmdId[11],
                        cmd_list_resp.commands[i].cmdId[12], cmd_list_resp.commands[i].cmdId[13],
                        cmd_list_resp.commands[i].cmdId[14], cmd_list_resp.commands[i].cmdId[15]);
                ESP_LOGI(TAG, "Command Name: %s", cmd_list_resp.commands[i].command);
                ESP_LOGI(TAG, "Command Status: %s", cmd_list_resp.commands[i].status);
            }
            printf("================================================================\n");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to list commands");
    }

    while (1)
    {
        // Wait for the relevant events
        xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(ConnectionEvents, MQTT_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupWaitBits(gatewaystate.COMMANDEVENTS, COMMAND_AVAILABLE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // Update the command status object
        memset(&command_status_update, 0, sizeof(command_status_update)); // Clear the structure
        memcpy(command_status_update.cmdId, command_obj->cmdId, sizeof(command_status_update.cmdId));
        command_status_update.status = ANEDYA_CMD_STATUS_RECEIVED;

        // Update the command status in the system
        update_command_status(&command_status_update);

        // Handle the command based on its data type
        if (command_obj->cmd_data_type == ANEDYA_DATATYPE_STRING)
        {
            if (strcmp(command_obj->command, "led") == 0)
            {
                if (strcmp(command_obj->data, "on") == 0)
                {
                    gpio_set_level(GPIO_NUM_2, true);
                    printf("LED turned on\n");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    update_command_status(&command_status_update);
                }
                else if (strcmp(command_obj->data, "off") == 0)
                {
                    gpio_set_level(GPIO_NUM_2, false);
                    printf("LED turned off\n");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    update_command_status(&command_status_update);
                }
                else
                {
                    ESP_LOGE("COMMAND_HANDLER", "Invalid Command");
                    command_status_update.status = ANEDYA_CMD_STATUS_FAILED;
                    update_command_status(&command_status_update);
                }
            }
        }
        else if (command_obj->cmd_data_type == ANEDYA_DATATYPE_BINARY)
        {
            printf("Data: ");
            for (int i = 0; i < command_obj->data_len; i++)
            {
                printf("%c", command_obj->data[i]);
            }
            printf("\n");
        }

        // Clear the command event bit to process the next command
        xEventGroupClearBits(gatewaystate.COMMANDEVENTS, COMMAND_AVAILABLE_BIT);
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

static void update_command_status(anedya_req_cmd_status_update_t *command_status_update)
{
    anedya_txn_t cmd_txn;
    anedya_txn_register_callback(&cmd_txn, TXN_COMPLETE, &current_task);

    ESP_LOGI(TAG, "Status update: %s", command_status_update->status); // Updated for string status

    anedya_op_cmd_status_update(&anedya_client, &cmd_txn, command_status_update);

    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (cmd_txn.is_success && cmd_txn.is_complete)
        {
            ESP_LOGI("COMMAND_STATUS_HANDLER", "----------------------");
            ESP_LOGI("COMMAND_STATUS_HANDLER", "Command Status Updated");
            ESP_LOGI("COMMAND_STATUS_HANDLER", "----------------------");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update Command Status");
        }
    }
}
