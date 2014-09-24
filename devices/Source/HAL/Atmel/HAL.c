#include "../../config.h"

#include <avr/interrupt.h>

#ifndef PRR0
#define PRR0  PRR
#endif  //  PRR0

#ifdef UART_PHY
#include "hal_uart.c"
#endif  //  UART_PHY

#ifdef CC11_PHY
#include "hal_cc11.c"
#endif  //  CC11_PHY

#ifdef ENC28J60_PHY
#include "hal_enc28j60.c"
#endif

#ifdef EXTDIO_USED
#include "hal_dio.c"
#endif  //  EXTDIO_USED

#ifdef EXTAIN_USED
#include "hal_ain.c"
#endif  //  EXTAIN_USED

#if (configCHECK_FOR_STACK_OVERFLOW > 0)

volatile uint8_t bad_task[configMAX_TASK_NAME_LEN];

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  uint8_t pos;

  for(pos = 0; pos < configMAX_TASK_NAME_LEN; pos++)
    bad_task[pos] = pcTaskName[pos];

  for(;;);
}
#endif  //  configCHECK_FOR_STACK_OVERFLOW


//#endif  // _AVR_IO_H_
