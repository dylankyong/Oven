// **** Include libraries here ****
// Standard libraries

//CMPE13 Support Library
#include "BOARD.h"
#include "Oled.h"
#include "OledDriver.h"
#include "Adc.h"
#include "Ascii.h"
#include "Buttons.h"
#include "Leds.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>



// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timer, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)

//oven off
#define LINE1 "|\x2\x2\x2\x2\x2|"
#define LINE2 "|     |"
#define LINE3 "|-----|"
#define LINE4 "|\x4\x4\x4\x4\x4|"

//oven on
#define LINE1ON "|\x1\x1\x1\x1\x1|"
#define LINE4ON "|\x3\x3\x3\x3\x3|"

//define the 5 Hz long press
#define LONG_PRESS 5

// **** Declare any datatypes here ****

//establishing the different states

enum {
    RESET, START, COUNTDOWN, PENDING_SELECTOR_CHANGE, PENDING_RESET, EC_I, EC_N
} state = RESET;

//oven data struct

struct OvenData {
    uint32_t cookTimeLeft;
    uint32_t initialCookTime;
    uint32_t temperature;
    int cookingMode;
    int ovenState;
    uint8_t buttonPressCount;
    int inputSelection;
};

static struct OvenData Oven1;

// **** Define any module-level, global, or external variables here ****

//static variables
static uint16_t AdcInput = 0x00;
static uint8_t butState = 0x00;
static uint8_t FRT = 0x00;
static uint8_t STFRT = 0x00;

static uint8_t HZC = 0x00;

//helper functions
void PrintOven();
void PrintLed();

// Configuration Bit settings

int main()
{
    BOARD_Init();

    // Configure Timer 1 using PBCLK as input. We configure it using a 1:256 prescalar, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR1 to F_PB / 256 / 2 yields a 0.5s timer.
    OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, BOARD_GetPBClock() / 256 / 2);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T1);
    INTSetVectorPriority(INT_TIMER_1_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_1_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T1, INT_ENABLED);

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescalar, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, BOARD_GetPBClock() / 256 / 5);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T3);
    INTSetVectorPriority(INT_TIMER_3_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T3, INT_ENABLED);

    /***************************************************************************************************
     * Your code goes in between this comment and the following one with asterisks.
     **************************************************************************************************/
    OledInit();
    AdcInit();
    LEDS_INIT();

    while (1) {
        switch (state) {
        case RESET:
            Oven1.cookTimeLeft = 0;
            //Oven1.cookingMode = 1; //0 = Bake, 1 = Toast, 2 = Broil
            Oven1.initialCookTime = 1;
            Oven1.temperature = 350;
            Oven1.ovenState = 0; //0 = off, 1 = on
            Oven1.inputSelection = 0; //0 = time, 1 = temperature
            LEDS_SET(0x00);
            OledSetDisplayNormal();
            butState = 0;
            //PrintOven();
            state = START;
            break;
        case START:
            //cooking mode toggle
            if (butState != 0 && butState & BUTTON_EVENT_3DOWN) {
                state = PENDING_SELECTOR_CHANGE;
                STFRT = FRT;
                //printf("FRT RESET\n");
                break;
            }
            //Countdown trigger
            if (butState != 0 && butState & BUTTON_EVENT_4DOWN) {
                state = COUNTDOWN;
                Oven1.cookTimeLeft = Oven1.initialCookTime;
                Oven1.ovenState = 1;
                Oven1.cookTimeLeft = Oven1.cookTimeLeft * 2;
                Oven1.initialCookTime = Oven1.initialCookTime * 2;

                TIMER_2HZ_RESET();
                HZC = FALSE;
                //PrintOven();
                //printf("2Hz RESET\n");
                break;
            }
            //PrintOven();
            if (Oven1.cookingMode == 0) { //BAKE (time variable, temp variable)
                if (AdcChanged()) {
                    //if adc changed and time is selected
                    if (Oven1.inputSelection == 0) {
                        AdcInput = AdcRead();
                        AdcInput = AdcInput >> 2;
                        AdcInput += 1;
                        Oven1.initialCookTime = AdcInput;

                    }//if adc changed and temperature is selected
                    else {
                        AdcInput = AdcRead();
                        AdcInput = AdcInput >> 2;
                        AdcInput += 300;
                        Oven1.temperature = AdcInput;
                    }
                }
            } else if (Oven1.cookingMode == 1) { //TOAST (time variable, temp N/A)
                Oven1.inputSelection = 0;
                if (AdcChanged()) {
                    AdcInput = AdcRead();
                    AdcInput = AdcInput >> 2;
                    AdcInput += 1;
                    Oven1.initialCookTime = AdcInput;
                }
            } else if (Oven1.cookingMode == 2) { //BROIL (time variable, temp 500)
                //Oven1.temperature = 500;
                Oven1.inputSelection = 0;
                if (AdcChanged()) {
                    AdcInput = AdcRead();
                    AdcInput = AdcInput >> 2;
                    AdcInput += 1;
                    Oven1.initialCookTime = AdcInput;
                }
            }
            PrintOven();
            break;

        case COUNTDOWN:
            //butState = ButtonsCheckEvents();
            if (butState & BUTTON_EVENT_4DOWN) {
                state = PENDING_RESET;
                STFRT = FRT;
            }
            if (HZC == TRUE && Oven1.cookTimeLeft > 0) {
                Oven1.cookTimeLeft--;
                //PrintOven();
                HZC = FALSE;
            } else if (HZC == TRUE && Oven1.cookTimeLeft == 0) {
                state = EC_I;
                HZC = FALSE;
            }
            PrintOven();
            PrintLed();
            break;
        case PENDING_SELECTOR_CHANGE:
            //FRT = 0;
            if ((FRT - STFRT) < LONG_PRESS && butState & BUTTON_EVENT_3UP) {
                if (Oven1.cookingMode < 2) {
                    Oven1.cookingMode++;
                } else {
                    Oven1.cookingMode = 0;
                }
                state = START;
            }
            //if(elapsed time > long press)
            if ((FRT - STFRT) >= LONG_PRESS) {
                if (Oven1.inputSelection == 0) {
                    Oven1.inputSelection = 1;
                } else {
                    Oven1.inputSelection = 0;
                }
                state = START;
            }
            PrintOven();
            break;
        case PENDING_RESET:
            //if button 4 state is up go back to countdown
            if (butState & BUTTON_EVENT_4UP) {
                state = COUNTDOWN;
                //if held longer than 1 second then go to reset
            } else if ((FRT - STFRT) >= LONG_PRESS) {
                state = RESET;
            }
            if (HZC == TRUE && Oven1.cookTimeLeft > 0) {
                Oven1.cookTimeLeft--;
                //PrintOven();
                HZC = 0;
            } else if (HZC == TRUE && Oven1.cookTimeLeft == 0) {
                state = EC_I;
                HZC = FALSE;
            }
            PrintLed();
            PrintOven();
            break;

            //EXTRA CREDIT: this part works, but only resets correctly if button 4 is long pressed
        case EC_I:
            if (butState & BUTTON_EVENT_4DOWN) {
                state = RESET;
            }
            OledSetDisplayInverted();
            if (HZC == TRUE) {
                state = EC_N;
                HZC = FALSE;
            }
            break;
        case EC_N:
            if (butState & BUTTON_EVENT_4DOWN) {
                state = RESET;
            }
            OledSetDisplayNormal();
            if (HZC == TRUE) {
                state = EC_I;
                HZC = FALSE;
            }
            break;
        }
    }
    /***************************************************************************************************
     * Your code goes in between this comment and the preceding one with asterisks
     **************************************************************************************************/
    while (1);
}

void __ISR(_TIMER_1_VECTOR, ipl4auto) TimerInterrupt2Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 4;
    HZC = TRUE; //increment clock timer
}

void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;
    FRT++; //increment long press timer

}

void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;
    butState = ButtonsCheckEvents(); //check for button event
}

void PrintOven()
{
    char oven[100];
    int minute = 0;
    int second = 0;
    int minute2 = 0;
    int second2 = 0;
    minute = Oven1.initialCookTime / 60;
    second = Oven1.initialCookTime % 60;
    minute2 = Oven1.cookTimeLeft / 120;
    second2 = Oven1.cookTimeLeft % 120;
    minute2 = (minute2 + 1) / 2;
    second2 = (second2 + 1) / 2;
    //if in off mode
    if (Oven1.ovenState == 0) {
        //if in BAKE mode and in time mode
        if (Oven1.cookingMode == 0 && Oven1.inputSelection == 0) {
            sprintf(oven, "%s   Mode: Bake\n%s  >Time: %d:%02d\n%s   Temp: %d%cF\n%s", LINE1, LINE2,
                    minute, second, LINE3, Oven1.temperature, 248, LINE4);
            //if in BAKE mode and temperature mode
        } else if (Oven1.cookingMode == 0 && Oven1.inputSelection == 1) {
            sprintf(oven, "%s   Mode: Bake\n%s   Time: %d:%02d\n%s  >Temp: %d%cF\n%s", LINE1, LINE2,
                    minute, second, LINE3, Oven1.temperature, 248, LINE4);
            //if in TOAST mode
        } else if (Oven1.cookingMode == 1) {
            sprintf(oven, "%s   Mode: Toast\n%s   Time: %d:%02d\n%s\n%s", LINE1, LINE2, minute,
                    second, LINE3, LINE4);
            //if in BROIL mode
        } else if (Oven1.cookingMode == 2) {
            sprintf(oven, "%s   Mode: Broil\n%s   Time: %d:%02d\n%s   Temp: %d%cF\n%s", LINE1, LINE2,
                    minute, second, LINE3, 500, 248, LINE4);
        }
        //if in on mode
    } else if (Oven1.ovenState == 1) {
        //if in BAKE and time mode
        if (Oven1.cookingMode == 0 && Oven1.inputSelection == 0) {
            sprintf(oven, "%s   Mode: Bake\n%s  >Time: %d:%02d\n%s   Temp: %d%cF\n%s", LINE1ON, LINE2,
                    minute2, second2, LINE3, Oven1.temperature, 248, LINE4ON);
            //if in BAKE and temperature mode
        } else if (Oven1.cookingMode == 0 && Oven1.inputSelection == 1) {
            sprintf(oven, "%s   Mode: Bake\n%s   Time: %d:%02d\n%s  >Temp: %d%cF\n%s", LINE1ON, LINE2,
                    minute2, second2, LINE3, Oven1.temperature, 248, LINE4ON);
            //if in TOAST
        } else if (Oven1.cookingMode == 1) {
            sprintf(oven, "%s   Mode: Toast\n%s   Time: %d:%02d\n%s\n%s", LINE1, LINE2, minute2,
                    second2, LINE3, LINE4ON);
            //if in BROIL 
        } else if (Oven1.cookingMode == 2) {
            sprintf(oven, "%s   Mode: Broil\n%s   Time: %d:%02d\n%s   Temp: %d%cF\n%s", LINE1ON, LINE2,
                    minute2, second2, LINE3, 500, 248, LINE4);
        }
    }
    OledClear(0);
    OledDrawString(oven);
    OledUpdate();

}

void PrintLed()
{
    //one led per 1/8th of the countdown timer
    if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 7) / 8)) {
        LEDS_SET(0xFF);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 6) / 8)) {
        LEDS_SET(0xFE);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 5) / 8)) {
        LEDS_SET(0xFC);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 4) / 8)) {
        LEDS_SET(0xF8);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 3) / 8)) {
        LEDS_SET(0xF0);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 2) / 8)) {
        LEDS_SET(0xE0);
    } else if (Oven1.cookTimeLeft > ((Oven1.initialCookTime * 1) / 8)) {
        LEDS_SET(0xC0);
    } else if (Oven1.cookTimeLeft > 0) {
        LEDS_SET(0x80);
    } else if (Oven1.cookTimeLeft == 0) {
        LEDS_SET(0x00);
    }
}