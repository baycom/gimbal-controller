#ifndef GIMBALCONTROLLER_H
#define GIMBALCONTROLLER_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define VERSION "0.1"

typedef void (*reset_vector_jump)(void); 
#define RESET_VECTOR_ADDRESS ((uint32_t *) XCHAL_RESET_VECTOR_VADDR) // a pointer to uint32_t

#define DEFAULT_TASK_PRIORITY (10)
#define DEFAULT_TASK_STACKSIZE (3072)

#endif
