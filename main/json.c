#include <cJSON.h>
#include <math.h>

#include "gimbalcontroller.h"
#include "config.h"
#include "gimbal_cmd.h"
#include "infrared.h"

typedef enum {
  CMD_NONE = 0,
  CMD_SETTINGS,
  CMD_SETTINGS_WRITE,
  CMD_FACTORY_RESET,
  CMD_REBOOT,
  CMD_POWER,
  CMD_POSITIONRESET,
  CMD_PAN,
  CMD_ROLL,
  CMD_PITCH,
  CMD_MODE,
  CMD_IR_SONY,
  CMD_UNKNOWN
} json_cmd_t;

char *json_status(void)
{
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "cmd", cJSON_CreateString("STATUS"));
    cJSON_AddItemToObject(root, "connected", cJSON_CreateNumber(gs.connected));
    cJSON_AddItemToObject(root, "powerState", cJSON_CreateNumber(gs.powerState));
    cJSON_AddItemToObject(root, "batteryLevel", cJSON_CreateNumber(round(gs.batteryLevel*100.0)/100.0));
    cJSON_AddItemToObject(root, "mode", cJSON_CreateNumber(gs.mode));
    cJSON_AddItemToObject(root, "pitch", cJSON_CreateNumber(round(gs.pitch*10.0)/10.0));
    cJSON_AddItemToObject(root, "roll", cJSON_CreateNumber(round(gs.roll*10.0)/10.0));
    cJSON_AddItemToObject(root, "pan", cJSON_CreateNumber(round(gs.pan*10.0)/10.0));
    cJSON_AddItemToObject(root, "keyPressed", cJSON_CreateNumber(gs.keyPressed));
    cJSON_AddItemToObject(root, "wheelMode", cJSON_CreateNumber(gs.wheelMode));
    cJSON_AddItemToObject(root, "wheelValue", cJSON_CreateNumber(gs.wheelValue));

	char* json = cJSON_Print(root);
	cJSON_Delete(root);
    return json;
}

static char *json_settings(void) 
{
    TickType_t ws_tick = 0;
    ws_tick = xTaskGetTickCount();
    settings_t *settings=get_config();
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "cmd", cJSON_CreateString("SETTINGS"));
    cJSON_AddItemToObject(root, "version", cJSON_CreateString(VERSION));
    cJSON_AddItemToObject(root, "uptime", cJSON_CreateNumber(ws_tick/configTICK_RATE_HZ));
    cJSON_AddItemToObject(root, "gimbal_joystickmode", cJSON_CreateNumber(settings->gimbal_joystickmode));
    cJSON_AddItemToObject(root, "wifi_opmode", cJSON_CreateNumber(settings->wifi_opmode));
    cJSON_AddItemToObject(root, "wifi_ssid", cJSON_CreateString(settings->wifi_ssid));
    cJSON_AddItemToObject(root, "wifi_secret", cJSON_CreateString(settings->wifi_secret));
    cJSON_AddItemToObject(root, "wifi_hostname", cJSON_CreateString(settings->wifi_hostname));

	  char* json = cJSON_Print(root);
	  cJSON_Delete(root);
    return json;
}

static esp_err_t json_parse_and_write_settings(cJSON *json)
{
    cJSON* field;
    settings_t *s=get_config();

    if ((field = cJSON_GetObjectItem(json, "gimbal_joystickmode"))) {
        s->gimbal_joystickmode = field->valueint;
    }
    if ((field = cJSON_GetObjectItem(json, "wifi_opmode"))) {
        s->wifi_opmode = field->valueint;
    }
    if ((field = cJSON_GetObjectItem(json, "wifi_ssid"))) {
        strcpy(s->wifi_ssid, field->valuestring);
    }
    if ((field = cJSON_GetObjectItem(json, "wifi_secret"))) {
        strcpy(s->wifi_secret, field->valuestring);
    }
    if ((field = cJSON_GetObjectItem(json, "wifi_hostname"))) {
        strcpy(s->wifi_hostname, field->valuestring);
    }
    save_config();
    return ESP_OK;
}

char *json_parse(char *msg)
{
    const static char* TAG="webserver_json_parse";
    cJSON *json = cJSON_Parse(msg);
    if (!json) {
        ESP_LOGI(TAG, "Error before: [%s]\n", cJSON_GetErrorPtr());
    } else {
        cJSON* field;
        //cmd: PAN, ROLL, PITCH, MODE, POWER, IR_SONY
        //val: 
        //repeat:
        json_cmd_t cmd=CMD_NONE;

        double   value1Double=0;
        uint16_t value1Unsigned=0;
        int16_t  value1Signed=0;

        double   value2Double=0;
        uint16_t value2Unsigned=0;
        int16_t  value2Signed=0;

        int repeat=0;

        if ((field = cJSON_GetObjectItem(json, "cmd"))) {
          if(!strcasecmp(field->valuestring, "SETTINGS"))      cmd=CMD_SETTINGS;
          else if(!strcasecmp(field->valuestring, "SETTINGS_WRITE")) {cmd=CMD_SETTINGS_WRITE; json_parse_and_write_settings(json);}
          else if(!strcasecmp(field->valuestring, "FACTORY_RESET")) cmd=CMD_FACTORY_RESET;
          else if(!strcasecmp(field->valuestring, "REBOOT"))        cmd=CMD_REBOOT;
          else if(!strcasecmp(field->valuestring, "POWER"))              cmd=CMD_POWER;
          else if(!strcasecmp(field->valuestring, "POSITIONRESET")) cmd=CMD_POSITIONRESET;
          else if(!strcasecmp(field->valuestring, "PAN"))           cmd=CMD_PAN;
          else if(!strcasecmp(field->valuestring, "ROLL"))          cmd=CMD_ROLL;
          else if(!strcasecmp(field->valuestring, "PITCH"))         cmd=CMD_PITCH;
          else if(!strcasecmp(field->valuestring, "MODE"))          cmd=CMD_MODE;
          else if(!strcasecmp(field->valuestring, "IR_SONY"))       cmd=CMD_IR_SONY;
          else cmd=CMD_UNKNOWN;
        } 
        if ((field = cJSON_GetObjectItem(json, "value1"))) {
          value1Double = field->valuedouble;
          value1Unsigned = (uint16_t)field->valueint;
          value1Signed = (int16_t)field->valueint;
        } 
        if ((field = cJSON_GetObjectItem(json, "value2"))) {
          value2Double = field->valuedouble;
          value2Unsigned = (uint16_t)field->valueint;
          value2Signed = (int16_t)field->valueint;
        } 
        if ((field = cJSON_GetObjectItem(json, "repeat"))) {
          repeat = field->valueint;
        } 
//        ESP_LOGI(TAG, "Heap: %d", xPortGetFreeHeapSize());

        cJSON_Delete(json);
//        ESP_LOGI(TAG, "received: cmd: %d, value1: %d, value2: %d, repeat: %d", cmd, value1Unsigned, value2Unsigned, repeat);
        switch(cmd) {
          case CMD_SETTINGS: {
              return json_settings();
          }
          case CMD_SETTINGS_WRITE: {
              break;
          }
          case CMD_FACTORY_RESET: reset_config(); 
              break;
          case CMD_REBOOT: esp_restart(); 
              break;
          case CMD_POWER: {
            if(value1Unsigned) {
              gimbalCmd2(GIMBAL_SET_POWERUP, 0xaa55);
              gimbalCmd(GIMBAL_SET_POSITIONRESET);
              gimbalCmd2(GIMBAL_SET_MODE, 1);
            } 
            else 
              gimbalCmd2(GIMBAL_SET_POWERDOWN, 0x55aa); 
            break;
          }
          case CMD_POSITIONRESET:
            gimbalCmd2(GIMBAL_SET_POSITIONRESET, value1Unsigned);
          case CMD_MODE: 
            gimbalCmd2(GIMBAL_SET_MODE, value1Unsigned); 
            break;
          case CMD_PAN: 
            gimbalCmd3(GIMBAL_SET_PAN, value1Unsigned, repeat); 
            break;
          case CMD_ROLL: 
            gimbalCmd3(GIMBAL_SET_ROLL, value1Unsigned, repeat); 
            break;
          case CMD_PITCH: 
            gimbalCmd3(GIMBAL_SET_PITCH, value1Unsigned, repeat); 
            break;
          case CMD_IR_SONY: {
              decode_results_t parms;
              parms.address = 0;
              parms.value = value1Unsigned;
              parms.bits = value2Unsigned;
              parms.decode_type = SONY;
//              ESP_LOGI(TAG, "rmt_transmit_code: %d %d", value1Unsigned, value2Unsigned);
              rmt_transmit_code(&parms);
              break;
          }

          default: break;
        }
        return NULL;
    }
    return NULL;
}
