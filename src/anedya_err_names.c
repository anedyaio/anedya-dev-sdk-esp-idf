/*
* This file is part of Anedya Core SDK
* (c) 2024, Anedya Systems Private Limited
*/

#include "anedya_sdk_config.h"
#include "anedya_err.h"
#include <stddef.h>


#ifdef ANEDYA_INCLUDE_ERR_NAMES
#define ERR_TBL_IT(err)    {err, #err}


typedef struct {
    anedya_err_t code;
    const char *msg;
} anedya_err_msg_t;

static const anedya_err_msg_t anedya_err_msg_table[] = {
    ERR_TBL_IT(ANEDYA_OK),
    ERR_TBL_IT(ANEDYA_ERR),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_REGION),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_DEVICE_ID),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_CREDENTIALS),
    ERR_TBL_IT(ANEDYA_ERR_MQTT_RATE_LIMIT_EXCEEDED),
    ERR_TBL_IT(ANEDYA_ERR_NO_MEMORY),
    ERR_TBL_IT(ANEDYA_ERR_NOT_CONNECTED),
    ERR_TBL_IT(ANEDYA_ERR_LOCK_FAILED),
    ERR_TBL_IT(ANEDYA_ERR_RX_BUFFER_OVERFLOW),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_UUID),
    ERR_TBL_IT(ANEDYA_ERR_MAX_TXN_EXCEEDED),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_DATATYPE),
    ERR_TBL_IT(ANEDYA_ERR_INVALID_DATA),
};

#endif

static const char anedya_unknown_msg[] =
#ifdef ANEDYA_INCLUDE_ERR_NAMES
    "ERROR";
#else
    "UNKNOWN ERROR";
#endif //ANEDYA_INCLUDE_ERR_NAMES

const char *anedya_err_to_name(anedya_err_t code)
{
#ifdef ANEDYA_INCLUDE_ERR_NAMES
    size_t i;

    for (i = 0; i < sizeof(anedya_err_msg_table)/sizeof(anedya_err_msg_table[0]); ++i) {
        if (anedya_err_msg_table[i].code == code) {
            return anedya_err_msg_table[i].msg;
        }
    }
#endif //ANEDYA_INCLUDE_ERR_NAMES

    return anedya_unknown_msg;
}