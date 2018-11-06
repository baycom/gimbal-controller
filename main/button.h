#ifdef __cplusplus
extern "C" {
#endif
#ifndef BUTTON_H
#define BUTTON_H
typedef enum {
	SINGLE_CLICK,
	MANY_CLICKS,
	TOO_MANY_CLICKS,
	LONG_PRESS
} btn_action_t;
typedef void(*btn_callback_f)(btn_action_t);
esp_err_t button_init(btn_callback_f callback);
#endif
#ifdef __cplusplus
}
#endif
