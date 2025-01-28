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

    //================================== Get Queued Commands List ================================
    anedya_txn_t cmd_list_txn;
    anedya_txn_register_callback(&cmd_list_txn, TXN_COMPLETE, &current_task);
    anedya_req_cmd_list_obj_t cmd_list_req = {
        .limit = 1,
        .offset = 0,
    };
    anedya_op_cmd_queued_obj_resp_t cmd_list_resp;
    anedya_command_queued_obj_t cmd_obj_list[cmd_list_req.limit];
    cmd_list_resp.commands = cmd_obj_list;
    cmd_list_txn.response = &cmd_list_resp;
    anedya_err_t err = anedya_op_cmd_queued_obj(&anedya_client, &cmd_list_txn, cmd_list_req);
    if (err != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to list commands: %d", err);
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (cmd_list_txn.is_success && cmd_list_txn.is_complete)
        {
            ESP_LOGI(TAG, "-------------------------------------");
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
            ESP_LOGI(TAG, "--------------------------------------");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to list commands");
    }
    ulNotifiedValue = 0x00;

    // ================================== Get next command ================================
    anedya_txn_t cmd_next_txn;
    anedya_txn_register_callback(&cmd_next_txn, TXN_COMPLETE, &current_task);
    anedya_op_cmd_next_resp_t cmd_next_resp;
    unsigned char data[100]; // You can adjust the buffer size as needed
    cmd_next_resp.data = data;
    cmd_next_txn.response = &cmd_next_resp;
    anedya_err_t nerr = anedya_op_cmd_next(&anedya_client, &cmd_next_txn);
    if (nerr != ANEDYA_OK)
    {
        ESP_LOGE(TAG, "Failed to get next command: %d", nerr);
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, 30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01)
    {
        if (cmd_next_txn.is_success && cmd_next_txn.is_complete)
        {
            ESP_LOGI(TAG, "Available: %s", cmd_next_resp.available ? "true" : "false");
            ESP_LOGI(TAG, "Command: %s", cmd_next_resp.command);
            ESP_LOGI(TAG, "Command Length: %u", cmd_next_resp.command_len);
            ESP_LOGI(TAG, "Data: %s", cmd_next_resp.data);
            ESP_LOGI(TAG, "Data Length: %u", cmd_next_resp.data_len);
            ESP_LOGI(TAG, "Status: %s", cmd_next_resp.status);
            ESP_LOGI(TAG, "Data Type: %u", cmd_next_resp.data_type);
            ESP_LOGI(TAG, "Issued At: %llu", cmd_next_resp.issued_at);
            ESP_LOGI(TAG, "Updated At: %llu", cmd_next_resp.updated);
            ESP_LOGI(TAG, "Next Available: %s", cmd_next_resp.nextavailable ? "true" : "false");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get next command");
    }
    ulNotifiedValue = 0x00;

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
                    ESP_LOGI(TAG, "LED turned on");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    update_command_status(&command_status_update);
                }
                else if (strcmp(command_obj->data, "off") == 0)
                {
                    gpio_set_level(GPIO_NUM_2, false);
                    ESP_LOGI(TAG, "LED turned off");
                    command_status_update.status = ANEDYA_CMD_STATUS_SUCCESS;
                    update_command_status(&command_status_update);
                }
                else
                {
                    ESP_LOGE(TAG, "Invalid Command");
                    command_status_update.status = ANEDYA_CMD_STATUS_FAILED;
                    update_command_status(&command_status_update);
                }
            }
        }
        else if (command_obj->cmd_data_type == ANEDYA_DATATYPE_BINARY)
        {
            ESP_LOGI(TAG, "Data: ");
            for (int i = 0; i < command_obj->data_len; i++)
            {
                ESP_LOGI(TAG, "%c", command_obj->data[i]);
            }
            ESP_LOGI(TAG, "");
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
            ESP_LOGI(TAG, "----------------------");
            ESP_LOGI(TAG, "Command Status Updated");
            ESP_LOGI(TAG, "----------------------");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update Command Status");
        }
    }
}

