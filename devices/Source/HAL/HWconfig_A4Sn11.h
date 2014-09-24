/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_A4SN11_H
#define HWCONFIG_A4SN11_H

// Arduino MEGA
// uc ATMega2560
// PHY: UART

// PORTA
// PA0  22
// PA1  23
// PA2  24
// PA3  25
// PA4  26
// PA5  27
// PA6  28
// PA7  29
// PORTB
// PB0  53      SS
// PB1  52      SCK       ICSP-3
// PB2  51      MOSI      ICSP-4
// PB3  50      MISO      ICSP-1
// PB4  10      OC2A
// PB5  11      OC1A
// PB6  12      OC1B
// PB7  13      OC0A/OC1C       LED_L
// PORTC
// PC0  37
// PC1  36
// PC2  35
// PC3  34
// PC4  33
// PC5  32
// PC6  31
// PC7  30
// PORTD
// PD0  21      SCL
// PD1  20      SDA
// PD2  19      RXD1
// PD3  18      TXD1
// PD4  --
// PD5  --
// PD6  --
// PD7  38
// PORTE
// PE0  0       RXD0
// PE1  1       TXD0
// PE2  --
// PE3  5       OC3A
// PE4  2       OC3B
// PE5  3       OC3C
// PE6  --
// PE7  --
// PORTF
// PF0  A0
// PF1  A1
// PF2  A2
// PF3  A3
// PF4  A4      JTAG-TCK
// PF5  A5      JTAG-TMS
// PF6  A6      JTAG-TDO
// PF7  A7      JTAG-TDO
// PORTG
// PG0  41
// PG1  40
// PG2  39
// PG3  --
// PG4  --
// PG5  4       OC0B
// PORTH
// PH0  17      RXD2
// PH1  16      TXD2
// PH2  --
// PH3  6       OC4A
// PH4  7       OC4B
// PH5  8       OC4C
// PH6  9       OC2B
// PH7  --
// PORTJ
// PJ0  15      RXD3
// PJ1  14      TXD3
// PJ2  --
// PJ3  --
// PJ4  --
// PJ5  --
// PJ6  --
// PJ7  --
// PORTK
// PK0  A8
// PK1  A9
// PK2  A10
// PK3  A11
// PK4  A12
// PK5  A13
// PK6  A14
// PK7  A15
// PORTL
// PL0  49
// PL1  48
// PL2  47
// PL3  46      OC5A
// PL4  45      OC5B
// PL5  44      OC5C
// PL6  43
// PL7  42



#include <avr/io.h>
#include <avr/eeprom.h>
//#include <util/delay.h>

#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"
#include "../../FreeRTOS/include/queue.h"
#include "../../FreeRTOS/include/semphr.h"

#define INIT_SYSTEM()

// DIO Section
#define DIO_PORT_SIZE               8
#define EXTDIO_MAXPORT_NR           11                                     // Number of digital Ports
#define EXTDIO_PORTNUM2PORT         {(uint16_t)&PORTA, (uint16_t)&PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD,  \
                                     (uint16_t)&PORTE, (uint16_t)&PORTF, (uint16_t)&PORTG, (uint16_t)&PORTH,  \
                                     (uint16_t)&PORTJ, (uint16_t)&PORTK, (uint16_t)&PORTL}
#define EXTDIO_PORTNUM2MASK         {(uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x70,  \
                                     (uint8_t)0xC7, (uint8_t)0x00, (uint8_t)0xD8, (uint8_t)0x84,  \
                                     (uint8_t)0xFC, (uint8_t)0x00, (uint8_t)0x00}
// End DIO Section

// UART Section
#define UART_PORT                   PORTE
#define UART_DDR                    DDRE
#define UART_RX_PIN                 PE0
#define UART_TX_PIN                 PE1
#define UART_RX_DATA                UDR0
#define UART_TX_DATA                UDR0

#define UART_TX_DISABLE_INT()       UCSR0B &= ~(1<<UDRIE0)
#define UART_TX_ENABLE_INT()        UCSR0B |= (1<<UDRIE0)
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
#define OD_DEV_UC_TYPE              'A'
#define OD_DEV_UC_SUBTYPE           '4'
#define OD_DEV_PHY1                 'S'
#define OD_DEV_PHY2                 'n'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '1'

#define OD_ADDR_TYPE                objUInt8

#include "../PHY/UART/uart_phy.h"

#define PHY1_Init                   UART_Init
#define PHY1_Send                   UART_Send
#define PHY1_Get                    UART_Get
#define PHY1_NodeId                 objRFNodeId
#define PHY1_GateId                 objGateID
#define UART_ADDR                   phy1addr
#define UART_ADDR_t                 uint8_t

#define eeprom_init_hw()
#define eeprom_read(pBuf, Addr, Len)  eeprom_read_block((void *)pBuf, (const void *)Addr, (size_t)Len)
#define eeprom_write(pBuf, Addr, Len) eeprom_write_block((const void *)pBuf, (void *)Addr, (size_t)Len)

#endif // HWCONFIG_A4SN11_H