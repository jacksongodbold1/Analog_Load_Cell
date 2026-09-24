#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "MECHws2812.h"
#include "MECHLCDLibrary.h"

#define ADC_PIN 26       
#define BUTTON_PIN 12    
#define BUZZER_PIN 13    
#define NUM_SAMPLES 32  

PIO pio;
uint sm;
uint offset;

// --- Calibration State Machine ---
typedef enum {
    CAL_IDLE,    
    CAL_ACTIVE,  
    CAL_DONE     
} CalState;

CalState cal_state = CAL_IDLE;
uint cal_step = 0;

// Arrays for calibration data
uint16_t adc_cal[8];                          
const float weight_cal[8] = {0, 1, 2, 5, 10, 20, 50, 100};  // These are the calibration samples. 

// --- ADC Reading and Averaging ---
uint16_t read_adc_average() {
    uint32_t sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += adc_read();
        sleep_us(500);
    }
    return sum / NUM_SAMPLES;
}

// --- ADC to Weight Mapping --- 
float interpolate_weight(uint16_t adc) {
    for (int i = 0; i < 7; i++) {  // 7 segments for 8 calibration points
        if (adc >= adc_cal[i] && adc <= adc_cal[i + 1]) {
            float slope = (weight_cal[i + 1] - weight_cal[i]) / 
                          (adc_cal[i + 1] - adc_cal[i]);
            return weight_cal[i] + (adc - adc_cal[i]) * slope;
        }
    }
    // Clamp values outside calibration range
    if (adc < adc_cal[0]) return weight_cal[0];
    if (adc > adc_cal[7]) return weight_cal[7];
    return -1.0f;  // Error case
}

// --- NEO Pixel Function ---
void update_led(float grams) {
    uint32_t color;

    // If calibration is not finished, keep LED blue
    if (cal_state != CAL_DONE) {
        color = urgb_u32(0, 0, 255);  // Blue (GRB: g=0, r=0, b=255)
    } else {
        // Weight-based thresholds once calibration is complete
        if (grams < 19.0f) {
            color = urgb_u32(255, 255, 0);  // Yellow (GRB: g=255, r=255, b=0)
        } else if (grams <= 23.0f) {
            color = urgb_u32(255, 0, 0);    // Red (GRB: g=0, r=255, b=0)
        } else {
            color = urgb_u32(0, 255, 0);    // Green (GRB: g=255, r=0, b=0)
        }
    }

    put_pixel(pio, sm, color);
}


// Check if button is pressed (active low)
bool button_pressed() {
    return !gpio_get(BUTTON_PIN);
}

// Beep buzzer for given duration (ms)
void beep_buzzer(uint duration_ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(duration_ms);
    gpio_put(BUZZER_PIN, 0);
}

// --- 8-Step Calibration Handler ---
void handle_calibration_step() {
    LCDclear();
    char msg[32];

    // Uniform message for all calibration steps, including 0 g
    sprintf(msg, "Place %.0fg", weight_cal[cal_step]);
    LCDWriteStringXY(0, 0, msg);
    LCDWriteStringXY(0, 1, "Press to save");

    // Wait for button press
    while (!button_pressed()) sleep_ms(10);
    sleep_ms(200);  // Debounce delay
    beep_buzzer(100);  // Confirm press

    // Save averaged ADC value
    adc_cal[cal_step] = read_adc_average();
    sprintf(msg, "Saved: %u", adc_cal[cal_step]);
    LCDclear();
    LCDWriteStringXY(0, 0, msg);
    sleep_ms(1000);

    cal_step++;
    if (cal_step >= 8) {
        // Calibration complete
        cal_state = CAL_DONE;
        LCDclear();
        LCDWriteStringXY(0, 0, "Calibration done");
        sleep_ms(1500);

        // Print calibration results to console
        for (int i = 0; i < 8; i++) {
            printf("Weight %.0f g → ADC %u\n", weight_cal[i], adc_cal[i]);
        }
    }
}


// --- Main Program ---
int main() {
    stdio_init_all();

    // Initialize ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);  // GPIO26 → ADC0

    // Initialize button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Initialize buzzer
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);

    // Pull GPIO 23 high to turn off power saving mode. This stabilizes Pico’s power supply.
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 1);

    // Initialize LCD
    LCDinit(LCDPINB4, LCDPINB5, LCDPINB6, LCDPINB7, LCDPINRS, LCDPINE, COLUMNS, ROWS);
    LCDclear();

    // Initialize WS2812 LED driver
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    sleep_ms(500);

    bool last_button_state = false;

    // Main loop
    while (true) {
        bool current_button = button_pressed();

        // Enter calibration mode on button press
        if (current_button && !last_button_state && cal_state != CAL_ACTIVE) {
            cal_state = CAL_ACTIVE;
            cal_step = 0;
            LCDclear();
            LCDWriteStringXY(0, 0, "Entering Cal Mode");
            beep_buzzer(100);
            sleep_ms(1000);
        }
        last_button_state = current_button;

        // Handle calibration or normal operation
        if (cal_state == CAL_ACTIVE && cal_step < 8) {
            handle_calibration_step();
        } else {
            uint16_t raw = read_adc_average();
            float grams = interpolate_weight(raw);

            LCDclear();
if (cal_state == CAL_DONE) {
    // Show weight if calibrated
    char line[16];
    sprintf(line, "%.1f g", grams);
    LCDWriteStringXY(0, 0, line);

    // Show number of bolts (each 4g)
    int bolts = (int)(grams / 4.0f);
    char bolts_line[16];
    sprintf(bolts_line, "%d bolts", bolts);
    LCDWriteStringXY(0, 1, bolts_line);

    update_led(grams);

    // Buzzer triggers once weight exceeds 50g
    gpio_put(BUZZER_PIN, grams > 50.0f ? 1 : 0);
} else {
    // Show message if not calibrated
    LCDWriteStringXY(0, 0, "Not Calibrated");
    update_led(0);
    gpio_put(BUZZER_PIN, 0);
}


            sleep_ms(500);
        }
    }

    return 0;
}
