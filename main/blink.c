#include "gimbalcontroller.h"
#include "driver/gpio.h"
#include "blink.h"

esp_err_t blink_init(void) {
    gpio_pad_select_gpio(CONFIG_LED_PIN);
    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

esp_err_t blink(int val) {
    return gpio_set_level(CONFIG_LED_PIN, val);
}