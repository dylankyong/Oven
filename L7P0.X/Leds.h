/* 
 * File:   Leds.h
 * Author: Dylan Yong
 *
 * Created on May 13, 2018, 2:01 PM
 */
#ifndef LEDS_H
#define	LEDS_H


#include <xc.h>         //this works for BOARD.h but not xc.h 
#include "BOARD.h"

#define LEDS_INIT() do {    \
    TRISE = 0x00;           \
    LATE = 0x00;            \
} while (0)

#define LEDS_GET() LATE

#define LEDS_SET(x) do {    \
    LATE = (x);             \
} while(0)




#endif	/* LEDS_H */

