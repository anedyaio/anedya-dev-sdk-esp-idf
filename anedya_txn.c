#include "anedya_txn.h"
#include "anedya_client.h"
#include <stdint.h>
#include<stdio.h>


anedya_err_t _anedya_txn_register(anedya_client_t *client, anedya_txn_t *txn) {
    // First go to store and get a new slot
    anedya_err_t err = _anedya_txn_store_aquire_slot(&client->txn_store, txn);
    if (err != ANEDYA_OK) {
        return err;
    }
    //printf("Aquired slot %d\n", txn->desc);
    return ANEDYA_OK;
}

anedya_err_t _anedya_txn_complete(anedya_client_t *client, anedya_txn_t *txn) {
    // Mark transaction as completed
    txn->is_complete = true;
    // First call the callback if it is not NULL
    if(txn->callback != NULL){
        txn->callback(txn, txn->ctx);
    }
    // Then release the slot
    //printf("Releasing slot %d\n", txn->desc);
    _anedya_txn_store_release_slot(&client->txn_store, txn);
    return ANEDYA_OK;
}


anedya_txn_desc_t _anedya_txn_get_desc(anedya_txn_t *txn) {
    return txn->desc;
}

void anedya_txn_register_callback(anedya_txn_t *txn, anedya_txn_cb_t callback, anedya_context_t ctx) {
    txn->callback = callback;
    txn->ctx = ctx;
}