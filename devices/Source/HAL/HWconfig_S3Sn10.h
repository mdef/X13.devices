/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_S3SN10_H
#define HWCONFIG_S3SN10_H

#include "stm32f10x.h"

#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"
#include "../../FreeRTOS/include/queue.h"
#include "../../FreeRTOS/include/semphr.h"

// DIO Section
#define DIO_PORT_SIZE               16
#define EXTDIO_MAXPORT_NR           3                                     // Number of digital Ports
#define EXTDIO_PORTNUM2MASK         {(uint16_t)0xFE00, (uint16_t)0x001C, (uint16_t)0x1800}
// End DIO Section

// UART Section
#define UART_BaudRate               38400

#define UARTx                       USART1
#define UARTx_IRQn                  USART1_IRQn
#define UARTx_IRQHandler            USART1_IRQHandler
#define UART_ENABLE_CLOCK()         RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE)
#define UART_GPIO_PORT              GPIOA
#define UART_GPIO_PinTX             GPIO_Pin_9
#define UART_GPIO_PinRX             GPIO_Pin_10

#define UART_RX_DATA                (uint8_t)(UARTx->DR & 0xFF)
#define UART_TX_DATA                UARTx->DR

#define UART_TX_ENABLE_INT()        USART_ITConfig(UARTx, USART_IT_TXE, ENABLE);
#define UART_TX_DISABLE_INT()       USART_ITConfig(UARTx, USART_IT_TXE, DISABLE);
// End UART Section

#define UART_PHY                    1
#define RF_NODE                     1

#define PHY1_ADDR_t                 uint8_t
#define RF_ADDR_t                   uint8_t
#define ADDR_BROADCAST_PHY1         (PHY1_ADDR_t)0x00
#define ADDR_UNDEF_PHY1             (PHY1_ADDR_t)0xFF
#define ADDR_DEFAULT_RF             (RF_ADDR_t)0x04
#define ADDR_UNDEF_RF               (RF_ADDR_t)0xFF

// Object's Dictionary Section
#define OD_DEV_UC_TYPE              'S'
#define OD_DEV_UC_SUBTYPE           '3'
#define OD_DEV_PHY1                 'S'
#define OD_DEV_PHY2                 'n'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '0'
#define OD_ADDR_TYPE                objUInt8

#include "../PHY/UART/uart_phy.h"

#define PHY1_Init                   UART_Init
#define PHY1_Send                   UART_Send
#define PHY1_Get                    UART_Get
#define PHY1_NodeId                 objRFNodeId
#define PHY1_GateId                 objGateID
#define UART_ADDR                   phy1addr
#define UART_ADDR_t                 uint8_t

void INIT_SYSTEM(void);

void eeprom_init_hw(void);
void eeprom_read(uint8_t *pBuf, uint32_t Addr, uint32_t Len);
void eeprom_write(uint8_t *pBuf, uint32_t Addr, uint32_t Len);


#endif  //  HWCONFIG_S3SN10_H
