// LCD library code for 16x2 LCD connected to Raspberry Pi Pico
// David Frame Mechatronics Utah Valley University 2022
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "MECHLCDLibrary.h"

struct LCDInstance
{
    int LCDpins[6];
    uint32_t LCDmask_c; // with clock
    uint32_t LCDmask;   // without clock
    int no_chars;
    int no_lines;
    int cursor_status[2];
} myLCD;

uint32_t pinValuesToMask(uint raw_bits[], int length)
{ // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS(, clock)
    uint32_t result = 0;
    uint pinArray[32];
    for (int i = 0; i < 32; i++)
    {
        pinArray[i] = 0;
    }
    for (int i = 0; i < length; i++)
    {
        pinArray[myLCD.LCDpins[i]] = raw_bits[i];
    }
    for (int i = 0; i < 32; i++)
    {
        result = result << 1;
        result += pinArray[31 - i];
    }
    return result;
}

void uintInto8bits(uint raw_bits[], uint one_byte)
{
    for (int i = 0; i < 8; i++)
    {
        raw_bits[7 - i] = one_byte % 2;
        one_byte = one_byte >> 1;
    }
}

void sendRawDataOneCycle(uint raw_bits[])
{ // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS
    uint32_t bit_value = pinValuesToMask(raw_bits, 5);
    gpio_put_masked(myLCD.LCDmask, bit_value);
    gpio_put(myLCD.LCDpins[E], HIGH);
    sleep_us(PIN_E_SLEEP_US);
    gpio_put(myLCD.LCDpins[E], LOW); // gpio values on other pins are pushed at the HIGH->LOW change of the clock.
    sleep_us(PIN_E_SLEEP_US);
}

void sendFullByte(uint rs, uint databits[])
{ // RS + array of Bit7, ... , Bit0
    // send upper nibble (MSN)
    uint rawbits[5];
    rawbits[4] = rs;
    for (int i = 0; i < 4; i++)
    {
        rawbits[i] = databits[i];
    }
    sendRawDataOneCycle(rawbits);
    // send lower nibble (LSN)
    for (int i = 0; i < 4; i++)
    {
        rawbits[i] = databits[i + 4];
    }
    sendRawDataOneCycle(rawbits);
}

void LCDinit(int bit4_pin, int bit5_pin, int bit6_pin, int bit7_pin, int rs_pin, int e_pin, int width, int depth)
{

    uint all_ones[6] = {1, 1, 1, 1, 1, 1};
    uint set_function_8[5] = {0, 0, 1, 1, 0};
    uint set_function_4a[5] = {0, 0, 1, 0, 0};

    uint set_function_4[8] = {0, 0, 1, 0, 0, 0, 0, 0};
    uint cursor_set[8] = {0, 0, 0, 0, 0, 1, 1, 0};
    uint display_prop_set[8] = {0, 0, 0, 0, 1, 1, 0, 0};

    myLCD.LCDpins[0] = bit7_pin;
    myLCD.LCDpins[1] = bit6_pin;
    myLCD.LCDpins[2] = bit5_pin;
    myLCD.LCDpins[3] = bit4_pin;
    myLCD.LCDpins[4] = rs_pin;
    myLCD.LCDpins[5] = e_pin;
    myLCD.no_chars = width;
    myLCD.no_lines = depth;

    // set mask, initialize masked pins and set to LOW
    myLCD.LCDmask_c = pinValuesToMask(all_ones, 6);
    myLCD.LCDmask = pinValuesToMask(all_ones, 5);
    gpio_init_mask(myLCD.LCDmask_c);          // init all LCDpins
    gpio_set_dir_out_masked(myLCD.LCDmask_c); // Set as output all LCDpins
    gpio_clr_mask(myLCD.LCDmask_c);           // LOW on all LCD pins

    // set LCD to 4-bit mode and 1 or 2 lines
    // by sending a series of Set Function commands to secure the state and set to 4 bits
    if (myLCD.no_lines == 2)
    {
        set_function_4[4] = 1;
    };
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_8);
    sendRawDataOneCycle(set_function_4a);

    // getting ready
    sendFullByte(COMMAND, set_function_4);
    sendFullByte(COMMAND, cursor_set);
    sendFullByte(COMMAND, display_prop_set);
    LCDclear();
    myLCD.cursor_status[0] = 0;
    myLCD.cursor_status[1] = 0;

}

void LCDclear()
{
    uint clear_display[8] = {0, 0, 0, 0, 0, 0, 0, 1};
    sendFullByte(COMMAND, clear_display);
    sleep_us(LCD_CLEAR_DELAY_US); // extra sleep due to equipment time needed to clear the display
}

void cursor_off()
{
    uint no_cursor[8] = {0, 0, 0, 0, 1, 1, 0, 0};
    sendFullByte(COMMAND, no_cursor);
    myLCD.cursor_status[0] = 0;
    myLCD.cursor_status[1] = 0;
}


void LCDcursorOn(bool blink)
{
    uint command_cursor[8] = {0, 0, 0, 0, 1, 1, 1, 0};
    if (blink)
        command_cursor[7] = 1;
    sendFullByte(COMMAND, command_cursor);
    myLCD.cursor_status[0] = 1;
    myLCD.cursor_status[1] = command_cursor[7];
}

void LCDdisplayOff()
{
    uint command_display[8] = {0, 0, 0, 0, 1, 0, 0, 0};
    command_display[7] = myLCD.cursor_status[1];
    command_display[6] = myLCD.cursor_status[0];
    sendFullByte(COMMAND, command_display);
}

void LCDdisplayOn()
{
    uint command_display[8] = {0, 0, 0, 0, 1, 1, 0, 0};
    command_display[7] = myLCD.cursor_status[1];
    command_display[6] = myLCD.cursor_status[0];
    sendFullByte(COMMAND, command_display);
}

void LCDgotoPos(int pos_i, int line)
{
    uint eight_bits[8];
    uint pos = (uint)pos_i;
    switch (myLCD.no_lines)
    {
    case 2:
        pos = 64 * line + pos + 0b10000000;
        break;
    case 4:
        if (line == 0 || line == 2)
        {
            pos = 64 * (line / 2) + pos + 0b10000000;
        }
        else
        {
            pos = 64 * ((line - 1) / 2) + 20 + pos + 0b10000000;
        };
        break;
    default:
        pos = pos;
    };
    uintInto8bits(eight_bits, pos);
    sendFullByte(COMMAND, eight_bits);
}

void LCDprint(const char *str)
{
    uint eight_bits[8];
    int i = 0;
    while (str[i] != 0)
    {
        uintInto8bits(eight_bits, (uint)(str[i]));
        sendFullByte(DATA, eight_bits);
        ++i;
    }
}

void LCDprintWrapped(const char *str)
{
    uint eight_bits[8];
    int line = 0;
    int charInLine = 0;
    int i = 0;

    LCDgotoPos(0, 0);

    while (str[i] != 0)
    {
        if (str[i] == 10) // if there is a new line char in the string (\n) start the next line
        {
            // this will make the program act as though the line has run out of space
            charInLine = myLCD.no_chars;
        }
        else
        {
            // if there is not a newline, print the next char
            uintInto8bits(eight_bits, (uint)(str[i]));
            sendFullByte(DATA, eight_bits);

            // update so we know the location in the line
            ++charInLine;
        }

        // if there is no room in this line, start the next one.
        if (charInLine == myLCD.no_chars)
        {
            ++ line;
            LCDgotoPos(0, line);
        }
        ++i;
    }
}

void LCDWriteStringXY(int x,int y, const char *msg) 
{
 LCDgotoPos(x,y);
 LCDprint(msg);
}

void LCDWriteIntXY(int x, int y, int var)
{
    char buffer[6]={0,0,0,0,0,0};
    sprintf(buffer,"%4.4d", var);
    LCDWriteStringXY(x,y,buffer);
}

void LCDWriteFloatXY(int x, int y, float var)
{
    char buffer[6]={0,0,0,0,0,0};
    sprintf(buffer,"%4.1f", var);
    LCDWriteStringXY(x,y,buffer);
}

// teh following is a test program to test the library.
// to run this program, you need to include the library in the same folder as this file.
int main_MECHLCDLibrary()
{
    int variable = 0;
    char buffer[33];
    stdio_init_all();
    LCDinit(LCDPINB4, LCDPINB5, LCDPINB6, LCDPINB7, LCDPINRS, LCDPINE, COLUMNS, ROWS);
    
    while(true)
    {
        LCDprint(" MECH Menu");
        LCDgotoPos(0,1);
        LCDprint("var1:");
        LCDcursorOn(true);
        LCDWriteIntXY(5,1,variable++);
        LCDWriteFloatXY(10,1,(float)variable);
        sleep_ms(5000);
        LCDclear();
        sleep_ms(1000);
    }

    return 0;
}
