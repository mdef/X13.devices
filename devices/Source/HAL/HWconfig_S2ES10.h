/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_S2ES10_H
#define HWCONFIG_S2ES10_H

#include "stm32f0xx.h"

#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"
#include "../../FreeRTOS/include/queue.h"
#include "../../FreeRTOS/include/semphr.h"

// DIO Section
#define DIO_PORT_SIZE               16
#define EXTDIO_MAXPORT_NR           3                                     // Number of digital Ports
#define PORTNUM_2_MASK              {(uint16_t)0x60F6, (uint16_t)0xF000, (uint16_t)0x0000}
// End DIO Section

// UART Section
//#define USE_UART_2

#define UART_BaudRate               38400

#ifndef USE_UART_2
  #define UARTx                     USART1
  #define UARTx_IRQn                USART1_IRQn
  #define UARTx_IRQHandler          USART1_IRQHandler
  #define UART_ENABLE_CLOCK()       {RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE); \
                                     RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); }
  #define UART_GPIO_PinSourceRX     GPIO_PinSource9
  #define UART_GPIO_PinSourceTX     GPIO_PinSource10
  #define UART_GPIO_PinRX           GPIO_Pin_9
  #define UART_GPIO_PinTX           GPIO_Pin_10
  #define UART_BUS_FREQUENCY        USART1CLK_Frequency
#else
  #define UARTx                     USART2
  #define UARTx_IRQn                USART2_IRQn
  #define UARTx_IRQHandler          USART2_IRQHandler
  #define UART_ENABLE_CLOCK()       {RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE); \
                                     RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);}
  #define UART_GPIO_PinSourceRX     GPIO_PinSource2
  #define UART_GPIO_PinSourceTX     GPIO_PinSource3
  #define UART_GPIO_PinRX           GPIO_Pin_2
  #define UART_GPIO_PinTX           GPIO_Pin_3
  #define UART_BUS_FREQUENCY        PCLK_Frequency
#endif

#define UART_RX_DATA                (UARTx->RDR & 0xFF)
#define UART_TX_DATA                UARTx->TDR

#define UART_TX_ENABLE_INT()        USART_ITConfig(UARTx, USART_IT_TXE, ENABLE);
#define UART_TX_DISABLE_INT()       USART_ITConfig(UARTx, USART_IT_TXE, DISABLE);
// End UART Section

// FRAM Section
#define M_NSS_PORT                  GPIOA
#define M_NSS_PIN                   GPIO_Pin_4

#define SPIm_PORT                   GPIOA
#define SPIm_SCK_PIN                GPIO_Pin_5
#define SPIm_SCK_PINSRC             GPIO_PinSource5
#define SPIm_MISO_PIN               GPIO_Pin_6
#define SPIm_MISO_PINSRC            GPIO_PinSource6
#define SPIm_MOSI_PIN               GPIO_Pin_7
#define SPIm_MOSI_PINSRC            GPIO_PinSource7
#define SPIm_PINS_AF                GPIO_AF_0
#define SPIm                        SPI1
#define SPI_I2S_ReceiveData         SPI_ReceiveData8
// End FRAM Section

// ENC Section
#define ENC_NSS_PORT                GPIOB
#define ENC_NSS_PIN                 GPIO_Pin_12

#define SPIe_PORT                   GPIOB
#define SPIe_SCK_PIN                GPIO_Pin_13
#define SPIe_SCK_PINSRC             GPIO_PinSource13
#define SPIe_MISO_PIN               GPIO_Pin_14
#define SPIe_MISO_PINSRC            GPIO_PinSource14
#define SPIe_MOSI_PIN               GPIO_Pin_15
#define SPIe_MOSI_PINSRC            GPIO_PinSource15
#define SPIe_PINS_AF                GPIO_AF_0
#define SPIe                        SPI2

#define ENC_SELECT()                GPIO_ResetBits(ENC_NSS_PORT, ENC_NSS_PIN)
#define ENC_RELEASE()               {while((SPIe->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET); \
                                      GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);}
// End ENC Section

#define inet_addr(d,c,b,a)          (((uint32_t)a<<24) | ((uint32_t)b << 16) | ((uint32_t)c<<8)  | ((uint32_t)d))

#define ENC28J60_PHY                1
#define LAN_NODE                    1
#define UART_PHY                    2
#define RF_NODE                     2

#define PHY1_ADDR_t                 uint32_t
#define ADDR_BROADCAST_PHY1         (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define ADDR_UNDEF_PHY1             (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define ADDR_BROADCAST_LAN          (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define LAN_ADDR                    phy1

#define PHY2_ADDR_t                 uint8_t
#define RF_ADDR_t                   uint8_t
#define ADDR_DEFAULT_RF             (RF_ADDR_t)0x04
#define ADDR_UNDEF_RF               (RF_ADDR_t)0xFF
#define ADDR_BROADCAST_PHY2         0x00
#define UART_ADDR                   phy2
#define UART_ADDR_t                 uint8_t

// Object's Dictionary Section
#define OD_DEV_UC_TYPE              'S'
#define OD_DEV_UC_SUBTYPE           '2'
#define OD_DEV_PHY1                 'E'
#define OD_DEV_PHY2                 'S'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '0'
#define OD_ADDR_TYPE                objUInt32
#define OD_DEV_MAC                  {0x00,0x04,0xA3,0x00,0x00,0x03}   // MAC MSB->LSB
#define OD_DEF_IP_ADDR              inet_addr(192,168,178,202)
#define OD_DEF_IP_MASK              inet_addr(255,255,255,0)
//#define OD_DEF_IP_ROUTER            inet_addr(192,168,10,1)
//#define OD_DEF_IP_BROKER            inet_addr(192,168,20,8)

#include "../PHY/ENC28J60/enc28j60_phy.h"
#include "../PHY/UART/uart_phy.h"

#define PHY1_Init                   ENC28J60_Init
#define PHY1_Send                   ENC28J60_Send
#define PHY1_Get                    ENC28J60_Get
#define PHY1_NodeId                 objIPAddr
#define PHY1_GateId                 objIPBroker

#define PHY2_Init                   UART_Init
#define PHY2_Send                   UART_Send
#define PHY2_Get                    UART_Get
#define PHY2_NodeId                 objRFNodeId

void INIT_SYSTEM(void);

void eeprom_init_hw(void);
void eeprom_read(uint8_t *pBuf, uint16_t Addr, uint16_t Len);
void eeprom_write(uint8_t *pBuf, uint16_t Addr, uint16_t Len);

void _delay_ms(uint32_t ms);
void _delay_us(uint32_t us);

#endif  //  HWCONFIG_S2ES10_H
