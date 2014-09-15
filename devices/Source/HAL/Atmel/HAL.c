#include "../../config.h"

#include <avr/interrupt.h>

#ifndef PRR0
#define PRR0  PRR
#endif  //  PRR0

#ifdef UART_PHY
#include "hal_uart.c"
#endif  //  UART_PHY

#ifdef EXTDIO_USED
#include "hal_dio.c"
#endif  //  EXTDIO_USED

//#endif  // _AVR_IO_H_
