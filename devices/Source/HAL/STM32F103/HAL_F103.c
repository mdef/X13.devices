#include "../../config.h"

#ifdef STM32F10X_MD

void INIT_SYSTEM(void)
{
  SystemInit();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
}

#define SIZE_OF_EEPROM  512

// Simulation
static uint8_t eeprom_buf[SIZE_OF_EEPROM];

void eeprom_init_hw(void)
{
  uint16_t pos;
  for(pos = 0; pos < SIZE_OF_EEPROM; pos++)
    eeprom_buf[pos] = 0xFF;
}

void eeprom_read(uint8_t *pBuf, uint16_t Addr, uint16_t Len)
{
  if((Addr + Len) < SIZE_OF_EEPROM)
    memcpy(pBuf, &eeprom_buf[Addr], Len);
}

void eeprom_write(uint8_t *pBuf, uint16_t Addr, uint16_t Len)
{
  if((Addr + Len) < SIZE_OF_EEPROM)
    memcpy(&eeprom_buf[Addr], pBuf, Len);
}


#ifdef ENC28J60_PHY
void enc28j60_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;
  
  // Enable Periphery Clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  
  // ENC_NSS_PIN
  GPIO_InitStructure.GPIO_Pin     = ENC_NSS_PIN;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
  GPIO_Init(ENC_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);
  
  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIe_SCK_PIN | SPIe_MISO_PIN | SPIe_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF_PP;
  GPIO_Init(SPIe_PORT, &GPIO_InitStructure);
  
  // SPIe Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIe, &SPI_InitStructure);
  // SPIe enable
  SPI_Cmd(SPIe, ENABLE);
}

uint8_t enc28j60exchg(uint8_t data)
{
  // Loop while DR register in not emplty
  //while(SPI_I2S_GetFlagStatus(SPIe, SPI_I2S_FLAG_TXE) == RESET);
  // Send byte through the SPI1 peripheral
  SPI_I2S_SendData(SPIe, data);
  // Wait to receive a byte
  while(SPI_I2S_GetFlagStatus(SPIe, SPI_I2S_FLAG_RXNE) == RESET);
  // Return the byte read from the SPI bus
  return SPI_I2S_ReceiveData(SPIe);
}
#endif  // ENC28J60_PHY

#ifdef UART_PHY
void uart_init_hw(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef UART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  // Init Port
  UART_ENABLE_CLOCK();
  // Configure USART Rx Pin as input floating
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinRX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(UART_GPIO_PORT, &GPIO_InitStructure );
  // Configure USART Tx Pin as alternate function push-pull
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinTX;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(UART_GPIO_PORT, &GPIO_InitStructure );

  // Configure UART
  UART_InitStructure.USART_BaudRate = UART_BaudRate;
  UART_InitStructure.USART_WordLength = USART_WordLength_8b;
  UART_InitStructure.USART_StopBits = USART_StopBits_1;
  UART_InitStructure.USART_Parity = USART_Parity_No;
  UART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  UART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(UARTx, &UART_InitStructure);

  // Configure NVIC
  NVIC_InitStructure.NVIC_IRQChannel = UARTx_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  // UART Enable
  USART_Cmd(UARTx, ENABLE);
  // Enable UART IRQ
  USART_ITConfig(UARTx, USART_IT_RXNE, ENABLE);
}

void UARTx_IRQHandler(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Received data is ready to be read 
  if(USART_GetITStatus(UARTx, USART_IT_RXNE) != RESET)
  {
    uart_rx_handler(&xHigherPriorityTaskWoken);
  }

   // Transmit data register empty
  if(USART_GetITStatus(UARTx, USART_IT_TXE) != RESET)
  {
    uart_tx_handler(&xHigherPriorityTaskWoken);
  }

  portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif  //  UART_PHY


#ifdef EXTDIO_USED
static const GPIO_TypeDef * pGPIOx[] = {GPIOA, GPIOB, GPIOC};

static GPIO_TypeDef * dioPortNr2GPIOx(uint8_t PortNr)
{
  if(PortNr < EXTDIO_MAXPORT_NR)
    return (GPIO_TypeDef *)pGPIOx[PortNr];

  return NULL;
}

void dioConfigPort(uint8_t PortNr, DIO_PORT_TYPE Mask, eDIOmode_t Mode)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  switch(Mode)
  {
    case DIO_MODE_IN_PD:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
      break;
    case DIO_MODE_IN_PU:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
      break;
    case DIO_MODE_OUT:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
      break;
    default:
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
      return;
  }

  GPIO_InitStructure.GPIO_Pin = Mask;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(dioPortNr2GPIOx(PortNr), &GPIO_InitStructure);
}

DIO_PORT_TYPE dioReadPort(uint8_t PortNr)
{
  return GPIO_ReadInputData(dioPortNr2GPIOx(PortNr));
}

void dioWritePort(uint8_t PortNr, DIO_PORT_TYPE Mask, bool Value)
{
  if(Value)
    GPIO_SetBits(dioPortNr2GPIOx(PortNr), Mask);
  else
    GPIO_ResetBits(dioPortNr2GPIOx(PortNr), Mask);
}
#endif  //  EXTDIO_USED


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
