#include "gimbalcontroller.h"
#include "config.h"

#define STORAGE_NAMESPACE "config"
#define TAG "config"

static settings_t s;

settings_t *get_config()
{
    return &s;
}

esp_err_t reset_config(void)
{
    nvs_handle hnd;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &hnd);
    if (err != ESP_OK) return err;
    nvs_erase_all(hnd);
    nvs_commit(hnd);
    nvs_close(hnd);
    return ESP_OK;   
}

esp_err_t save_config(void)
{    
    nvs_handle hnd;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &hnd);
    if (err != ESP_OK) return err;

    ESP_ERROR_CHECK(nvs_set_i32(hnd, "joystickmode", s.gimbal_joystickmode));
    ESP_ERROR_CHECK(nvs_set_i32(hnd, "wifi_opmode", s.wifi_opmode));
    ESP_ERROR_CHECK(nvs_set_str(hnd, "wifi_ssid", s.wifi_ssid));
    ESP_ERROR_CHECK(nvs_set_str(hnd, "wifi_secret", s.wifi_secret));
    ESP_ERROR_CHECK(nvs_set_str(hnd, "wifi_hostname", s.wifi_hostname));
    ESP_ERROR_CHECK(nvs_commit(hnd));
    nvs_close(hnd);
    ESP_LOGI(TAG, "save_config: %d %d %s %s %s", s.gimbal_joystickmode, s.wifi_opmode, s.wifi_ssid, s.wifi_secret, s.wifi_hostname);
    return ESP_OK;
}

static void get_generic_name(char *name, char *base) {
	uint8_t mac[6];
	esp_efuse_mac_get_default(mac);
	sprintf(name, "%s-%02X%02X%02X%02X", base, mac[0], mac[1], mac[4], mac[5]);
    ESP_LOGI(TAG, "get_generic_name: %s", name);
}

esp_err_t load_config(void) 
{
    nvs_handle hnd;
    bool write_cfg = false;
    size_t len;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &hnd);
    if (err != ESP_OK) return err;
    if(nvs_get_i32(hnd, "joystickmode", &s.gimbal_joystickmode) != ESP_OK) { ESP_LOGI(TAG, "default: gimbal_joystickmode"); s.gimbal_joystickmode=0; write_cfg=true;}
    if(nvs_get_i32(hnd, "wifi_opmode", &s.wifi_opmode) != ESP_OK) { ESP_LOGI(TAG, "default: wifi_opmode"); s.wifi_opmode=0; write_cfg=true;}
    if(nvs_get_str(hnd, "wifi_ssid", NULL, &len) != ESP_OK && len>sizeof(member_size(settings_t, wifi_ssid))) { ESP_LOGI(TAG, "default: wifi_ssid"); get_generic_name(s.wifi_ssid, CONFIG_WIFI_SSID); write_cfg=true;} 
    else { nvs_get_str(hnd, "wifi_ssid", s.wifi_ssid, &len); };
    if(nvs_get_str(hnd, "wifi_secret", NULL, &len) != ESP_OK && len>sizeof(member_size(settings_t, wifi_secret))) { ESP_LOGI(TAG, "default: wifi_secret"); s.wifi_secret[0]=0; write_cfg=true;} 
    else { nvs_get_str(hnd, "wifi_secret", s.wifi_secret, &len); };
    if(nvs_get_str(hnd, "wifi_hostname", NULL, &len) != ESP_OK && len>sizeof(member_size(settings_t, wifi_hostname))) { ESP_LOGI(TAG, "default: wifi_hostname"); get_generic_name(s.wifi_hostname, CONFIG_WIFI_HOSTNAME); write_cfg=true;} 
    else { nvs_get_str(hnd, "wifi_hostname", s.wifi_hostname, &len); };
    nvs_close(hnd);

    ESP_LOGI(TAG, "load_config: %d %d %s %s %s", s.gimbal_joystickmode, s.wifi_opmode, s.wifi_ssid, s.wifi_secret, s.wifi_hostname);

    if(write_cfg) save_config();

    return ESP_OK;
}
