#include "gimbalcontroller.h"
#include "driver/gpio.h"
#include "button.h"
#include "config.h"

#define TAG "button"
#define ESP_INTR_FLAG_DEFAULT 0

typedef struct {
	uint8_t gpio_num;
	uint8_t gpio_val;
	uint32_t timestamp;
} gpio_event_t;

static QueueHandle_t gpio_evt_queue;
static uint32_t last_click = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
	uint32_t t = xTaskGetTickCount() * portTICK_PERIOD_MS;
	if (t - last_click < 80) return;
	last_click = t;
    gpio_event_t gpio_evt = {
    	.gpio_num = (uint8_t)(uint32_t)arg,
        .gpio_val = !gpio_get_level(CONFIG_BUTTON_PIN),
	.timestamp = t
    };
    xQueueSendFromISR(gpio_evt_queue, &gpio_evt, NULL);
}

static void button_watchdog_task() {
	gpio_event_t gpio_evt;
	int count = 0;
	uint32_t first_click = 0;
	uint32_t last_down = 0;

	while(1) {
		if (xQueueReceive(gpio_evt_queue, &gpio_evt, 1000)) {
//			_callback(SINGLE_CLICK);
			//20 sec to perform the action
			if(gpio_evt.gpio_val) {
				if (!first_click || gpio_evt.timestamp - first_click > 20000) {
					first_click = gpio_evt.timestamp;
					count = 0;
				}
				count++;
				last_down = gpio_evt.timestamp;
				ESP_LOGI(TAG, "click gpio[%d] [%d in sequence]",gpio_evt.gpio_num, count);
			} else {
				if (last_down && gpio_evt.timestamp - last_down > 5000) {
					ESP_LOGI(TAG, "long press gpio[%d] [%d in sequence]",gpio_evt.gpio_num, count);
					reset_config();
					esp_restart();
//					_callback(LONG_PRESS);
					first_click=0;
					last_down=0;
					count=0;
				}
			}

			//due to flickering we cannot precisely count all clicks anyway
			if (count == 10) {
//				_callback(MANY_CLICKS);
			}
			if (count >= 20) {
//				_callback(TOO_MANY_CLICKS);
				first_click=0;
				count=0;
			}
		}
	}
}

esp_err_t button_init() 
{
	gpio_evt_queue = xQueueCreate(10, sizeof(gpio_event_t));
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
	gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(CONFIG_BUTTON_PIN, GPIO_PULLUP_ONLY);
	gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_ANYEDGE);
	gpio_isr_handler_add(CONFIG_BUTTON_PIN, gpio_isr_handler, NULL);
	xTaskCreate((TaskFunction_t)button_watchdog_task, "button_watchdog_task", 1024*2, NULL, DEFAULT_TASK_PRIORITY+2, NULL);
	return ESP_OK; 
}
