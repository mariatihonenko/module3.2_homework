#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define LED_PIN GPIO_NUM_5

#define ADC_CHANNEL ADC_CHANNEL_3
#define WINDOW_SIZE 10

#define THRESHOLD_DARK 1200
#define THRESHOLD_LIGHT 1800

static int buffer[WINDOW_SIZE];
static int buf_idx = 0;
static int buf_count = 0;
static int sum = 0;

int apply_sma(int val) {
    if (buf_count == WINDOW_SIZE) {
        sum -= buffer[buf_idx];
    } else {
        buf_count++;
    }

    buffer[buf_idx] = val;
    sum += val;
    buf_idx = (buf_idx + 1) % WINDOW_SIZE;

    return sum / buf_count;
}

void app_main(void) {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);

    bool led_on = false;

    while (true) {
        int raw = 0;
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);

        int filtered = apply_sma(raw);

        if (filtered < THRESHOLD_DARK && !led_on) {
            led_on = true;
            gpio_set_level(LED_PIN, 1);
            printf("Dark: %d. LED ON\n", filtered);
        } 
        else if (filtered > THRESHOLD_LIGHT && led_on) {
            led_on = false;
            gpio_set_level(LED_PIN, 0);
            printf("Light: %d. LED OFF\n", filtered);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}