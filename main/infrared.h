#ifdef __cplusplus
extern "C" {
#endif
#ifndef INFRARED_H
#define INFRARED_H

#define SONY_TC_RESET	0x00003d7d
#define SONY_DATA_CODE	0x00003d33
#define SONY_SCAN_SLOW_BACK	0x00022d22
#define SONY_SCAN_SLOW_FORWARD	0x00022d23
#define SONY_SKIP_BACK	0x00022d30
#define SONY_SKIP_FORWARD	0x00022d31
#define SONY_PLAY	0x00022d32
#define SONY_PAUSE	0x00022d39
#define SONY_STOP	0x00022d38
#define SONY_UP	0x00022d79
#define SONY_DOWN	0x00022d7a
#define SONY_LEFT	0x00022d7b
#define SONY_RIGHT	0x00022d7c
#define SONY_ENTER	0x00022d0b
#define SONY_DISPLAY	0x00022d54
#define SONY_MODE	0x00022d1b
#define SONY_REC	0x00003d30
#define SONY_TELE	0x00006c9a
#define SONY_WIDE	0x00006c9b

typedef
	enum {
		UNKNOWN      = -1,
		UNUSED       =  0,
		RC5,
		RC6,
		NEC,
		SONY,
		PANASONIC,
		JVC,
		SAMSUNG,
		WHYNTER,
		AIWA_RC_T501,
		LG,
		SANYO,
		MITSUBISHI,
		DISH,
		SHARP,
		DENON,
		PRONTO,
		LEGO_PF,
	}
decode_type_t;

typedef struct 
{
		decode_type_t          decode_type;  // UNKNOWN, NEC, SONY, RC5, ...
		uint32_t               address;      // Used by Panasonic & Sharp [16-bits]
		uint32_t               value;        // Decoded value [max 32-bits]
		int                    bits;         // Number of bits in decoded value
} decode_results_t;

#define REPEAT 0xFFFFFFFF

esp_err_t rmt_tx_init();
esp_err_t rmt_rx_init();
int rmt_transmit_code(decode_results_t *parms);
#endif
#ifdef __cplusplus
}
#endif
