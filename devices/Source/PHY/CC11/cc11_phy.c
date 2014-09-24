/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

// CC11xx RF interface

#include "../../config.h"

#ifdef CC11_PHY

#include "cc11_reg.h"

#define RF_WAIT_LOW_MISO()      while(RF_PIN & (1<<RF_PIN_MISO))
#define RF_SELECT()             RF_PORT &= ~(1<<RF_PIN_SS)
#define RF_RELEASE()            RF_PORT |= (1<<RF_PIN_SS)

#define CC11_RF_POWER           0x50

typedef enum
{
  CC11_STATE_IDLE = 0,

  CC11_STATE_RXIDLE,
  CC11_STATE_RXDATA,
  
  CC11_STATE_TXHDR,
  CC11_STATE_TXDATA
}CC11_STATE_e;

static const uint8_t cc11config[][2] =
{
  {CC11_IOCFG2,   CC11_GDO_DISABLE},    // GDO2 - High impedance (3-State)
  {CC11_IOCFG0,   CC11_GDO_SYNC},       // GDO0 - Asserts when sync word has been sent/received, and de-asserts at the end of the packet.
  {CC11_FIFOTHR,  0x47},                // ADC_RETENTION, RX Attenuation: 0 dB, FIFO Threshold 33/32 bytes 
  {CC11_PKTLEN,   0x3D},                // default packet length 61 byte
  {CC11_PKTCTRL1, 0x06},                // Append Status, Check Address and Broadcast
  {CC11_PKTCTRL0, 0x05},                // CRC calculation: enable, variable packet length
  {CC11_FSCTRL1,  0x06},                // IF = 152,3 kHz
  {CC11_FREQ2,    CC11_DEFVAL_FREQ2},   // Set default carrier frequency
  {CC11_FREQ1,    CC11_DEFVAL_FREQ1},
  {CC11_FREQ0,    CC11_DEFVAL_FREQ0},
  {CC11_MDMCFG4,  0xCA},                // RX filter BW 101,6 kHz
  {CC11_MDMCFG3,  0x83},                // Data Rate = 38,383 kBaud
  {CC11_MDMCFG2,  0x13},                // Sensitivity optimized, GFSK, sync word 30/32 bit detected
  {CC11_DEVIATN,  0x35},                // Deviation 20 kHz
  {CC11_MCSM0,    0x18},                // Automatically calibrate when going from IDLE to RX or TX,  PO_TIMEOUT: 150uS
  {CC11_FOCCFG,   0x16},                // Frequency offset compensation 67,5 kHz
  {CC11_AGCCTRL2, 0x43},                // The highest gain setting can not be used, Target amplitude from channel filter: 33 dB 
  {CC11_WORCTRL,  0xFB},
  {CC11_FSCAL3,   0xE9},
  {CC11_FSCAL2,   0x2A},
  {CC11_FSCAL1,   0x00},
  {CC11_FSCAL0,   0x1F},
  {CC11_TEST2,    0x81},
  {CC11_TEST1,    0x35},
  {CC11_TEST0,    0x09}
};

static QueueHandle_t  cc11_in_queue = NULL;
static QueueHandle_t  cc11_out_queue;

static uint8_t        cc11s_Channel;
static uint16_t       cc11s_Group;
static uint8_t        cc11s_NodeID;

static volatile CC11_STATE_e cc11v_State = CC11_STATE_IDLE;

extern uint8_t        hal_cc11_spiExch(uint8_t data);

// Send command strobe to the CC1101 IC via SPI
static void cc11_cmdStrobe(uint8_t cmd) 
{
  RF_SELECT();                          // Select CC1101
  RF_WAIT_LOW_MISO();                   // Wait until MISO goes low
  hal_cc11_spiExch(cmd);
  RF_RELEASE();                         // Release CC1101
}

// Write single register into the CC1101 IC via SPI
static void cc11_writeReg(uint8_t Addr, uint8_t value) 
{
  RF_SELECT();                          // Select CC1101
  RF_WAIT_LOW_MISO();                   // Wait until MISO goes low
  hal_cc11_spiExch(Addr);               // Send register address
  hal_cc11_spiExch(value);              // Send value
  RF_RELEASE();                         // Release CC1101
}

// Read single CC1101 register via SPI
static uint8_t cc11_readReg(uint8_t Addr)
{
  uint8_t retval;

  RF_SELECT();                          // Select CC1101
  RF_WAIT_LOW_MISO();                   // Wait until MISO goes low
  hal_cc11_spiExch(Addr);               // Send register address
  // Read result
  retval = hal_cc11_spiExch(0);
  RF_RELEASE();                         // Release CC1101
  return retval;
}


static bool cc11_can_send(void)
{
  if((cc11v_State == CC11_STATE_RXIDLE) &&                                          // State is RxIdle
     (cc11_readReg(CC11_PKTSTATUS | CC11_STATUS_REGISTER) & CC11_PKTSTATUS_CCA))    // Channel Clear
  {
    TxLEDon();
    cc11_cmdStrobe(CC11_SIDLE);         // Switch to Idle state
    cc11_cmdStrobe(CC11_SFTX);          // Flush the TX FIFO buffer
    cc11v_State = CC11_STATE_TXHDR;
    return true;
  }

  return false;
}

static void cc11_send(MQ_t *pSBuf)
{
  // Fill Buffer
  uint8_t i, len;

  len = pSBuf->Length;
  // Send burst
  RF_SELECT();                                    // Select CC1101
  RF_WAIT_LOW_MISO();                             // Wait until MISO goes low
  hal_cc11_spiExch(CC11_BIT_BURST | CC11_TXFIFO);
  hal_cc11_spiExch(len + 2);                      // Set data length at the first position of the TX FIFO
  hal_cc11_spiExch(*pSBuf->CC_ADDR);              // Send destination address
  hal_cc11_spiExch(cc11s_NodeID);                 // Send Source address
  for(i = 0; i < len; i++)                        // Send Payload
    hal_cc11_spiExch(pSBuf->raw[i]);
  RF_RELEASE();                                   // Release CC1101

  vPortFree(pSBuf);

  // CCA enabled: will enter TX state only if the channel is clear
  cc11_cmdStrobe(CC11_STX);
}


/////////////////////////////////////////////////
// API

void CC11_Init(void)
{
  // HW Initialise
  RF_DISABLE_IRQ();
  RF_PORT_INIT();                       // Ports Init
  RF_SPI_INIT();                        // init SPI controller
  RF_IRQ_CFG();                         // init IRQ input
  // HW End
  // Reset CC1101
  _delay_us(5);
  RF_SELECT();
  _delay_us(10);
  RF_RELEASE();
  _delay_us(40);
  RF_SELECT();
  RF_WAIT_LOW_MISO();                   // Wait until MISO goes low
  hal_cc11_spiExch(CC11_SRES);
  RF_WAIT_LOW_MISO();                   // Wait until MISO goes low
  RF_RELEASE();

  // Configure CC1101
  uint8_t i;
  for (i=0; i<(sizeof(cc11config)/sizeof(cc11config[0])); i++)
    cc11_writeReg(cc11config[i][0], cc11config[i][1]);
    
  // Load Group ID(Synchro)
  uint8_t Len = sizeof(cc11s_Group);
  ReadOD(objRFGroup, MQTTSN_FL_TOPICID_PREDEF,  &Len, (uint8_t *)&cc11s_Group);
  cc11_writeReg(CC11_SYNC1, cc11s_Group>>8);
  cc11_writeReg(CC11_SYNC0, cc11s_Group & 0xFF);
  // Load Device ID
  Len = sizeof(cc11s_NodeID);
  ReadOD(objRFNodeId, MQTTSN_FL_TOPICID_PREDEF,  &Len, &cc11s_NodeID);
  cc11_writeReg(CC11_ADDR, cc11s_NodeID);
  // Load Frequency channel
  ReadOD(objRFChannel, MQTTSN_FL_TOPICID_PREDEF, &Len, &cc11s_Channel);
  cc11s_Channel >>= 3;                             // !!!  Channel Space = 200 kHz not 25
  cc11_writeReg(CC11_CHANNR, cc11s_Channel);
    
  // Configure PATABLE, No Ramp
  cc11_writeReg(CC11_PATABLE, CC11_RF_POWER);

  // Init Internal variables
  cc11v_State = CC11_STATE_IDLE;
//  cc11v_pRxBuf = NULL;

  // Create Task
  if(cc11_in_queue == NULL)
  {
    cc11_in_queue  = xQueueCreate(4, sizeof(void *));
    cc11_out_queue = xQueueCreate(4, sizeof(void *));
  }
  else
  {
    xQueueReset(cc11_in_queue);
    xQueueReset(cc11_out_queue);
  }
  
  RF_ENABLE_IRQ();                      // configure interrupt controller
}

void CC11_Send(void *pBuf)
{
  if(cc11_can_send())
    cc11_send(pBuf);
  else if(xQueueSend(cc11_in_queue, &pBuf, 1) != pdTRUE)
    vPortFree(pBuf);
}

void CC11_Get(void *pBuf)
{
  while(xQueueReceive(cc11_out_queue, pBuf, 0) != pdTRUE)
  {
    if(cc11v_State == CC11_STATE_RXIDLE)
    {
      if((uxQueueMessagesWaiting(cc11_in_queue) != 0) &&    // input queue is not empty
          cc11_can_send())                                  // can send
      {
        MQ_t * pSBuf;
        if(xQueueReceive(cc11_in_queue, &pSBuf, 0) == pdTRUE)
          cc11_send(pSBuf);
      }
    }
    else if(cc11v_State == CC11_STATE_IDLE)
    {
      cc11_cmdStrobe(CC11_SIDLE);       // Enter to the IDLE state
      cc11_cmdStrobe(CC11_SFTX);
      cc11_cmdStrobe(CC11_SFRX);
      cc11_cmdStrobe(CC11_SRX);         // Enter to RX State
      cc11v_State = CC11_STATE_RXIDLE;
    }

    taskYIELD();
  }
}

void CC11_IRQ_Handler(BaseType_t * pxHigherPriorityTaskWoken)
{
  uint8_t marcs = cc11_readReg(CC11_MARCSTATE | CC11_STATUS_REGISTER);
    
  switch(cc11v_State)
  {
    case CC11_STATE_RXIDLE:
      if(marcs != CC11_MARCSTATE_RX)  // Synchro received
        break;

      RxLEDon();
      cc11v_State = CC11_STATE_RXDATA;
      return;
    case CC11_STATE_RXDATA:                  // Data received
      if(marcs == CC11_MARCSTATE_IDLE)
      {
        // read number of bytes in receive FIFO
        // Due a chip bug, the RXBYTES register must read the same value twice in a row to guarantee an accurate value.
        uint8_t frameLen, tmp, i;
        frameLen = cc11_readReg(CC11_RXBYTES | CC11_STATUS_REGISTER);
        do
        {
          tmp = frameLen;
          frameLen = cc11_readReg(CC11_RXBYTES | CC11_STATUS_REGISTER);
        }while (tmp != frameLen);

        if(((tmp & 0x7F) < 7) ||                        // Packet is too small
            (tmp & 0x80) ||                             // or Overflow
            (tmp > (MQTTSN_MSG_SIZE + 3)))              // or Packet is too Big
          break;

        MQ_t * pTmp;
        pTmp = (MQ_t *)pvPortMalloc(sizeof(MQ_t));
        if(pTmp == NULL)                                // No Memory
          break;

        frameLen -= 5;
        pTmp->Length = frameLen;

        // Read Burst
        RF_SELECT();                                    // Select CC1101
        RF_WAIT_LOW_MISO();                             // Wait until MISO goes low
        hal_cc11_spiExch(CC11_BIT_READ | CC11_BIT_BURST | CC11_RXFIFO);
        hal_cc11_spiExch(0);                            // Read Length
        hal_cc11_spiExch(0);                            // Read Destination address
        *pTmp->CC_ADDR = hal_cc11_spiExch(0);            // Read Source address

        for(i = 0; i < frameLen; i++)                   // Read Payload
          pTmp->raw[i] = hal_cc11_spiExch(0);

        tmp = hal_cc11_spiExch(0);                      // Read RSSI
        i = hal_cc11_spiExch(0);                        // Read LQI 
        RF_RELEASE();                                   // Release CC1101

        if(i & CC11_LQI_CRC_OK)                         // is CRC Ok ?
        {
          //cc11v_Foffs = cc11_readReg(CC11_FREQEST | CC11_STATUS_REGISTER);    // int8_t frequency offset
#ifdef RF_USE_RSSI
          cc11v_RSSI = tmp;
#endif  //  RF_USE_RSSI
          xQueueSendFromISR(cc11_out_queue, &pTmp, pxHigherPriorityTaskWoken);
        }
        else                                            // Bad CRC
          vPortFree(pTmp);
      }
      break;
    case CC11_STATE_TXHDR:
      if(marcs != CC11_MARCSTATE_TX)
        break;

      cc11v_State = CC11_STATE_TXDATA;
      return;
    default:
      break;
  }

  LEDsOff();
  cc11_cmdStrobe(CC11_SIDLE);     // Enter to the IDLE state
  cc11_cmdStrobe(CC11_SFTX);
  cc11_cmdStrobe(CC11_SFRX);
  cc11_cmdStrobe(CC11_SRX);       // Enter to RX State
  cc11v_State = CC11_STATE_RXIDLE;
}

#endif  //  CC11_PHY
