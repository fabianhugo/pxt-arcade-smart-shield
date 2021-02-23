#ifndef JD_USER_CONFIG_H
#define JD_USER_CONFIG_H

#define JD_CONFIG_TEMPERATURE 1

//#include "board.h"
#include "dmesg.h"

#define JD_LOG DMESG

#ifdef PSCREEN
#define PIN_LOG1 PA_12
#else
#define PIN_LOG1 PA_9
#endif

#ifdef BB_V1
#define PIN_LED PC_6
#define PIN_LOG0 PC_14
#elif defined(PROTO_V2)
#define PIN_LED PB_14
#define PIN_LOG0 PB_12
#undef PIN_LOG1
#define PIN_LOG1 PB_13
#else
#define PIN_LED PC_6
#define PIN_LOG0 PA_10
#endif

#ifndef PIN_LED_GND
#define PIN_LED_GND -1
#endif

// secondary LED
#ifndef PIN_LED2
#define PIN_LED2 -1
#endif

// logging pins for JD implementation
#ifndef PIN_LOG0
#define PIN_LOG0 -1 // sig_write
#endif
#ifndef PIN_LOG1
#define PIN_LOG1 -1
#endif
#ifndef PIN_LOG2
#define PIN_LOG2 -1 // sig error
#endif
#ifndef PIN_LOG3
#define PIN_LOG3 -1
#endif

// logging pins for application
#ifndef PIN_P0
#define PIN_P0 -1
#endif
#ifndef PIN_P1
#define PIN_P1 -1
#endif

#ifndef PIN_PWR
#define PIN_PWR -1
#endif

#endif
