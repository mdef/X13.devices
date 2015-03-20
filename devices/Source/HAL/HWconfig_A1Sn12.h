/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_A1SN12_H
#define HWCONFIG_A1SN12_H

// uNode Version 2.0
// uc ATMega328p
// Phy1: UART

// 0 - 7    PORTA - not exist
// PORTB
//  8   PB0     --      LED
//  9   PB1     --      OC1A
// 10   PB2     --      OC1B
// 11   PB3     ISP-4
// 12   PB4     ISP-1
// 13   PB5     ISP-3
// 14   PB6     --      OSC
// 15   PB7     --      OSC
// PORT C
// 16   PC0     SV1-18  Ain0
// 17   PC1     SV1-17  Ain1
// 18   PC2     SV1-16  Ain2
// 19   PC3     SV1-15  Ain3
// 20   PC4     SV1-14  SDA
// 21   PC5     SV1-13  SCL
// --   PC6     ISP-5   RESET
// --   --      SV1-20  Ain6
// --   --      SV1-19  Ain7
// PORT D
// 24   PD0     SV1-10  RXD
// 25   PD1     SV1-9   TXD
// 26   PD2     SV1-8
// 27   PD3     SV1-7
// 28   PD4     SV1-6
// 29   PD5     SV1-5   OC0B
// 30   PD6     SV1-4   OC0A
// 31   PD7     SV1-3

#ifdef __cplusplus
extern "C" {
#endif

#define F_CPU                       16000000UL

#include "AVR/hal.h"

// DIO Section
#define EXTDIO_USED                 1
#define EXTDIO_BASE_OFFSET          1
#define EXTDIO_MAXPORT_NR           3                                     // Number of digital Ports
#define EXTDIO_PORTNUM2PORT         {(uint16_t)&PORTB, (uint16_t)&PORTC, (uint16_t)&PORTD}
#define EXTDIO_PORTNUM2MASK         {(uint8_t)0xC0, (uint8_t)0xE0, (uint8_t)0x03}
// End DIO Section

// PWM Section
#define EXTPWM_USED                 1
#define EXTPWM_MAXPORT_NR           4
#define EXTPWM_PORT2CFG             {1, 0, 8, 9}            // bits 7-3 Timer, bits 2-0 Channel
#define EXTPWM_PORT2DIO             {29, 30, 9, 10}         // Mapping PWM channel to DIO
// End PWM Section

// Analogue Inputs
#define EXTAIN_USED                 1
#define EXTAIN_MAXPORT_NR           9           // ADC0-ADC7, Vbg
#define EXTAIN_BASE_2_APIN          {0, 1, 2, 3, 4, 5, 6, 7, 14}
#define EXTAIN_BASE_2_DIO           {16, 17, 18, 19, 20, 21, 0xFF, 0xFF, 0xFF}
#define EXTAIN_REF                  0x06        // Bit0 - Ext, Bit1 - Vcc, Bit2 - Int1, Bit3 - Int2
// End Analogue Inputs

// TWI Section
#define EXTTWI_USED                 1
#define TWIM_SCL_STAT()             (PINC & (1<<PC5))
// End TWI Section

// LEDs
#define LED1_On()                   PORTB &= ~(1<<PB0)
#define LED1_Off()                  PORTB |= (1<<PB0)
#define LEDsInit()                  {DDRB |= (1<<PB0); PORTB |= (1<<PB0);}

// UART Section
#define UART0_PORT                  PORTD
#define UART0_DDR                   DDRD
#define UART0_RX_PIN                PD0
#define UART0_TX_PIN                PD1

#define UART_PHY_PORT               0
// End UART Section

#define UART_PHY                    1

#define PHY1_ADDR_t                 uint8_t
#define ADDR_BROADCAST_PHY1         (PHY1_ADDR_t)0x00
#define ADDR_UNDEF_PHY1             (PHY1_ADDR_t)0xFF

#define RF_ADDR_t                   uint8_t
#define ADDR_UNDEF_RF               (RF_ADDR_t)0xFF
//#define ADDR_DEFAULT_RF             (RF_ADDR_t)0x04

// Object's Dictionary Section
#define OD_DEV_UC_TYPE              'A'
#define OD_DEV_UC_SUBTYPE           '1'
#define OD_DEV_PHY1                 'S'
#define OD_DEV_PHY2                 'n'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '2'

#define OD_ADDR_TYPE                objUInt8

#include "../PHY/UART/uart_phy.h"

#define PHY1_Init                   UART_Init
#define PHY1_Send                   UART_Send
#define PHY1_Get                    UART_Get
#define PHY1_GetAddr                UART_GetAddr
#define PHY1_NodeId                 objRFNodeId
#define PHY1_GateId                 objGateID

#ifdef __cplusplus
}
#endif

#endif // HWCONFIG_A1SN12_H