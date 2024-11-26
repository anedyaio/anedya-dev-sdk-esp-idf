#pragma once

#ifndef _ANEDYA_TXN_H_
#define _ANEDYA_TXN_H_

#include <stdint.h>
#include <stdbool.h>
#include "anedya_err.h"
#include "anedya_client.h"
#include "anedya_sdk_config.h"

typedef uint8_t anedya_txn_desc_t;
typedef size_t anedya_op_t;
typedef size_t anedya_op_err_t;
typedef void *anedya_response_t;
typedef void (*anedya_txn_cb_t)(anedya_txn_t *txn, anedya_context_t ctx);

struct anedya_txn_t
{
    anedya_txn_desc_t desc;
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
    char *_txbody;
    char *_rxbody;
#endif
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char _rxbody[ANEDYA_RX_BUFFER_SIZE];
#endif
    size_t _rx_len;
    bool is_complete;
    bool is_success;
    anedya_context_t ctx;
    anedya_txn_cb_t callback;
    anedya_op_t _op;
    anedya_err_t _op_err;
    anedya_response_t response;
};

void anedya_txn_register_callback(anedya_txn_t *txn, anedya_txn_cb_t callback, anedya_context_t ctx);

anedya_err_t _anedya_txn_register(anedya_client_t *client, anedya_txn_t *txn);
anedya_err_t _anedya_txn_complete(anedya_client_t *client, anedya_txn_t *txn);
anedya_txn_desc_t _anedya_txn_get_desc(anedya_txn_t *txn);

#endif /* _ANEDYA_TXN_H_ */