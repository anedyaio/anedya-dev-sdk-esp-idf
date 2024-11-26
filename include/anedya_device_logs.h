#pragma once

#include "anedya_commons.h"
#include "anedya_err.h"

#ifdef ANEDYA_ENABLE_DEVICE_LOGS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {} anedya_log_batch_t;

typedef struct {} anedya_log_t;

anedya_err_t anedya_submit_log(anedya_client_t *client, anedya_log_t *logs);

anedya_err_t anedya_submit_bulk_log(anedya_client_t *client, anedya_log_batch_t *batch);

#ifdef __cplusplus
}
#endif

#endif