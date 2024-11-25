#pragma once

#include "anedya_sdk_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ANEDYA_TLS_ENABLE_RSA
#ifdef ANEDYA_EMBED_DER
    extern const unsigned char anedya_tls_root_ca[] extern const unsigned int anedya_tls_root_ca_len;
#endif
#ifdef ANEDYA_EMBED_PEM
    // array size is 1310
    extern const char anedya_tls_root_ca[];
    extern const unsigned int anedya_tls_root_ca_len;
#endif
#endif

#ifdef ANEDYA_TLS_ENABLE_ECC
#ifdef ANEDYA_EMBED_DER
    extern const unsigned char anedya_tls_root_ca[];
    extern const unsigned int anedya_tls_root_ca_len;
#endif
#ifdef ANEDYA_EMBED_PEM
    extern const char anedya_tls_root_ca[];
    extern const unsigned int anedya_tls_root_ca_len;
#endif
#endif

#ifdef __cplusplus
}
#endif