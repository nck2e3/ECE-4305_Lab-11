/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"

#define MIN_DELAY 0                    //Minimum delay in milliseconds...
#define MAX_DIGITS 3                   //Maximum digits for the flashing period...
#define F1_SCANCODE 0xFFFFFFF0         //Scancode for the F1 key...
#define DEFAULT_SPEED 500              //Default LED flashing speed...
#define LED_COUNT 16                   //Number of LEDs...

void int_to_sseg(SsegCore *sseg, int value, bool pause) {
    sseg->write_1ptn(0b10110111, 3);
    sseg->write_1ptn(0b00001100, 4);
    sseg->write_1ptn(sseg->h2s(5), 5);

    if(pause) {
        sseg->write_1ptn(0b00001100, 7);
        sseg->set_dp(2*2*2*2*2*2*2);
    } else {
        sseg->write_1ptn(0b11111111, 7);
        sseg->set_dp(0);
    }

    //Extract and display individual digits from the integer value...
    int position = 0; //Start at the rightmost display position...
    while (value >= 0 && position < 3) {
        int digit = value % 10;        //Extract the rightmost digit...
        uint8_t seg_val = sseg->h2s(digit); //Convert digit to 7-segment encoding...
        sseg->write_1ptn(seg_val, position); //Write to the current display position...
        value /= 10;                   //Remove the rightmost digit...
        position++;                    //Move to the next display position...
    }
}

void ps2_check(Ps2Core *ps2_p, GpoCore *led_p, SsegCore *sseg_p) {
    char ch;
    bool pause = false;                   //Pause flag...
    bool waiting_for_digits = false;      //Flag to track F1 input...
    char buffer[MAX_DIGITS + 1] = {0};    //Input buffer (extra space for null terminator)...
    int buffer_index = 0;                 //Index to track the buffer position...
    int value = DEFAULT_SPEED;            //Default speed value...
    int pos = 0;                          //Current LED position...
    int direction = -1;                   //Direction (-1 = left, 1 = right)...
    unsigned long last_update = now_ms(); //Time of the last LED update...

    uart.disp("\n\rPS2 Keyboard Test\n\r");
    ps2_p->init();

    while (1) {
        //Handle keyboard input...
        if (ps2_p->get_kb_ch(&ch)) {
            if (ch == F1_SCANCODE) {   //Detect F1 key press...
            waiting_for_digits = true;  //Set the flag to expect three digits...
            buffer_index = 0;           //Reset the buffer index...

            //Clear the buffer manually...
            for (int i = 0; i < sizeof(buffer); i++) {
                buffer[i] = 0;
            }

            uart.disp("\n\rF1 pressed. Enter three digits for the flashing period:\n\r");
            } else if (waiting_for_digits) {
            //Handle digit input after F1...
            if (ch >= '0' && ch <= '9') {
                if (buffer_index < MAX_DIGITS) {
                    buffer[buffer_index++] = ch;  //Add digit to buffer...
                    uart.disp(ch);               //Echo the entered digit...

                    if (buffer_index == MAX_DIGITS) {
                        //When three digits are entered, process the value...
                        int temp_value = 0;

                        //Manual conversion of buffer to integer...
                        for (int i = 0; i < MAX_DIGITS; i++) {
                        temp_value = temp_value * 10 + (buffer[i] - '0');
                        }

                        //Enforce minimum delay...
                        if (temp_value < MIN_DELAY) {
                        temp_value = MIN_DELAY;
                        uart.disp("\n\rValue below minimum delay. Setting to 50ms.\n\r");
                        }

                        value = temp_value;

                        uart.disp("\n\rNew speed: ");
                        uart.disp(value);
                        uart.disp(" ms\n\r");

                        waiting_for_digits = false;  //Reset the flag...
                    }
                }
            } else {
                uart.disp("\n\rInvalid input. Expecting three digits.\n\r");
                waiting_for_digits = false;  //Exit the digit entry mode...
            }
            } else if (ch == 'p' || ch == 'P') {  //Toggle pause...
            pause = !pause;
            uart.disp("\n\rPause toggled: ");
            uart.disp(pause ? "ON" : "OFF");
            uart.disp("\n\r");
            }
        }

        // Handle LED updates...
        if (!pause && (now_ms() - last_update >= value)) {
            //Update LEDs only when not paused and delay has elapsed...
            led_p->write(1 << pos);  //Light up the current LED...
            last_update = now_ms(); //Reset the last update time...

            //Move LED position...
            if (pos == 0) {
            direction = 1;  //Move right...
            } else if (pos == LED_COUNT - 1) {
            direction = -1; //Move left...
            }
            pos += direction;
        }

    int_to_sseg(sseg_p, value, pause);
    }
}



GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
DdfsCore ddfs(get_slot_addr(BRIDGE_BASE, S12_DDFS));
AdsrCore adsr(get_slot_addr(BRIDGE_BASE, S13_ADSR), &ddfs);

int main() {
   //uint8_t id, ;

   while (1) {
      ps2_check(&ps2, &led, &sseg);
   } //while
} //main

