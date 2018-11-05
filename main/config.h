#ifdef __cplusplus
extern "C" {
#endif
#ifndef CONFIG_H
#define CONFIG_H

#define member_size(type, member) sizeof(((type *)0)->member)

typedef struct {
    int32_t gimbal_joystickmode;
    int32_t wifi_opmode;
    char wifi_ssid[32];
    char wifi_secret[64];
    char wifi_hostname[64];
} settings_t;

esp_err_t reset_config();
esp_err_t save_config();
esp_err_t load_config();
settings_t *get_config();
#endif
#ifdef __cplusplus
}
#endif
