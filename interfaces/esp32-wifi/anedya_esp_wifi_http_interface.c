/*
 * Anedya ESP32 WiFi – HTTP Interface
 * (c) 2024, Anedya Systems Private Limited
 *
 * Implements the _anedya_interface_http_post / _anedya_interface_http_get
 * contract declared in anedya_interface.h for the ESP32 internal WiFi
 * interface.
 *
 * This file is only compiled when CONFIG_AN_INTERFACE_ESP32_WIFI is selected
 * (i.e. same sdkconfig guard as the existing MQTT WiFi interface) AND
 * ANEDYA_CONNECTION_METHOD_HTTP is defined (i.e. CONFIG_CONN_ANEDYA_HTTPS).
 */

#include "anedya_sdk_config.h"

#ifdef CONFIG_AN_INTERFACE_ESP32_WIFI
#ifdef ANEDYA_CONNECTION_METHOD_HTTP

#include "anedya_client.h"
#include "anedya_commons.h"
#include "anedya_interface.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "time.h"
#include <string.h>
#include <sys/time.h>

static const char *TAG = "ANEDYA_HTTP";

anedya_err_t _anedya_interface_init(anedya_client_t *client) {
  return ANEDYA_OK;
}
void _anedya_interface_sleep_ms(size_t ms) {
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

uint64_t _anedya_interface_get_time_ms() {
  struct timeval ts;
  gettimeofday(&ts, NULL);
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_usec / 1000;
}

void _anedya_interface_std_out(const char *str) { ESP_LOGI(TAG, "%s", str); }

/* ────────────────────────────────────────────────────────────────────────────
 * Internal response-accumulator callback
 *
 * esp_http_client calls this repeatedly as HTTP response data arrives.
 * We accumulate the chunks into resp_buf until resp_buf_size is exhausted.
 * ───────────────────────────────────────────────────────────────────────────
 */
typedef struct {
  char *buf;
  int buf_size;
  int written;
} _anedya_http_ctx_t;

static esp_http_client_handle_t global_http_client = NULL;
static SemaphoreHandle_t http_mutex = NULL;
static _anedya_http_ctx_t current_http_ctx;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  _anedya_http_ctx_t *ctx = (_anedya_http_ctx_t *)evt->user_data;
  switch (evt->event_id) {
  case HTTP_EVENT_ON_DATA:
    if (ctx->written + evt->data_len < ctx->buf_size - 1) {
      memcpy(ctx->buf + ctx->written, evt->data, evt->data_len);
      ctx->written += evt->data_len;
    } else {
      ESP_LOGW(TAG, "HTTP response truncated: buffer full");
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

static anedya_err_t _init_global_http_client(anedya_client_t *client) {
  if (global_http_client != NULL) {
    return ANEDYA_OK;
  }
  char url[200];
  snprintf(url, sizeof(url), "https://%s/", client->http_base_url);

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = _http_event_handler,
      .user_data = &current_http_ctx, // Bind to our static context
      .timeout_ms = client->config->timeout * 1000,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .keep_alive_enable = true,
  };
  global_http_client = esp_http_client_init(&config);
  if (global_http_client == NULL) {
    ESP_LOGE(TAG, "Failed to initialise HTTP client");
    return ANEDYA_ERR;
  }
  return ANEDYA_OK;
}

/* ────────────────────────────────────────────────────────────────────────────
 * _anedya_interface_http_post
 *
 * Performs a TLS-secured HTTP POST to:
 *   https://device.<region>.anedya.io<path>
 *
 * Auth headers:
 *   Auth-mode: key
 *   Authorization: <connection_key>
 *   Content-Type: application/json
 * ───────────────────────────────────────────────────────────────────────────
 */
anedya_err_t _anedya_interface_http_post(anedya_client_t *client,
                                         const char *path, const char *payload,
                                         int payload_len, char *resp_buf,
                                         int resp_buf_size, int *resp_len) {
  if (http_mutex == NULL) {
    http_mutex = xSemaphoreCreateMutex();
  }

  xSemaphoreTake(http_mutex, portMAX_DELAY);

  /* Build the full URL: https://device.<region>.anedya.io<path> */
  char url[200];
  snprintf(url, sizeof(url), "https://%s%s", client->http_base_url, path);

  /* Set up static context for response (thread-safe due to mutex) */
  memset(resp_buf, 0, resp_buf_size);
  current_http_ctx.buf = resp_buf;
  current_http_ctx.buf_size = resp_buf_size;
  current_http_ctx.written = 0;

  if (global_http_client == NULL) {
    if (_init_global_http_client(client) != ANEDYA_OK) {
      xSemaphoreGive(http_mutex);
      return ANEDYA_ERR;
    }
  }

  esp_http_client_set_url(global_http_client, url);
  esp_http_client_set_method(global_http_client, HTTP_METHOD_POST);

  /* Set authentication and content-type headers */
  esp_http_client_set_header(global_http_client, "Auth-mode", "key");
  esp_http_client_set_header(global_http_client, "Authorization",
                             client->config->connection_key);
  esp_http_client_set_header(global_http_client, "Content-Type",
                             "application/json");

  /* Attach the request body */

  ESP_LOGI(TAG, "Path %s, Payload %s", path, payload);
  esp_http_client_set_post_field(global_http_client, payload, payload_len);

  esp_err_t err = esp_http_client_perform(global_http_client);
  anedya_err_t ret = ANEDYA_OK;

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    // If persistent connection fails, clean up so we recreate it fresh next
    // time
    esp_http_client_cleanup(global_http_client);
    global_http_client = NULL;
    ret = ANEDYA_ERR;
  } else {
    int http_status = esp_http_client_get_status_code(global_http_client);
    if (http_status != 200) {
      ESP_LOGW(TAG, "HTTP POST returned status %d", http_status);
    }
    if (current_http_ctx.written == 0) {
      ret = ANEDYA_ERR;
    }
    ESP_LOGI(TAG, "Path %s, Response %s", path, resp_buf);
    /* Null-terminate the response and report its length */
    resp_buf[current_http_ctx.written] = '\0';
    *resp_len = current_http_ctx.written;
    ESP_LOGD(TAG, "HTTP POST OK, resp_len=%d", current_http_ctx.written);
  }

  xSemaphoreGive(http_mutex);
  return ret;
}

/* ────────────────────────────────────────────────────────────────────────────
 * _anedya_interface_http_get
 *
 * Reserved for future use. All current Anedya device endpoints use POST.
 * ───────────────────────────────────────────────────────────────────────────
 */
anedya_err_t _anedya_interface_http_get(anedya_client_t *client,
                                        const char *path, char *resp_buf,
                                        int resp_buf_size, int *resp_len) {
  if (http_mutex == NULL) {
    http_mutex = xSemaphoreCreateMutex();
  }

  xSemaphoreTake(http_mutex, portMAX_DELAY);

  char url[200];
  snprintf(url, sizeof(url), "https://%s%s", client->http_base_url, path);

  memset(resp_buf, 0, resp_buf_size);
  current_http_ctx.buf = resp_buf;
  current_http_ctx.buf_size = resp_buf_size;
  current_http_ctx.written = 0;

  if (global_http_client == NULL) {
    if (_init_global_http_client(client) != ANEDYA_OK) {
      xSemaphoreGive(http_mutex);
      return ANEDYA_ERR;
    }
  }

  esp_http_client_set_url(global_http_client, url);
  esp_http_client_set_method(global_http_client, HTTP_METHOD_GET);

  esp_http_client_set_header(global_http_client, "Auth-mode", "key");
  esp_http_client_set_header(global_http_client, "Authorization",
                             client->config->connection_key);

  esp_err_t err = esp_http_client_perform(global_http_client);
  anedya_err_t ret = ANEDYA_OK;

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(global_http_client);
    global_http_client = NULL;
    ret = ANEDYA_ERR;
  } else {
    int http_status = esp_http_client_get_status_code(global_http_client);
    if (http_status != 200) {
      ESP_LOGW(TAG, "HTTP GET returned status %d", http_status);
    }
    if (current_http_ctx.written == 0) {
      ret = ANEDYA_ERR;
    }
    resp_buf[current_http_ctx.written] = '\0';
    *resp_len = current_http_ctx.written;
  }

  xSemaphoreGive(http_mutex);
  return ret;
}

#endif /* ANEDYA_CONNECTION_METHOD_HTTP */
#endif /* CONFIG_AN_INTERFACE_ESP32_WIFI */
