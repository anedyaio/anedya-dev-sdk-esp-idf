#pragma once

#ifndef _ANEDYA_REQ_H_
#define _ANEDYA_REQ_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "anedya_sdk_config.h"
#include "anedya_commons.h"


typedef struct
{
    #ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
        char key[50];
        char value[50];
    #endif 
    #ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
        char *key;
        char *value;
    #endif
    size_t key_len;
    size_t value_len;
} anedya_asset_metadata_t;

typedef struct
{
    anedya_uuid_t asset_id;
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
    char asset_identifier[50];
    char asset_version[50];
    char asset_checksum[255];
    char asset_url[1000];
    char asset_signature[255];
#endif
#ifdef ANEDYA_ENABLE_DYNAMIC_ALLOCATION
    char *asset_identifier;
    char *asset_version;
    char *asset_checksum;
    char *asset_url;
#endif
    anedya_asset_metadata_t *asset_metadata;
    size_t asset_metadata_len;
    size_t asset_identifier_len;
    size_t asset_version_len;
    size_t asset_checksum_len;
    size_t asset_url_len;
    bool asset_signed;
    size_t asset_signature_len;
    size_t asset_size;
} anedya_asset_t;

typedef struct
{
    anedya_device_id_str_t devId;
    char *binding_secret;
    size_t binding_secret_len;
} anedya_req_bind_device_t;

anedya_err_t anedya_req_bind_device_marshal(anedya_req_bind_device_t *req, char *buffer);
anedya_err_t anedya_req_bind_device_unmarshal(anedya_req_bind_device_t *req, char *buffer);

anedya_err_t _anedya_uuid_parse(const char *in, anedya_uuid_t uuid);
void _anedya_uuid_marshal(const anedya_uuid_t uuid, char *out);
int _anedya_strcmp(const char* x, const char* y);
unsigned int _anedya_base64_encode(unsigned char *data, unsigned char *output);
unsigned int _anedya_base64_decode(unsigned char* encoded, char *data);
#endif