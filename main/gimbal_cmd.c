#include "gimbalcontroller.h"
#include "ble.h"
#include "gimbal_cmd.h"
#include "crc16_ccitt_zero.h"
#include "blink.h"

#define TAG "gimbal_cmd"

gimbalstate_t gs;
SemaphoreHandle_t gimbalCmd_mutex;

int gimbalCmd3(int cmd, int val, int repeat)
{
    esp_err_t ret=ESP_OK;
    uint8_t buf[7];
    if(!(xEventGroupWaitBits(ble_event_group, BLE_GATT_CONNECTED, pdFALSE, pdTRUE, pdMS_TO_TICKS(100))&BLE_GATT_CONNECTED)) {
        ESP_LOGW(TAG, "BLE not connected");
        return ESP_FAIL;
    } 
    buf[0] = 6;
    buf[1] = (cmd>>8) & 0xff;
    buf[2] = cmd & 0xff;
    buf[3] = (val>>8) & 0xff;
    buf[4] = val & 0xff;
    int crc = crc16_ccit_zero(buf, 5);
    buf[5] = (crc>>8) & 0xff;
    buf[6] = crc & 0xff;

//    TickType_t start=xTaskGetTickCount();
//    esp_log_buffer_hex(TAG, cmd, sizeof(cmd));
    for(int i = 0; i < repeat; i++) {
        if(xEventGroupWaitBits(ble_event_group, BLE_GATT_WRITE_IDLE, pdTRUE, pdTRUE, pdMS_TO_TICKS(100))&BLE_GATT_WRITE_IDLE) {
            xSemaphoreTake(gimbalCmd_mutex,portMAX_DELAY);
            blink(1);
            ret=esp_ble_gattc_write_char( gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                    gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                    gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                    7,
                                    (uint8_t *)buf,
                                    ESP_GATT_WRITE_TYPE_RSP,
                                    ESP_GATT_AUTH_REQ_NONE);
            if(ret!=ESP_OK) {
                ESP_LOGE(TAG, "esp_ble_gattc_write_char failed with %02x", ret);                
                vTaskDelay(pdMS_TO_TICKS(100)); 
            } else {
                if(!(xEventGroupWaitBits(ble_event_group, BLE_GATT_WRITE_IDLE, pdFALSE, pdTRUE, pdMS_TO_TICKS(1000))&BLE_GATT_WRITE_IDLE)) {
                    ESP_LOGW(TAG, "BLE command not finished within 100ms");
                    blink(0);
                    vTaskDelay(pdMS_TO_TICKS(50)); 
                    xEventGroupSetBits(ble_event_group, BLE_GATT_WRITE_IDLE);
                    xSemaphoreGive(gimbalCmd_mutex);
                    break;
                }                                
            }
            blink(0);
            xSemaphoreGive(gimbalCmd_mutex);

        } else {
            ESP_LOGW(TAG, "BLE subsystem not idle within 100ms");
        }
    }
//    ESP_LOGI(TAG, "%d cmds in %.3fs", repeat, (double)((double)(xTaskGetTickCount()-start)/(double)configTICK_RATE_HZ));
    return ret;
}

void gimbalReceive(uint8_t *data, uint16_t len) 
{
    if(len == 7 && data[0] == 6) {
        uint16_t crc = crc16_ccit_zero(data, len-2);
        if(crc == (data[5]<<8 | data[6])) {
            uint16_t cmd = data[1]<<8 | data[2];
            uint16_t val = data[3]<<8 | data[4];
            int16_t vals = data[3]<<8 | data[4];
            switch(cmd) {
                case GIMBAL_GET_BATTERYVOLTAGE: gs.batteryLevel = (float)(val/100.0); break;
                case GIMBAL_GET_POWERSTATE: gs.powerState = val; break;
                case GIMBAL_GET_PITCH: gs.pitch = (float)(vals/100.0); break;
                case GIMBAL_GET_ROLL: gs.roll = (float)(vals/100.0); break;
                case GIMBAL_GET_PAN: gs.pan  = (float)(vals/100.0); break;
                case GIMBAL_GET_MODE: gs.mode = val; break;
                case GIMBAL_GET_KEYPRESSED: gs.keyPressed = val; break;
                default: 
                if((cmd&0xc000)!=0x8000 && !(cmd&0x1000)) {
                    ESP_LOGI(TAG, "unhandled cmd: %04x val: %04x float: %.2f", cmd, val, (float)(vals/100.0));
                }
            }
            //ESP_LOGI(TAG, "batt: %.2f, power: %X, mode: %X, pitch: %.2f, roll: %.2f, pan: %.2f", gs.batteryLevel, gs.powerState, gs.mode, gs.pitch, gs.roll, gs.pan);
        } else {
            ESP_LOGI(TAG, "crc failed: %04X", crc);
            esp_log_buffer_hex(TAG, data, len);
        }
    } else if(len == 18 && data[0]==0x24 && data[1]==0x3e && data[2]==0x0c){
        uint16_t crc = crc16_ccit_zero(data+3, len-5);
        if(crc == (data[17]<<8 | data[16])) {
            uint8_t *payload=data+3;
            gs.wheelMode = payload[7];
            gs.wheelStatus = payload[8];
            gs.wheelValue = (int8_t)payload[9];
        } else {
            ESP_LOGI(TAG, "crc failed: %04X", crc);
            esp_log_buffer_hex(TAG, data, len);
        }
    } 
    else {
        if(len == 4 && data[0]==0xc0 && data[1]==0x20 && data[2]==0x0) {
            uint16_t val=data[3]&0xff;
            gs.keyPressed = val;
        } else {
            ESP_LOGI(TAG, "unhandled notification with length: %d", len);
            esp_log_buffer_hex(TAG, data, len);
        }
    }
    gs.connected=true;
}

int gimbalCmd2(int cmd, int val) {
    return gimbalCmd3(cmd, val, 1);
}

int gimbalCmd(int cmd) {
    return gimbalCmd3(cmd, 0, 1);
}

esp_err_t gimbalCmd_init()
{
    esp_err_t ret;
    ret=ble_init();
    gimbalCmd_mutex = xSemaphoreCreateMutex();

    return ret;
}