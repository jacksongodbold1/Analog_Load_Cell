// LCD library code for 16x2 LCD connected to Raspberry Pi Pico
// David Frame Mechatronics Utah Valley University 2022
#ifndef MECHLCDLIBRARY
    #define MECHLCDLIBRARY
    #include <stdio.h>
    #include "pico/stdlib.h"

    #define BLINK true
    #define NO_BLINK false
    // Pin positions in LCDpins array
    #define LCDPINB4 9
    #define LCDPINB5 8
    #define LCDPINB6 7
    #define LCDPINB7 6
    #define LCDPINRS 15
    #define LCDPINE 14
    // the following are setting a pointer to a position in an array, not pins
    #define RS 4
    #define E 5
    #define COLUMNS 16
    #define ROWS 2
    // Pin values
    #define HIGH 1
    #define LOW 0
    // LCD pin RS meaning
    #define COMMAND 0
    #define DATA 1

    #define PIN_E_SLEEP_US 40 // was 5000
    #define LCD_CLEAR_DELAY_US 10000 // defualt 10000, I didn't test to see what is nessary

    // prototypes
    void LCDinit(int bit4_pin, int bit5_pin, int bit6_pin, int bit7_pin, int rs_pin, int e_pin, int width, int depth);
    void LCDclear(void);
    void LCDprintWrapped(const char *str);
    void LCDprint(const char *str);
    void LCDgotoPos(int pos_i, int line);
    void LCDdisplayOn();
    void LCDdisplayOff();
    void LCDcursorOn(bool blink);
    void LCDclear();
    void LCDWriteStringXY(int x,int y, const char *msg);
    void LCDWriteIntXY(int x, int y, int var);
    void LCDWriteFloatXY(int x, int y, float var);

    //internal use only!!
    void sendFullByte(uint rs, uint databits[]);
    void sendRawDataOneCycle(uint raw_bits[]);
    void uintInto8bits(uint raw_bits[], uint one_byte);
    uint32_t pinValuesToMask(uint raw_bits[], int length);

#endif