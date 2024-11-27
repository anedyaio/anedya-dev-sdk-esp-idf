#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "anedya.h"
#include "anedya_err.h"
#include "anedya_config.h"

#define parsehex(x) (('0' <= x && x <= '9') ? (x - '0') : (('a' <= x && x <= 'f') ? (10 + (x - 'a')) : (('A' <= x && x <= 'F') ? (10 + (x - 'A')) : (-1))))

anedya_err_t anedya_parse_device_id(const char deviceID[37], anedya_device_id_t devID)
{
    static const int8_t si[16] = {0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34};
    if (strnlen(deviceID, 37) != 36)
    {
        return ANEDYA_ERR_INVALID_DEVICE_ID;
    }
    for (int i = 0; i < 36; i++)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (deviceID[i] != '-')
            {
                return ANEDYA_ERR_INVALID_DEVICE_ID;
            }
        }
        else
        {
            if (parsehex(deviceID[i]) == -1)
            {
                return ANEDYA_ERR_INVALID_DEVICE_ID;
            }
        }
    }
    for (int i = 0; i < 16; i++)
    {
        int8_t hi = (int8_t)parsehex(deviceID[si[i] + 0]);
        int8_t lo = (int8_t)parsehex(deviceID[si[i] + 1]);
        devID[i] = ((hi << 4) | lo);
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_config_init(anedya_config_t *config, anedya_device_id_t devId, const char *connection_key, size_t connection_key_len)
{
    config->connection_key = connection_key;
    config->connection_key_len = connection_key_len;
    memcpy(config->device_id, devId, sizeof(anedya_device_id_t));
    char uuid_str[37];
    for (int i = 0; i < 37; i++)
    {
        uuid_str[i] = 0;
    }
    sprintf(uuid_str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            devId[0], devId[1],
            devId[2], devId[3],
            devId[4], devId[5],
            devId[6], devId[7],
            devId[8], devId[9],
            devId[10], devId[11],
            devId[12], devId[13],
            devId[14], devId[15]);
    // printf("UUID: %s\n", uuid_str);
    memcpy(config->_device_id_str, uuid_str, 37);
    config->event_handler = NULL;
    config->on_connect = NULL;
    config->on_disconnect = NULL;
    return ANEDYA_OK;
}

anedya_err_t anedya_config_set_region(anedya_config_t *config, const char *region)
{
    if (strcmp(region, ANEDYA_REGION_AP_IN_1) == 0)
    {
        config->region = ANEDYA_REGION_AP_IN_1;
    }
    else
    {
        return ANEDYA_ERR_INVALID_REGION;
    }
    return ANEDYA_OK;
}

anedya_err_t anedya_config_set_timeout(anedya_config_t *config, size_t timeout_sec)
{
    config->timeout = timeout_sec;
    return ANEDYA_OK;
}

#ifdef ANEDYA_CONNECTION_METHOD_MQTT
anedya_err_t anedya_config_set_connect_cb(anedya_config_t *config, anedya_on_connect_cb_t on_connect, anedya_context_t ctx)
{
    if (on_connect == NULL)
    {
        return ANEDYA_ERR;
    }
    config->on_connect = on_connect;
    config->on_connect_ctx = ctx;
    return ANEDYA_OK;
}

anedya_err_t anedya_config_set_disconnect_cb(anedya_config_t *config, anedya_on_disconnect_cb_t on_disconnect, anedya_context_t ctx)
{
    if (on_disconnect == NULL)
    {
        return ANEDYA_ERR;
    }
    config->on_disconnect = on_disconnect;
    config->on_disconnect_ctx = ctx;
    return ANEDYA_OK;
}

anedya_err_t anedya_config_register_event_handler(anedya_config_t *config, anedya_event_handler_t event_handler, anedya_context_t ctx)
{
    if (event_handler == NULL)
    {
        return ANEDYA_ERR;
    }
    config->event_handler = event_handler;
    config->event_handler_ctx = ctx;
    return ANEDYA_OK;
}
#endif