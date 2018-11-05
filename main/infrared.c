#include "gimbalcontroller.h"
#include "infrared.h"

#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"


static const char* TAG = "infrared";

//CHOOSE SELF TEST OR NORMAL TEST
//#define RMT_RX_SELF_TEST   1

#if RMT_RX_SELF_TEST
#define RMT_RX_ACTIVE_LEVEL  1   /*!< Data bit is active high for self test mode */
#define RMT_TX_CARRIER_EN    0   /*!< Disable carrier for self test mode  */
#else
//Test with infrared LED, we have to enable carrier for transmitter
//When testing via IR led, the receiver waveform is usually active-low.
#define RMT_RX_ACTIVE_LEVEL  0   /*!< If we connect with a IR receiver, the data is active low */
#define RMT_TX_CARRIER_EN    1   /*!< Enable carrier for IR transmitter test with IR led */
#endif

#define RMT_TX_CHANNEL    1     /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM  5     /*!< GPIO number for transmitter signal */
#define RMT_RX_CHANNEL    0     /*!< RMT channel for receiver */
#define RMT_RX_GPIO_NUM  14     /*!< GPIO number for receiver */
#define RMT_CLK_DIV      100    /*!< RMT counter clock divider */
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

#define SONY_HEADER_HIGH_US    2400                         /*!< SONY protocol header: positive 2400us */
#define SONY_HEADER_LOW_US      600                         /*!< SONY protocol header: negative 600us*/
#define SONY_BIT_ONE_HIGH_US   1200                         /*!< SONY protocol data bit 1: positive 1200us */
#define SONY_BIT_ONE_LOW_US     600                         /*!< SONY protocol data bit 1: negative 600us */
#define SONY_BIT_ZERO_HIGH_US   600                         /*!< SONY protocol data bit 0: positive 600us */
#define SONY_BIT_ZERO_LOW_US    600                         /*!< SONY protocol data bit 0: negative 600us */
#define SONY_BIT_END            45076                       /*!< SONY protocol end: space 45ms */
#define SONY_BIT_MARGIN         100                         /*!< SONY parse margin time */

#define SONY_ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */
#define SONY_DATA_ITEM_NUM   12  /*!< SONY code item number: header + 32bit data + end */
#define rmt_item32_timeout_us  9500   /*!< RMT receiver timeout value(us) */

/*
 * @brief Build register value of waveform for SONY one data bit
 */
static inline void sony_fill_item_level(rmt_item32_t* item, int high_us, int low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}

/*
 * @brief Generate sony header value: active 9ms + negative 4.5ms
 */
static void sony_fill_item_header(rmt_item32_t* item)
{
    sony_fill_item_level(item, SONY_HEADER_HIGH_US, SONY_HEADER_LOW_US);
}

/*
 * @brief Generate sony data bit 1: positive 0.56ms + negative 1.69ms
 */
static void sony_fill_item_bit_one(rmt_item32_t* item)
{
    sony_fill_item_level(item, SONY_BIT_ONE_HIGH_US, SONY_BIT_ONE_LOW_US);
}

/*
 * @brief Generate sony data bit 0: positive 0.56ms + negative 0.56ms
 */
static void sony_fill_item_bit_zero(rmt_item32_t* item)
{
    sony_fill_item_level(item, SONY_BIT_ZERO_HIGH_US, SONY_BIT_ZERO_LOW_US);
}

/*
 * @brief Check whether duration is around target_us
 */
inline bool sony_check_in_range(int duration_ticks, int target_us, int margin_us)
{
    if(( SONY_ITEM_DURATION(duration_ticks) < (target_us + margin_us))
        && ( SONY_ITEM_DURATION(duration_ticks) > (target_us - margin_us))) {
        return true;
    } else {
        return false;
    }
}

/*
 * @brief Check whether this value represents an sony header
 */
static bool sony_header_if(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && sony_check_in_range(item->duration0, SONY_HEADER_HIGH_US, SONY_BIT_MARGIN)
        && sony_check_in_range(item->duration1, SONY_HEADER_LOW_US, SONY_BIT_MARGIN)) {
        return true;
    }
    return false;
}

/*
 * @brief Check whether this value represents an sony data bit 1
 */
static bool sony_bit_one_if(rmt_item32_t* item, bool no_check_d1)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && sony_check_in_range(item->duration0, SONY_BIT_ONE_HIGH_US, SONY_BIT_MARGIN)
        && (no_check_d1 || sony_check_in_range(item->duration1, SONY_BIT_ONE_LOW_US, SONY_BIT_MARGIN))) {
        return true;
    }
    return false;
}

/*
 * @brief Check whether this value represents an sony data bit 0
 */
static bool sony_bit_zero_if(rmt_item32_t* item, bool no_check_d1)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && sony_check_in_range(item->duration0, SONY_BIT_ZERO_HIGH_US, SONY_BIT_MARGIN)
        && (no_check_d1 || sony_check_in_range(item->duration1, SONY_BIT_ZERO_LOW_US, SONY_BIT_MARGIN))) {
        return true;
    }
    return false;
}

static void debug_item(rmt_item32_t* item)
{
    ESP_LOGD(TAG, "item: %p l0:%d d0:%d l1:%d d1:%d", item, item->level0, item->duration0, item->level1, item->duration1);
}

static int sony_parse_items(rmt_item32_t* item, int item_num, decode_results_t *result)
{
    int w_len = item_num;
    if(w_len < (SONY_DATA_ITEM_NUM+1)) {
//        ESP_LOGI(TAG, "w_len: %d", w_len);
        return -1;
    }
    int i = 0, j = 0;
    if(!sony_header_if(item++)) {
        ESP_LOGI(TAG, "header failed");
        return 1;
    }
    uint32_t data_t = 0;
    for(j = 0; j < (w_len-1); j++) {
        bool last=j==(w_len-2);
        //debug_item(item);

        if(sony_bit_one_if(item, last)) {
            //ESP_LOGI(TAG, "item num: %d bit: 1", j);
            data_t = (data_t<<1)|1;
        } else if(sony_bit_zero_if(item, last)) {
            //ESP_LOGI(TAG, "item num: %d bit: 0", j);
            data_t = data_t<<1;
        } else {
            ESP_LOGI(TAG, "something is wrong");
            break;
        }
        item++;
        i++;
    }
    if(i==12 || i==15 || i==20) {
        result->value=0;
        for(int j=0;j<i;j++) {
            int bit = ((data_t>>j) & 1);
            int position = i-j-1;
            result->value |= bit<<position;
            //ESP_LOGI(TAG, "bit: %d, pos: %d", bit, position);
        }
        result->bits = i;
        result->decode_type = SONY;
    } else { 
        result->decode_type = UNKNOWN;
        result->value = 0;
        result->address = 0;
        result->bits = 0;
    }
    return i+1;
}

/*
 * @brief Build sony 32bit waveform.
 */
static int sony_build_items(int channel, rmt_item32_t* item, int item_num, uint32_t cmd_data)
{
    int i = 0, j = 0;
    if(item_num < SONY_DATA_ITEM_NUM || item_num > 20) {
        return -1;
    }
//    ESP_LOGI(TAG, "sony_build_items: %p %d %08x", item, item_num, cmd_data);
    for(int repeat=0; repeat < 5; repeat++) {
        sony_fill_item_header(item++);
        i++;
        for(j = 0; j < item_num; j++) {
            if(cmd_data & 0x1) {
                sony_fill_item_bit_one(item);
            } else {
                sony_fill_item_bit_zero(item);
            }
            item++;
            i++;
            cmd_data >>= 1;
        }
//    sony_fill_item_end(item);
//    i++;
    }
    return i;
}

/*
 * @brief RMT transmitter initialization
 */
esp_err_t rmt_tx_init()
{
    rmt_config_t rmt_tx;
    rmt_tx.channel = (rmt_channel_t)RMT_TX_CHANNEL;
    rmt_tx.gpio_num = (gpio_num_t)RMT_TX_GPIO_NUM;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = RMT_CLK_DIV;
    rmt_tx.tx_config.loop_en = false;
    rmt_tx.tx_config.carrier_duty_percent = 50;
    rmt_tx.tx_config.carrier_freq_hz = 38000;
    rmt_tx.tx_config.carrier_level = (rmt_carrier_level_t)1;
    rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_EN;
    rmt_tx.tx_config.idle_level = (rmt_idle_level_t)0;
    rmt_tx.tx_config.idle_output_en = true;
    rmt_tx.rmt_mode = (rmt_mode_t)0;
    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
    return ESP_OK;
}

/*
 * @brief RMT receiver initialization
 */
esp_err_t rmt_rx_init()
{
    rmt_config_t rmt_rx;
    rmt_rx.channel = (rmt_channel_t)RMT_RX_CHANNEL;
    rmt_rx.gpio_num = (gpio_num_t)RMT_RX_GPIO_NUM;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = true;
    rmt_rx.rx_config.filter_ticks_thresh = 100;
    rmt_rx.rx_config.idle_threshold = rmt_item32_timeout_us / 10 * (RMT_TICK_10_US);
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
    return ESP_OK;
}

/**
 * @brief RMT receiver demo, this task will print each received sony data.
 *
 */
static void rmt_example_sony_rx_task(void *p)
{
    rmt_channel_t channel = (rmt_channel_t)RMT_RX_CHANNEL;
    rmt_rx_init();
    RingbufHandle_t rb = NULL;
    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(channel, &rb);
    rmt_rx_start(channel, 1);
    while(rb) {
        size_t rx_size = 0;
        //try to receive data from ringbuffer.
        //RMT driver will push all the data it receives to its ringbuffer.
        //We just need to parse the value and return the spaces of ringbuffer.
        rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 1000);
        int len=rx_size/sizeof(rmt_item32_t);
 //       ESP_LOGI(TAG, "got %p items: %d %d", item, rx_size, len);
#if 0
        for(int i=0; i<len; i++) {
            debug_item(item+i);
        }
#endif
        if(item) {
            decode_results_t rmt_cmd;
            int offset = 0;
            while(1) {
                //parse data value from ringbuffer.
                int res = sony_parse_items(item + offset, len, &rmt_cmd);
                if(res > 1) {
                    ESP_LOGI(TAG, "RMT RCV --- type: %02x len: %d cmd: 0x%08x", (unsigned int)rmt_cmd.decode_type, rmt_cmd.bits, rmt_cmd.value);
                } else if (res < 1) {
                    break;
                }
                len -= res;
                offset += res*sizeof(rmt_item32_t);
            }
            ESP_LOGI(TAG, "vRingbufferReturnItem %p", item);
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(rb, (void*) item);
        } else {
            //break;
        }
    }
    vTaskDelete(NULL);
}

int rmt_transmit_code(decode_results_t *parms)
{
        rmt_channel_t channel = (rmt_channel_t)RMT_TX_CHANNEL;
        size_t size = sizeof(rmt_item32_t)*200;
        rmt_item32_t* items = (rmt_item32_t*) malloc(size);
        memset((void*) items, 0, size);
        int num_items=-1;

        switch(parms->decode_type) {
            case SONY: num_items = sony_build_items(channel, items, parms->bits, parms->value | parms->address);
                break;
            default:
                break;
        }
        rmt_write_items(channel, items, num_items, true);
        rmt_wait_tx_done(channel, portMAX_DELAY);
        free(items);
        return num_items;
}
