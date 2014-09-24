/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef HWCONFIG_S3EN10_H
#define HWCONFIG_S3EN10_H

#include "stm32f10x.h"

#include "../../FreeRTOS/include/FreeRTOS.h"
#include "../../FreeRTOS/include/task.h"
#include "../../FreeRTOS/include/queue.h"
#include "../../FreeRTOS/include/semphr.h"

// DIO Section
#define DIO_PORT_SIZE               16
#define EXTDIO_MAXPORT_NR           3                                     // Number of digital Ports
#define EXTDIO_PORTNUM2MASK         {(uint16_t)0xF8E0, (uint16_t)0x003C, (uint16_t)0x1800}
// End DIO Section

// ENC Section
#define ENC_NSS_PORT                GPIOB
#define ENC_NSS_PIN                 GPIO_Pin_5

#define SPIe_PORT                   GPIOA
#define SPIe_SCK_PIN                GPIO_Pin_5
#define SPIe_MISO_PIN               GPIO_Pin_6
#define SPIe_MOSI_PIN               GPIO_Pin_7
#define SPIe                        SPI1

#define ENC_ENABLE_CLOCK()          RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE)
#define ENC_SELECT()                GPIO_ResetBits(ENC_NSS_PORT, ENC_NSS_PIN)
#define ENC_RELEASE()               {while((SPIe->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET); \
                                      GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);}
// End ENC Section

#define inet_addr(d,c,b,a)          (((uint32_t)a<<24) | ((uint32_t)b << 16) | ((uint32_t)c<<8)  | ((uint32_t)d))

#define ENC28J60_PHY                1
#define LAN_NODE                    1

#define PHY1_ADDR_t                 uint32_t
#define ADDR_BROADCAST_PHY1         (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define ADDR_UNDEF_PHY1             (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define ADDR_BROADCAST_LAN          (PHY1_ADDR_t)inet_addr(255,255,255,255)
#define LAN_ADDR                    phy1addr

// Object's Dictionary Section
#define OD_DEV_UC_TYPE              'S'
#define OD_DEV_UC_SUBTYPE           '3'
#define OD_DEV_PHY1                 'E'
#define OD_DEV_PHY2                 'n'
#define OD_DEV_HW_TYP_H             '1'
#define OD_DEV_HW_TYP_L             '0'
#define OD_ADDR_TYPE                objUInt32
#define OD_DEV_MAC                  {0x00,0x04,0xA3,0x00,0x00,0x03}   // MAC MSB->LSB
#define OD_DEF_IP_ADDR              inet_addr(192,168,10,202)
#define OD_DEF_IP_MASK              inet_addr(255,255,255,0)
//#define OD_DEF_IP_ROUTER            inet_addr(192,168,10,1)
//#define OD_DEF_IP_BROKER            inet_addr(192,168,20,8)

#include "../PHY/ENC28J60/enc28j60_phy.h"

#define PHY1_Init                   ENC28J60_Init
#define PHY1_Send                   ENC28J60_Send
#define PHY1_Get                    ENC28J60_Get
#define PHY1_NodeId                 objIPAddr
#define PHY1_GateId                 objIPBroker

void INIT_SYSTEM(void);

void eeprom_init_hw(void);
void eeprom_read(uint8_t *pBuf, uint32_t Addr, uint32_t Len);
void eeprom_write(uint8_t *pBuf, uint32_t Addr, uint32_t Len);

void _delay_ms(uint32_t ms);
void _delay_us(uint32_t us);

#endif  //  HWCONFIG_S3EN10_H
