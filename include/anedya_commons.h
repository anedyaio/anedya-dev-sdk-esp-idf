#pragma once

#include <stdlib.h>
#include "anedya_sdk_config.h"
#include "anedya_err.h"

/**
 * @brief: This file contains common datatypes and constants used in the SDK
 */

#ifdef __cplusplus
extern "C"
{
#endif

// Region Codes
#define ANEDYA_REGION_AP_IN_1 "ap-in-1"

#define ANEDYA_URL_PREFIX "device."
#define ANEDYA_URL_SUFFIX ".anedya.io"

#define ANEDYA_TXN_COMPLETE 0x1
#define ANEDYA_TXN_INCOMPLETE 0x0

    // Common datatypes
    typedef uint8_t anedya_uuid_t[16];
    typedef unsigned char anedya_device_id_t[16];
    typedef char anedya_device_id_str_t[37];
    typedef unsigned char anedya_bind_secret_t[32];
    typedef unsigned int anedya_client_descriptor_t;
    typedef void *anedya_context_t;
    typedef uint8_t anedya_event_t;
    typedef struct
    {

    } anedya_command_t;

    typedef struct anedya_client_t anedya_client_t;
    typedef struct anedya_txn_t anedya_txn_t;
    typedef struct anedya_txn_store_t anedya_txn_store_t;

#ifdef ANEDYA_CONNECTION_METHOD_MQTT
    typedef void *anedya_mqtt_client_handle_t;
    /** @brief: On connect callback, which is called when connection is established with the MQTT Server */
    typedef void (*anedya_on_connect_cb_t)(anedya_context_t ctx);
    /** @brief: On connect callback, which is called when client is disconnected with the MQTT Server. It can be intentional or due to some other error */
    typedef void (*anedya_on_disconnect_cb_t)(anedya_context_t ctx);
    /** @brief: On command callback, which is called when a command is received from the MQTT Server */
    typedef void (*anedya_on_command_cb_t)(anedya_context_t ctx, anedya_command_t *command);
    /** @brief: On event callback, which is called when an event is received from the MQTT Server */
    typedef void (*anedya_event_handler_t)(anedya_client_t *cl, anedya_event_t event_type, void *event_data);
#endif

#define ANEDYA_DATATYPE_UNKNOWN 0
#define ANEDYA_DATATYPE_STRING 1
#define ANEDYA_DATATYPE_BINARY 2

#ifdef __cplusplus
}
#endif