/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

// Extensions digital inputs/outputs

#include "../config.h"
#include "extdio.h"

#ifdef EXTDIO_USED

#if (DIO_PORT_SIZE == 8)
#define DIO_PORT_POS        3
#define DIO_PORT_MASK       0x07
#elif (DIO_PORT_SIZE == 16)
#define DIO_PORT_POS        4
#define DIO_PORT_MASK       0x0F
#elif (DIO_PORT_SIZE == 32)
#define DIO_PORT_POS        5
#define DIO_PORT_MASK       0x1F
#endif  //  DIO_PORT_SIZE

static const DIO_PORT_TYPE portnum2mask[EXTDIO_MAXPORT_NR] = PORTNUM_2_MASK;

static DIO_PORT_TYPE pin_write_mask[EXTDIO_MAXPORT_NR];

static DIO_PORT_TYPE pin_read_mask[EXTDIO_MAXPORT_NR];
static DIO_PORT_TYPE pin_state_flag[EXTDIO_MAXPORT_NR];
static DIO_PORT_TYPE pin_change_flag[EXTDIO_MAXPORT_NR];

static DIO_PORT_TYPE * pPin_status;

// HAL
extern void dioConfigPort(uint8_t PortNr, DIO_PORT_TYPE Mask, eDIOmode_t Mode);
extern DIO_PORT_TYPE dioReadPort(uint8_t PortNr);
extern void dioWritePort(uint8_t PortNr, DIO_PORT_TYPE Mask, bool Value);

// Convert Base to Mask
static DIO_PORT_TYPE dioBase2Mask(uint16_t base)
{
  DIO_PORT_TYPE retval = 1;
  base &= DIO_PORT_MASK;
  while(base--)
    retval <<= 1;
  return retval;
}

// Convert Base to Port Number
static uint8_t dioBase2Port(uint16_t base)     // Digital Ports
{
  uint8_t tmp = base>>DIO_PORT_POS;
  if(tmp >= EXTDIO_MAXPORT_NR)
    return EXTDIO_MAXPORT_NR;
  return tmp;
}

static uint8_t dioCheckBase(uint16_t base)
{
  DIO_PORT_TYPE pinmask = dioBase2Mask(base);
  uint8_t port = dioBase2Port(base);

  if((port == EXTDIO_MAXPORT_NR) || (portnum2mask[port] & pinmask))
    return 2; // Port not exist

  if((pin_read_mask[port] & pinmask) ||
     (pin_write_mask[port] & pinmask))
    return 1; // Port busy

  return 0; // Port free
}

// Check Index digital inp/out
uint16_t dioCheckIdx(subidx_t * pSubidx)
{
  uint16_t base = pSubidx->Base;
  if((dioCheckBase(base) == 2) ||       // Not Exist
     ((pSubidx->Type != objPinNPN) && (pSubidx->Type != objPinPNP)
#ifdef ASLEEP
     && (pSubidx->Type != objActPNP) && (pSubidx->Type != objActNPN)
#endif  //  ASLEEP
     ))
    return 0xFFFF;    // Not Supported
  return (EXTDIO_BASE + base);
}

void dioInit(void *pBuf)
{
  pPin_status = (DIO_PORT_TYPE *)pBuf;

  uint8_t port;
  for(port = 0; port < EXTDIO_MAXPORT_NR; port++)
  {
    pin_write_mask[port] = 0;
    pin_read_mask[port] = 0;
    pin_state_flag[port] = 0;
    pin_change_flag[port] = 0;
  }
}

// Read digital Inputs
static uint8_t dioReadOD(subidx_t * pSubidx, uint8_t *pLen, uint8_t *pBuf)
{
  uint16_t base = pSubidx->Base;
  DIO_PORT_TYPE state = pPin_status[dioBase2Port(base)];
  DIO_PORT_TYPE mask = dioBase2Mask(base);
  pin_change_flag[dioBase2Port(base)] &= ~mask;
  
  if(pSubidx->Type == objPinNPN)
    state = ~state;
  *pLen = 1;
  *pBuf = ((state & mask) != 0) ? 1 : 0;
  return MQTTSN_RET_ACCEPTED;
}

// Write DIO Object's
static uint8_t dioWriteOD(subidx_t * pSubidx, uint8_t Len, uint8_t *pBuf)
{
  uint16_t base = pSubidx->Base;
  uint8_t state = *pBuf;
  uint8_t port = dioBase2Port(base);
  DIO_PORT_TYPE mask = dioBase2Mask(base);
  pin_change_flag[dioBase2Port(base)] &= ~mask;

  if(pSubidx->Type == objPinNPN)
    state = ~state;

  if(state & 1)
    pPin_status[port] |= mask;
  else
    pPin_status[port] &= ~mask;

  return MQTTSN_RET_ACCEPTED;
}

// Poll Procedure
static uint8_t dioPollOD(subidx_t * pSubidx, uint8_t sleep)
{
  uint16_t base = pSubidx->Base;
  return ((pin_change_flag[dioBase2Port(base)] & dioBase2Mask(base)) != 0) ? 1 : 0;
}

// Register digital inp/out/pwm Object
uint8_t dioRegisterOD(indextable_t *pIdx)
{
  uint16_t base = pIdx->sidx.Base;
  if(dioCheckBase(base) != 0)
    return MQTTSN_RET_REJ_INV_ID;
  
  uint8_t port = dioBase2Port(base);
  DIO_PORT_TYPE mask = dioBase2Mask(base);
  pin_change_flag[port] &= ~mask;

  if(pIdx->sidx.Place == objDout)
  {
    dioConfigPort(port, mask, DIO_MODE_OUT);
    pin_write_mask[port] |= mask;

    if(pIdx->sidx.Type == objPinPNP)
    {
      dioWritePort(port, mask, 0);
      pPin_status[port] &= ~mask;
    } // else NPN
    else
    {
      dioWritePort(port, mask, 1);
      pPin_status[port] |= mask;
    }
  }
  else if(pIdx->sidx.Place == objDin)
  {
    pIdx->cbPoll = &dioPollOD;
    pin_read_mask[port] |= mask;

    if(pIdx->sidx.Type == objPinPNP)
    {
      dioConfigPort(port, mask, DIO_MODE_IN_PD);
      pin_state_flag[port] &= ~mask;
      pPin_status[port] &= ~mask;
    } // else NPN
    else
    {
      dioConfigPort(port, mask, DIO_MODE_IN_PU);
      pin_state_flag[port] |= mask;
      pPin_status[port] |= mask;
    }
  }

  pIdx->cbRead = &dioReadOD;
  pIdx->cbWrite = &dioWriteOD;

  return MQTTSN_RET_ACCEPTED;
}

void dioDeleteOD(subidx_t * pSubidx)
{
  uint16_t base = pSubidx->Base;
  uint8_t port = dioBase2Port(base);
  DIO_PORT_TYPE mask = dioBase2Mask(base);
  
  pin_write_mask[port] &= ~mask;
  pin_read_mask[port] &= ~mask;

  dioConfigPort(port, mask, DIO_MODE_IN_FLOAT);
}

void dioProc(void)
{
  uint8_t port;
  DIO_PORT_TYPE state, mask;
  
  for(port = 0; port < EXTDIO_MAXPORT_NR; port++)
  {
    mask = pin_read_mask[port];
    if(mask != 0)
    {
      state = dioReadPort(port) & mask;
      
      DIO_PORT_TYPE maskp = 1;
      
      while(maskp)
      {
        DIO_PORT_TYPE maski = mask & maskp;
        if(maski)
        {
          if((pin_state_flag[port] ^ state) & maski)
          {
            pin_state_flag[port] &= ~maski;
            pin_state_flag[port] |= state & maski;
          }
          else
          {
            DIO_PORT_TYPE staterd =  ((pPin_status[port] ^ state) & maski);
            if(staterd)
            {
              pPin_status[port] ^= staterd;
              pin_change_flag[port] |= staterd;
            }
          }
        }
        maskp <<= 1;
      }
    }
    
    mask = pin_write_mask[port];
    if(mask != 0)
    {
      state = pPin_status[port] & mask;
      if(state)
        dioWritePort(port, state, 1);
        
      state = ~pPin_status[port] & mask;
      if(state)
        dioWritePort(port, state, 0);
    }
  }
}
#endif    //  EXTDIO_USED
