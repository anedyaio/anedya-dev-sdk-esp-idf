/*
 * This file is part of Anedya Core SDK
 * (c) 2024, Anedya Systems Private Limited
 */

#pragma once

#ifdef ___cpluplus
extern "C"
{
#endif

    typedef int anedya_err_t;

    const char *anedya_err_to_name(anedya_err_t code);

#define ANEDYA_OK 0
#define ANEDYA_ERR -1
#define ANEDYA_INTERFACE_BUSY -2

/* Anedya Error Definitions and Constants */
#define ANEDYA_ERR_INVALID_REGION 1
#define ANEDYA_ERR_INVALID_DEVICE_ID 2

// MQTT ERROR CODES
#define ANEDYA_ERR_INVALID_CREDENTIALS 3
#define ANEDYA_ERR_MQTT_RATE_LIMIT_EXCEEDED 4
#define ANEDYA_ERR_NO_MEMORY 5
#define ANEDYA_ERR_NOT_CONNECTED 6
#define ANEDYA_ERR_MAX_TXN_EXCEEDED 7
#define ANEDYA_ERR_LOCK_FAILED 8

#define ANEDYA_ERR_RX_BUFFER_OVERFLOW 9
#define ANEDYA_ERR_INVALID_UUID 10
#define ANEDYA_ERR_PARSE_ERROR 11
#define ANEDYA_ERR_INVALID_DATATYPE 12
#define ANEDYA_ERR_INVALID_DATA 13

#ifdef ___cpluplus
}
#endif
