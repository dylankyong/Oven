/* 
 * File:   LedsTest.c
 * Author: galah
 *
 * Created on May 13, 2018, 2:54 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "Leds.h"
#include "BOARD.h"
#include <plib.h>

/*
 * 
 */
int main()
{
    BOARD_Init();

    while (1) {
        LEDS_INIT();
        int i;
        LEDS_SET(0xCC); //1100 1100
        printf("%x\n", LEDS_GET());
        for (i = 0; i < 100000000; i++);
        LEDS_SET(0xDD); //1101 1101
        printf("%x\n", LEDS_GET());
        for (i = 0; i < 100000000; i++);
        LEDS_SET(0); //0000 0000
        printf("%x\n", LEDS_GET());
        for (i = 0; i < 100000000; i++);
        LEDS_SET(0xFF); //1111 1111
        printf("%x\n", LEDS_GET());
        for (i = 0; i < 100000000; i++);
    }
    while (1);
}

