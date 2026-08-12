#include "anedya.h"
#include "anedya_op_commands.h"
#include "commandHandler.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "otaManagement.h"
#include "submitData.h"
#include "submitLog.h"
#include "sync.h"
#include "timeManagement.h"
#include "valueStore.h"
#include "wifi.h"
#include <stdio.h>

sync_data_t gatewaystate;
anedya_config_t anedya_client_config;
anedya_client_t anedya_client;

static const char *TAG = "MAIN";

static uint32_t ulNotifiedValue;
static TaskHandle_t current_task;
static EventGroupHandle_t event_group;

anedya_command_obj_t *command_obj = NULL;

/* ============================================================================
 * app_main
 * ========================================================================== */
void app_main(void) {
  char connkey[64] = CONFIG_CONNECTION_KEY;
  anedya_device_id_t devid;
  anedya_parse_device_id(CONFIG_PHYSICAL_DEVICE_ID, devid);

  current_task = xTaskGetCurrentTaskHandle();
  event_group = xEventGroupCreate();
  ConnectionEvents = xEventGroupCreate();
  OtaEvents = xEventGroupCreate();
  gatewaystate.DeviceTimeEvents = xEventGroupCreate();
  gatewaystate.COMMANDEVENTS = xEventGroupCreate();

  xEventGroupSetBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT);

  /* ── WiFi path ─────────────────────────────────────────────────────────── */
  xEventGroupSetBits(ConnectionEvents, WIFI_FAIL_BIT);
  xTaskCreate(wifi_task, "WIFI", 4096, NULL, 1, NULL);
  /* Wait until WiFi gets an IP before doing anything else */
  xEventGroupWaitBits(ConnectionEvents, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
                      portMAX_DELAY);
  ESP_LOGI(TAG, "WiFi connected");

  /* ──────────────────────────────────────────────────────────────────────────
   * Sync device time (SNTP / modem clock)
   * ──────────────────────────────────────────────────────────────────────────
  //  */
  xTaskCreate(&syncTime_task, "syncTime", 4096, &gatewaystate, 4, NULL);
  xEventGroupWaitBits(gatewaystate.DeviceTimeEvents, SYNCED_DEVICE_TIME_BIT,
                      pdFALSE, pdFALSE, portMAX_DELAY);

  /* ──────────────────────────────────────────────────────────────────────────
   * Anedya client configuration
   * ──────────────────────────────────────────────────────────────────────────
   */
  anedya_config_init(&anedya_client_config, devid, connkey, strlen(connkey));
  anedya_config_set_region(&anedya_client_config, ANEDYA_REGION_AP_IN_1);
  anedya_config_set_timeout(&anedya_client_config, 30000);

  /* Initialize and connect */
  anedya_client_init(&anedya_client_config, &anedya_client);
  anedya_err_t aerr = anedya_client_connect(&anedya_client);
  if (aerr != ANEDYA_OK) {
    ESP_LOGE("CLIENT", "anedya_client_connect: %s", anedya_err_to_name(aerr));
  }
  xEventGroupSetBits(ConnectionEvents, ANEDYA_CONNECTED_BIT);

  /* ──────────────────────────────────────────────────────────────────────────
   * Optional: Device provisioning (bind)
   * ──────────────────────────────────────────────────────────────────────────
   */
#ifdef CONFIG_EN_PROVISIONING
  anedya_txn_t bind_txn;
  anedya_txn_register_callback(&bind_txn, TXN_COMPLETE, &current_task);
  anedya_req_bind_device_t req = {.binding_secret = CONFIG_BINDING_KEY,
                                  .binding_secret_len =
                                      strlen(CONFIG_BINDING_KEY)};
  ESP_LOGI("Prov", "Sending binding details");
  aerr = anedya_device_bind_req(&anedya_client, &bind_txn, &req);
  if (aerr != ANEDYA_OK) {
    ESP_LOGE("Prov", "bind_req err: %d", aerr);
  }
  xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue,
                  30000 / portTICK_PERIOD_MS);
  if (ulNotifiedValue == 0x01 && bind_txn.is_complete) {
    ESP_LOGI("Prov",
             bind_txn.is_success ? "Binding successful" : "Binding failed");
  } else {
    ESP_LOGE("Prov", "Binding TXN timeout");
  }
  ulNotifiedValue = 0x00;
  vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif

  /* ──────────────────────────────────────────────────────────────────────────
   * Start application tasks
   * ──────────────────────────────────────────────────────────────────────────
   */
  xTaskCreate(ota_management_task, "OTA", 20240, &gatewaystate, 1, NULL);
  xTaskCreate(submitData_task, "SUBMITDATA", 8096, NULL, 2, NULL);
  xTaskCreate(valueStore_task, "VALUESTORE", 10240, NULL, 4, NULL);
  xTaskCreate(commandHandling_task, "COMMANDHANDLER", 10240, NULL, 1, NULL);
  xTaskCreate(submitLog_task, "SUBMITLOG", 8096, NULL, 4, NULL);

  /* ──────────────────────────────────────────────────────────────────────────
   * Main loop: periodic heartbeat
   * ──────────────────────────────────────────────────────────────────────────
   */
  for (;;) {
    xEventGroupWaitBits(ConnectionEvents, ANEDYA_CONNECTED_BIT, pdFALSE,
                        pdFALSE, portMAX_DELAY);
    xEventGroupWaitBits(OtaEvents, OTA_NOT_IN_PROGRESS_BIT, pdFALSE, pdFALSE,
                        portMAX_DELAY);

    anedya_txn_t hb_txn;
    anedya_txn_register_callback(&hb_txn, TXN_COMPLETE, &current_task);
    aerr = anedya_device_send_heartbeat(&anedya_client, &hb_txn);
    if (aerr != ANEDYA_OK) {
      ESP_LOGE("CLIENT", "heartbeat err: %s", anedya_err_to_name(aerr));
    }
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue,
                    30000 / portTICK_PERIOD_MS);
    if (ulNotifiedValue == 0x01) {
      ESP_LOGI(TAG, "--------------------------------------");
      ESP_LOGI(TAG, "Heartbeat sent to Anedya successfully");
      ESP_LOGI(TAG, "--------------------------------------");
    } else {
      ESP_LOGE(TAG, "Failed to send heartbeat");
    }
    vTaskDelay(30000 / portTICK_PERIOD_MS);
  }
}
