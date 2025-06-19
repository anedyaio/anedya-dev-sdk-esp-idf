#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        unsigned int uart_port_num;
        void *queuehandle;
    } anedya_ext_config_t;

#ifdef __cplusplus
}
#endif