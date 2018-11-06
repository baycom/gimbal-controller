#include "gimbalcontroller.h"
#include "gimbal_cmd.h"
//#include "udp_server.h"
#include "infrared.h"
#include "ble.h"
#include "web_server.h"
#include "blink.h"
#include "config.h"
#include "wifi.h"
#include "button.h"

#define TAG "gimbalcontroller"

void button_cb(btn_action_t action) {
    switch(action) {
        case LONG_PRESS:
            reset_config();
            esp_restart();
            break;
        default:
            break;
    }
}

#if 0
extern "C" {
void app_main();
}
#endif
void app_main()
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    load_config();
    settings_t *settings=get_config();

    if(settings->wifi_opmode) {
        ESP_ERROR_CHECK(wifi_sta_init(settings));
    } else {
        ESP_ERROR_CHECK(wifi_ap_init(settings));
    }
    ESP_ERROR_CHECK(button_init(button_cb));
    ESP_ERROR_CHECK(gimbalCmd_init());
    ESP_ERROR_CHECK(rmt_tx_init());
    ESP_ERROR_CHECK(web_init());
    ESP_ERROR_CHECK(blink_init());

//    ESP_ERROR_CHECK(udp_server_init());
    
    int initdone=0;
    int counter=0;
    while(1) {
        if(xEventGroupWaitBits(ble_event_group, BLE_GATT_CONNECTED, pdFALSE, pdTRUE, pdMS_TO_TICKS(10))&BLE_GATT_CONNECTED) { 
#if 1
            if(!initdone) {
                ESP_LOGI(TAG, "initial set mode to locked"); 
                initdone=1;
                gimbalCmd2(GIMBAL_SET_MODE, 1);
                gimbalCmd2(GIMBAL_SET_JOYSTICKREVERSE, settings->gimbal_joystickmode);
            }
            if(1 || !(counter%10)) {
                gimbalCmd(GIMBAL_GET_BATTERYVOLTAGE);
                gimbalCmd(GIMBAL_GET_POWERSTATE);
                gimbalCmd(GIMBAL_GET_MODE);
            }
            gimbalCmd(GIMBAL_GET_PITCH);
            gimbalCmd(GIMBAL_GET_ROLL);
            gimbalCmd(GIMBAL_GET_PAN);
#endif            
        } else {
            gs.connected = false;
            initdone=0;
        }
        if(1 || !(counter++%4)) {
            int freeHeap=xPortGetFreeHeapSize();
            TickType_t now=xTaskGetTickCount();
            int delay=((int)(now-ws_tick))/(int)configTICK_RATE_HZ;
            ESP_LOGI(TAG, "Uptime: %d Heap: %d Delay: %d", (now/configTICK_RATE_HZ), freeHeap, delay);
#if 1
            if(freeHeap < 60000 || delay > 2 ) {
                ESP_LOGE(TAG, "low memory %d, ws_tick: %d, largest block: %d emergency restart", freeHeap, delay, heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
                ESP_LOGE(TAG, "heap_caps_check_integrity: %d", heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true));
                ESP_LOGE(TAG, "heap_caps_dump_all");
                heap_caps_dump_all();
                ((reset_vector_jump)*RESET_VECTOR_ADDRESS)();
                esp_restart();
            }
#endif
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

