#include "../../config.h"

#ifdef STM32F0XX_MD

void INIT_SYSTEM(void)
{
  SystemInit();
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
}

//#include "hal_eeprom_fram.c"
#include "hal_eeprom_flash.c"

#if (defined ENC28J60_PHY)
#include "hal_enc28j60.c"
#endif  //  ENC_PHY

#ifdef UART_PHY
#include "hal_uart.c"
#endif  //  UART_PHY

#ifdef EXTDIO_USED
#include "hal_dio.c"
#endif  //  EXTDIO_USED

#ifdef EXTAIN_USED
#include "hal_ain.c"
#endif  //  EXTAIN_USED

void _delay_ms(uint32_t ms)
{
  ms *= (configCPU_CLOCK_HZ / 12000UL);
  while(ms > 0)
    ms--;
}

void _delay_us(uint32_t us)
{
  us *= (configCPU_CLOCK_HZ / 12000000UL);
  while(us > 0)
    us--;
}

// Hardware fault handler
void HardFault_Handler( void ) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
  for( ;; );
}
#endif  //  STM32F0XX_MD
