#pragma once

#include "anedya_commons.h"
#include "anedya_models.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    bool deployment_available;
    anedya_uuid_t deployment_id;
    anedya_asset_t asset;
} anedya_op_next_ota_resp_t;

typedef struct {
    const char *status;
    anedya_uuid_t *deployment_id;
} anedya_req_ota_update_status_t;

#define ANEDYA_OTA_STATUS_START "start"
#define ANEDYA_OTA_STATUS_SUCCESS "success"
#define ANEDYA_OTA_STATUS_FAILURE "failure"
#define ANEDYA_OTA_STATUS_SKIPPED "skipped"

anedya_err_t _anedya_op_ota_next_parser(json_t *json, anedya_op_next_ota_resp_t *resp);

#ifdef __cplusplus
}
#endif
