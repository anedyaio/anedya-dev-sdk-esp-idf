
#include "anedya_sdk_config.h"
#include "anedya_log.h"
#include "anedya_interface.h"

void anedya_debug_log(const char *log) {
    // Call interface output API
    #ifdef ANEDYA_ENABLE_DEBUG_OUTPUT
        _anedya_interface_std_out(log);
    #endif
}