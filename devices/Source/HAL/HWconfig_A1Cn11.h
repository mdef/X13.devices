/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_A1CN11_H
#define HWCONFIG_A1CN11_H

// uNode Version 2.0
// uc ATMega328p
// Phy1: RF - CC1101

// 0 - 7    PORTA - not exist
// PORTB
// --   PB0     --      LED
// --   PB1     --      RF_IRQ
// --   PB2     --      RF_CSN
// --   PB3     ISP-4   RF_MOSI
// --   PB4     ISP-1   RF_MISO
// --   PB5     ISP-3   RF_SCK
// --   PB6     --      OSC
// --   PB7     --      OSC
// PORT C
// 16   PC0     SV1-18   Ain0
// 17   PC1     SV1-17   Ain1
// 18   PC2     SV1-16   Ain2
// 19   PC3     SV1-15   Ain3
// 20   PC4     SV1-14   SDA
// 21   PC5     SV1-13 - SCL
// --   PC6      ISP-5   RESET
// --   --      SV1-20   Ain6
// --   --      SV1-19   Ain7
// PORT D
// 24   PD0     SV1-10  RXD - On gateway busy
// 25   PD1     SV1-9   TXD - On gateway busy
// 26   PD2     SV1-8  IRQ 0 //** RF-IRQ
// 27   PD3     SV1-7  IRQ 1
// 28   PD4     SV1-6
// 29   PD5     SV1-5  PWM0
// 30   PD6     SV1-4  PWM1
// 31   PD7     SV1-3

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"
#include "../../FreeRTOS/include/queue.h"
#include "../../FreeRTOS/include/semphr.h"

#define INIT_SYSTEM()

// DIO Section
#define DIO_PORT_SIZE               8
#define EXTDIO_MAXPORT_NR           2           // Number of digital Ports
#define EXTDIO_BASE_OFFSET          2           // Numeration started from Port: 0 - A, 1 - B, 2 - C ...
#define EXTDIO_PORTNUM2PORT         {(uint16_t)&PORTC, (uint16_t)&PORTD}
#define EXTDIO_PORTNUM2MASK         {(uint8_t)0xC0, (uint8_t)0x00}
// End DIO Section

// RF Section
#define TxLEDon()                   PORTB &= ~(1<<PB0);
#define RxLEDon()                   PORTB &= ~(1<<PB0);
#define LEDsOff()                   PORTB |= (1<<PB0);

#define RF_PORT                     PORTB
#define RF_DDR                      DDRB
#define RF_PIN                      PINB
#define RF_PIN_SS                   PB2
#define RF_PIN_MOSI                 PB3
#define RF_PIN_MISO                 PB4
#define RF_PIN_SCK                  PB5

// RF IRQ
#define RF_IRQ_PORT                 PORTB
#define RF_IRQ_DDR                  DDRB
#define RF_IRQ_PIN                  PIND
#define RF_PIN_IRQ                  PB1
#define RF_IRQ_CFG()                {PCIFR = (1<<PCIF0); PCICR = (1<<PCIE0);}
#define RF_DISABLE_IRQ()            PCMSK0 = 0
#define RF_ENABLE_IRQ()             PCMSK0 = (1<<RF_PIN_IRQ)
#define RF_INT_vect                 PCINT0_vect

#define RF_PORT_INIT()              {PRR &= ~(1<<PRSPI);                      \
                                     RF_PORT = (1<<RF_PIN_SS) | (1<<PB0);     \
                                     RF_DDR = (1<<PB0) | (1<<RF_PIN_SCK) |    \
                                        (1<<RF_PIN_MOSI) | (1<<RF_PIN_SS);    \
                                     RF_IRQ_DDR &= ~(1<<RF_PIN_IRQ);          \
                                     RF_IRQ_PORT |= (1<<RF_PIN_IRQ);}
// RF SPI
#define RF_SPI_DATA                 SPDR
#define RF_SPI_BISY                 (!(SPSR &(1<<SPIF)))

#if (F_CPU > 13000000UL)
#define RF_SPI_INIT()               {SPCR = (1<<SPE) | (1<<MSTR); SPSR = 0;}            // F_CPU/4
#else   //  (F_CPU <= 13000000UL)
#define RF_SPI_INIT()               {SPCR = (1<<SPE) | (1<<MSTR); SPSR = (1<<SPI2X);}   // F_CPU/2
#endif  //  (F_CPU > 13000000UL)
//  End RF Section

#define CC11_PHY                    1
#define RF_NODE                     1

#define PHY1_ADDR_t                 uint8_t
#define RF_ADDR_t                   uint8_t
#define ADDR_BROADCAST_PHY1         (PHY1_ADDR_t)0x00
#define ADDR_UNDEF_PHY1             (PHY1_ADDR_t)0xFF
#define ADDR_DEFAULT_RF             (RF_ADDR_t)0x22
#define ADDR_UNDEF_RF               (RF_ADDR_t)0xFF

// Object's Dictionary Section
#define OD_DEV_UC_TYPE              'A'
#define OD_DEV_UC_SUBTYPE           '1'
#define OD_DEV_PHY1                 'C'
#define OD_DEV_PHY2                 'n'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '1'
#define OD_ADDR_TYPE                objUInt8

#include "../PHY/CC11/cc11_phy.h"

#define PHY1_NodeId                 objRFNodeId
#define PHY1_GateId                 objGateID
#define PHY1_Init                   CC11_Init
#define PHY1_Send                   CC11_Send
#define PHY1_Get                    CC11_Get
#define CC_ADDR                     phy1addr

#define eeprom_init_hw()
#define eeprom_read(pBuf, Addr, Len)  eeprom_read_block((void *)pBuf, (const void *)Addr, (size_t)Len)
#define eeprom_write(pBuf, Addr, Len) eeprom_write_block((const void *)pBuf, (void *)Addr, (size_t)Len)

#endif // HWCONFIG_A1CN11_H