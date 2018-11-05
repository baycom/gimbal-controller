#ifdef __cplusplus
extern "C" {
#endif
#ifndef UDPServer_h
#define UDPServer_h

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <esp_log.h>
#include <expat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

int udp_server_init(void);

#endif /* UDPServer_h */
#ifdef __cplusplus
}
#endif