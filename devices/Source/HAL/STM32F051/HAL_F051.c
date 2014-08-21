#include "../../config.h"

#ifdef STM32F0XX_MD

void INIT_SYSTEM(void)
{
  SystemInit();
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
}

void eeprom_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;

  // Enable Periphery Clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  
  // M_NSS
  GPIO_InitStructure.GPIO_Pin     = M_NSS_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_Init(M_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);

  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIm_SCK_PIN | SPIm_MISO_PIN | SPIm_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
  GPIO_Init(SPIm_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_SCK_PINSRC,  SPIm_PINS_AF);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_MISO_PINSRC, SPIm_PINS_AF);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_MOSI_PINSRC, SPIm_PINS_AF);
  
  // SPIm Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIm, &SPI_InitStructure);

  SPI_RxFIFOThresholdConfig(SPIm, SPI_RxFIFOThreshold_QF);
  // SPIm enable
  SPI_Cmd(SPIm, ENABLE);
}

//Op-code Commands 
#define FRAM_CMD_WRSR                   0x01    // Write Status Register
#define FRAM_CMD_WRITE                  0x02    // Write Memory Data
#define FRAM_CMD_READ                   0x03    // Read Memory Data
#define FRAM_CMD_WRDI                   0x04    // Write Disable
#define FRAM_CMD_RDSR                   0x05    // Read Status Register
#define FRAM_CMD_WREN                   0x06    // Set Write Enable Latch

void eeprom_read(uint8_t *pBuf, uint16_t Addr, uint16_t Len)
{
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);        // Select Chip
  SPI_SendData8(SPIm, FRAM_CMD_READ);
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr >> 8);                             // Send High Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr & 0xFF);                           // Send Low Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready

  while((SPIm->SR & SPI_I2S_FLAG_RXNE)  != (uint16_t)RESET)   // flush FIFO
    *pBuf = SPI_ReceiveData8(SPIm);

  while(Len)
  {
    Len--;
    SPI_SendData8(SPIm, 0xFF);
    while((SPIm->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
    *pBuf = SPI_ReceiveData8(SPIm);
    pBuf++;
  }
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);
}

void eeprom_write(uint8_t *pBuf, uint16_t Addr, uint16_t Len)
{
  // Write Enable Latch
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);
  SPI_SendData8(SPIm, FRAM_CMD_WREN);
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);                        // Release Chip
  // Write Header
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);                    
  SPI_SendData8(SPIm, FRAM_CMD_WRITE);                        // Send Command - Write
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr >> 8);                             // Send High Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr & 0xFF);                           // Send Low Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  // Send Data
  while(Len)
  {
    SPI_SendData8(SPIm, *pBuf);
    pBuf++;
    Len--;
    while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  // Wait until SPI is ready
  }
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);
}

#if (defined ENC28J60_PHY)
void enc28j60_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;

  // Enable Periphery Clocks
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  
  // ENC_NSS_PIN
  GPIO_InitStructure.GPIO_Pin     = ENC_NSS_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_Init(ENC_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);

  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIe_SCK_PIN | SPIe_MISO_PIN | SPIe_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
  GPIO_Init(SPIe_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_SCK_PINSRC,  SPIe_PINS_AF);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_MISO_PINSRC, SPIe_PINS_AF);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_MOSI_PINSRC, SPIe_PINS_AF);
  
  // SPIe Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // 12M
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIe, &SPI_InitStructure);

  SPI_RxFIFOThresholdConfig(SPIe, SPI_RxFIFOThreshold_QF);
  // SPIm enable
  SPI_Cmd(SPIe, ENABLE);
}

uint8_t enc28j60exchg(uint8_t data)
{
  SPI_SendData8(SPIe,data);
  while((SPIe->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  return SPI_ReceiveData8(SPIe);
}
/*
void enc28j60wr(uint8_t data)
{
  while((SPIe->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);
  SPI_SendData8(SPIe,data);
}

uint8_t enc28j60rd(void)
{
  while((SPIe->SR & SPI_I2S_FLAG_BSY));
  while(SPI_GetReceptionFIFOStatus(SPIe)) // flush FIFO
    SPI_I2S_ReceiveData16(SPIe);

  SPI_SendData8(SPIe, 0x00);
  //while((SPIe->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);
  while((SPIe->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  return SPI_ReceiveData8(SPIe);
}
*/
#endif  //  ENC_PHY

#ifdef UART_PHY
void uart_init_hw(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef UART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  // Init Port
  UART_ENABLE_CLOCK();
  GPIO_PinAFConfig(GPIOA, UART_GPIO_PinSourceRX, GPIO_AF_1);
  GPIO_PinAFConfig(GPIOA, UART_GPIO_PinSourceTX, GPIO_AF_1);
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinRX | UART_GPIO_PinTX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

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
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void uart_enable(void)
{
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
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
      GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
      break;
    case DIO_MODE_IN_PU:
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
      GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
      break;
    case DIO_MODE_OUT:
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
      GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
      break;
    default:
      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
      GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
      return;
  }

  GPIO_InitStructure.GPIO_Pin     = Mask;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
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
