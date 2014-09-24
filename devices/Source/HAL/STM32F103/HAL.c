#include "../../config.h"

#ifdef STM32F10X_MD

void INIT_SYSTEM(void)
{
  SystemInit();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
}

#include "hal_eeprom_flash.c"

#ifdef ENC28J60_PHY
#include "hal_enc28j60.c"
#endif  // ENC28J60_PHY

#ifdef UART_PHY
#include "hal_uart.c"
#endif  //  UART_PHY

#ifdef EXTDIO_USED
#include "hal_dio.c"
#endif  //  EXTDIO_USED

#ifdef EXTAIN_USED
#include "hal_ain.c"
#endif  //  EXTAIN_USED

// Optimisation Level 1
void _delay_ms(uint32_t ms)
{
  ms *= (configCPU_CLOCK_HZ / 6000UL);
  while(ms > 0)
  {
    ms--;
    //asm("NOP");
  }
}

void _delay_us(uint32_t us)
{
  us *= (configCPU_CLOCK_HZ / 6000000UL);
  while(us > 0)
  {
    us--;
    //asm("NOP");
  }
}

// Hardware fault handler
void HardFault_Handler( void ) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
  for( ;; );
}
#endif  //  STM32F0XX_MD
