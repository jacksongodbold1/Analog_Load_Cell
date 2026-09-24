
#ifndef WS2812_H
    #define WS2812_H
    #include <stdio.h>
    #include <stdlib.h>

    #include "pico/stdlib.h"
    #include "hardware/pio.h"
    #include "hardware/clocks.h"
    #include "MECHws2812.pio.h"

    /**
     * NOTE:
     *  Take into consideration if your WS2812 is a RGB or RGBW variant.
     *
     *  If it is RGBW, you need to set IS_RGBW to true and provide 4 bytes per 
     *  pixel (Red, Green, Blue, White) and use urgbw_u32().
     *
     *  If it is RGB, set IS_RGBW to false and provide 3 bytes per pixel (Red,
     *  Green, Blue) and use urgb_u32().
     *
     *  When RGBW is used with urgb_u32(), the White channel will be ignored (off).
     *
     */
    #define IS_RGBW false
    #define NUM_PIXELS 2

    #ifdef PICO_DEFAULT_WS2812_PIN
    #define WS2812_PIN PICO_DEFAULT_WS2812_PIN
    #else
    // default to pin 2 if the board doesn't have a default WS2812 pin defined
    #define WS2812_PIN 16
    #endif

    // Check the pin is compatible with the platform
    #if WS2812_PIN >= NUM_BANK0_GPIOS
    #error Attempting to use a pin>=32 on a platform that does not support it
    #endif
    extern inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb);
    extern inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b); 
#endif