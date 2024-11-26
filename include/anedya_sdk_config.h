/*
 * This file is part of Anedya Core SDK
 * (c) 2024, Anedya Systems Private Limited
 */

#pragma once
#include "sdkconfig.h"

// Feature management
#define ANEDYA_ENABLE_DEVICE_LOGS
#define ANEDYA_ENABLE_COMMANDS
#define ANEDYA_ENABLE_VALUESTORE
#define ANEDYA_ENABLE_DEBUG_OUTPUT

// #define ANEDYA_EMBED_PEM    // Enable this to embed certificates in PEM format
#define ANEDYA_EMBED_DER // Enable this to embed certificates in DER format

#if defined(ANEDYA_EMBED_PEM) && defined(ANEDYA_EMBED_DER)
#warning "Embedding certifciates in both PEM and DER format can increase binary size"
#endif

#if !defined(ANEDYA_EMBED_PEM) && !defined(ANEDYA_EMBED_DER)
#error "Please define either ANEDYA_EMBED_PEM or ANEDYA_EMBED_DER"
#endif

#define ANEDYA_TLS_ENABLE_ECC // Using ECC can save roughly 400 bytes of static storage
// #define ANEDYA_TLS_ENABLE_RSA

/*
This setting defines the method which is used to connect with the platform.
Supported methods are:
 - MQTT
 - HTTP

Defining both of the methods will result in compilation error.
Depending on the method selected, corresponding implementation of APIs will be implemented.

*/
#ifdef CONFIG_CONN_ANEDYA_MQTT
#define ANEDYA_CONNECTION_METHOD_MQTT
#endif
// #define ANEDYA_CONNECTION_METHOD_HTTP

// Include error definitions, omit these definitions reduces binary size by a small margin
#define ANEDYA_INCLUDE_ERR_NAMES

// Depending on your platform, anedya-core library can be tailored to use all statically assigned memory or to use dynamic memory allocation.
// If you are using static memory allocation, please define ANEDYA_ENABLE_STATIC_ALLOCATION.
// If you are using dynamic memory allocation, please define ANEDYA_ENABLE_DYNAMIC_ALLOCATION.
//
// For static allocation of memory, additional values are required to be defined.
#define ANEDYA_ENABLE_STATIC_ALLOCATION
// #define ANEDYA_ENABLE_DYNAMIC_ALLOCATION

#if defined(ANEDYA_CONNECTION_METHOD_MQTT) && defined(ANEDYA_CONNECTION_METHOD_HTTP)
#error "ANEDYA_CONNECTION_METHOD_MQTT and ANEDYA_CONNECTION_METHOD_HTTP cannot be defined at the same time"
#endif

#if !defined(ANEDYA_CONNECTION_METHOD_MQTT) && !defined(ANEDYA_CONNECTION_METHOD_HTTP)
#error "ANEDYA CONFIG: Connection methods needs to be specified"
#endif

#if defined(ANEDYA_ENABLE_STATIC_ALLOCATION) && defined(ANEDYA_ENABLE_DYNAMIC_ALLOCATION)
#error "ANEDYA_ENABLE_STATIC_ALLOCATION and ANEDYA_ENABLE_DYNAMIC_ALLOCATION cannot be defined at the same time"
#endif

#if defined(ANEDYA_TLS_ENABLE_ECC) && defined(ANEDYA_TLS_ENABLE_RSA)
#error "ANEDYA_TLS_ENABLE_ECC and ANEDYA_TLS_ENABLE_RSA cannot be defined at the same time"
#endif

// Define different buffer sizes(in bytes) to preallocate memeory to carry out different transactions
// Please provide sufficient buffer according to your requirement.
// Lower buffer will not be able to include multiple datapoint submission in single request
// Higher buffer will be able to include multiple datapoint submission in single request but will occupy higher SRAM
// Default buffer size is 1024 bytes
#ifdef ANEDYA_ENABLE_STATIC_ALLOCATION
#define ANEDYA_TX_BUFFER_SIZE CONFIG_AN_TX_BUFFER
#define ANEDYA_RX_BUFFER_SIZE CONFIG_AN_RX_BUFFER
#ifdef ANEDYA_ENABLE_DEVICE_LOGS
#define ANEDYA_LOG_BUFFER 1024  // Anedya max size of the log
#define ANEDYA_MAX_LOG_BATCH 10 // Maximum Number of logs that can be submitted in a single request
#endif
#endif

#define ANEDYA_MAX_CONCURRENT_TXN CONFIG_AN_MAX_CONCURRENT_TXN // Number of maximum under process concurrent requests to Anedya. Please note that each concurrent transaction will consume