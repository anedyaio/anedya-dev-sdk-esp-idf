#ifndef _TIMEMANAGEMENT_H_
#define _TIMEMANAGEMENT_H_

#include "freertos/FreeRTOS.h"
#include "sync.h"


void syncTime_task(void *pvParameters);

#endif // !_TIMEMANAGEMENT_H_

