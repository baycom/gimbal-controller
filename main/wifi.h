#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIFI_H
#define WIFI_H

#define WIFI_MAXIMUM_RETRY  4
#define CONFIG_AP_STA_MAX_CONN 5

esp_err_t wifi_ap_init(settings_t *s);
esp_err_t wifi_sta_init(settings_t *s);

#endif
#ifdef __cplusplus
}
#endif
