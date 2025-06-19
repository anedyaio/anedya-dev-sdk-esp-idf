#pragma once

#include "anedya_sdk_config.h"

#ifdef ___cpluplus
extern "C"
{
#endif

#ifdef ASDK_NI_MODEM_QUECTEL

#include "esp_err.h"
#include "anedya_commons.h"
#include "anedya_config.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h> //Todo: REMOVE THIS

  typedef struct
  {
    int status_code;
    int content_length;
    int bytes_read;
    int _lock_uart_event_handler;
  } anedya_ext_net_reader_t;

  // Internal Functions
  anedya_err_t _anedya_ext_send_AT_command(char *cmd, unsigned int cmd_type, char *resp, char *expected_resp, size_t timeout);

  anedya_err_t anedya_ext_uart_init(anedya_client_t *parent);
  anedya_err_t anedya_ext_connectivity_check(anedya_client_t *client, int timeout);
  anedya_err_t anedya_ext_restore_settings_to_factory_defaults(anedya_client_t *client, int timeout);
  anedya_err_t anedya_ext_set_fun_mode(anedya_client_t *client, int fun, int rst, bool wait_for_rdy, int timeout);

  anedya_err_t anedya_ext_network_reg_status(anedya_client_t *client,int *stat, int timeout);
  anedya_err_t anedya_ext_network_operator(anedya_client_t *client,int *mode, int timeout);
  anedya_err_t anedya_ext_signal_quality(anedya_client_t *client, int *rssi, int *ber, int timeout);
  anedya_err_t anedya_ext_pdp_context_status(anedya_client_t *client, char *pdp_context, int timeout);
  anedya_err_t anedya_ext_activate_pdp_context(anedya_client_t *client, int cid, int timeout);
  anedya_err_t anedya_ext_deactivate_pdp_context(anedya_client_t *client, int cid, int timeout);
  anedya_err_t anedya_ext_read_pdp_context(anedya_client_t *client, char *pdp_context, int timeout);


  anedya_err_t anedya_ext_net_check(anedya_client_t *client, char *url, int timeout);
  anedya_err_t anedya_ext_set_apn(anedya_client_t *client, int cid, char *ip_ver, char *apn, char *user, char *pass);
  anedya_err_t anedya_ext_get_modem_time(anedya_client_t *client, int mode, char *output_dateTime);

  // OTA related functions
  anedya_err_t anedya_ext_http_get_range_request(anedya_client_t *client, anedya_ext_net_reader_t *reader, char *url, int url_len, int starting_position, int readlen, int timeout);
  size_t anedya_ext_ota_read_next(anedya_client_t *client, anedya_ext_net_reader_t *reader, int read_len, char *output, int timeout);
  anedya_err_t anedya_ext_ota_reader_close(anedya_client_t *client, anedya_ext_net_reader_t *reader);

#endif

#ifdef __cplusplus
}
#endif