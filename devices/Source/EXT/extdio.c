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

static const DIO_PORT_TYPE dio_portnum2mask[EXTDIO_MAXPORT_NR] = EXTDIO_PORTNUM2MASK;

static DIO_PORT_TYPE dio_write_mask[EXTDIO_MAXPORT_NR];

static DIO_PORT_TYPE dio_read_mask[EXTDIO_MAXPORT_NR];
static DIO_PORT_TYPE dio_state_flag[EXTDIO_MAXPORT_NR];
static DIO_PORT_TYPE dio_change_flag[EXTDIO_MAXPORT_NR];

DIO_PORT_TYPE dio_status[EXTDIO_MAXPORT_NR];

// HAL
void hal_dio_configure(uint8_t PortNr, DIO_PORT_TYPE Mask, eDIOmode_t Mode);
DIO_PORT_TYPE hal_dio_read(uint8_t PortNr);
void hal_dio_set(uint8_t PortNr, DIO_PORT_TYPE Mask);
void hal_dio_reset(uint8_t PortNr, DIO_PORT_TYPE Mask);

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
    uint8_t tmp;
#ifdef EXTDIO_BASE_OFFSET
    tmp = (base>>DIO_PORT_POS) - EXTDIO_BASE_OFFSET;
#else
    tmp = (base>>DIO_PORT_POS);
#endif  //  EXTDIO_BASE_OFFSET

    if(tmp >= EXTDIO_MAXPORT_NR)
        return EXTDIO_MAXPORT_NR;
    return tmp;
}

static uint8_t dioCheckBase(uint16_t base)
{
    DIO_PORT_TYPE pinmask = dioBase2Mask(base);
    uint8_t port = dioBase2Port(base);

    if((port == EXTDIO_MAXPORT_NR) || (dio_portnum2mask[port] & pinmask))
        return 2; // Port not exist

    if((dio_read_mask[port] & pinmask) || (dio_write_mask[port] & pinmask))
        return 1; // Port busy

    return 0; // Port free
}

// Check Index digital inp/out
uint8_t dioCheckIdx(subidx_t * pSubidx)
{
    if(
#ifdef ASLEEP
       (pSubidx->Type != objActPNP) && (pSubidx->Type != objActNPN) &&
#endif  //  ASLEEP
       (pSubidx->Type != objPinNPN) && (pSubidx->Type != objPinPNP))
        return 2;

    return dioCheckBase(pSubidx->Base);
}

void dioInit()
{
    uint8_t port;
    for(port = 0; port < EXTDIO_MAXPORT_NR; port++)
    {
        dio_write_mask[port] = 0;
        dio_read_mask[port] = 0;
        dio_state_flag[port] = 0;
        dio_change_flag[port] = 0;
        dio_status[port] = 0;
    }
}

// Read digital Inputs
e_MQTTSN_RETURNS_t dioReadOD(subidx_t * pSubidx, uint8_t *pLen, uint8_t *pBuf)
{
    uint16_t base = pSubidx->Base;
    DIO_PORT_TYPE state = dio_status[dioBase2Port(base)];
    DIO_PORT_TYPE mask = dioBase2Mask(base);
    dio_change_flag[dioBase2Port(base)] &= ~mask;
  
    if(pSubidx->Type == objPinNPN)
        state = ~state;
    *pLen = 1;
    *pBuf = ((state & mask) != 0) ? 1 : 0;
    return MQTTSN_RET_ACCEPTED;
}

// Write DIO Object's
e_MQTTSN_RETURNS_t dioWriteOD(subidx_t * pSubidx, uint8_t Len, uint8_t *pBuf)
{
    uint16_t base = pSubidx->Base;
    uint8_t state = *pBuf;
    uint8_t port = dioBase2Port(base);
    DIO_PORT_TYPE mask = dioBase2Mask(base);
    dio_change_flag[dioBase2Port(base)] &= ~mask;

    if(pSubidx->Type == objPinNPN)
        state = ~state;

    if(state & 1)
        dio_status[port] |= mask;
    else
        dio_status[port] &= ~mask;

    return MQTTSN_RET_ACCEPTED;
}

// Poll Procedure
uint8_t dioPollOD(subidx_t * pSubidx, uint8_t sleep)
{
    uint16_t base = pSubidx->Base;
    return ((dio_change_flag[dioBase2Port(base)] & dioBase2Mask(base)) != 0) ? 1 : 0;
}

// Register digital inp/out/pwm Object
e_MQTTSN_RETURNS_t dioRegisterOD(indextable_t *pIdx)
{
    uint16_t base = pIdx->sidx.Base;
    if(dioCheckBase(base) != 0)
        return MQTTSN_RET_REJ_INV_ID;

    uint8_t port = dioBase2Port(base);
    DIO_PORT_TYPE mask = dioBase2Mask(base);
    dio_change_flag[port] &= ~mask;

    if(pIdx->sidx.Place == objDout)
    {
        hal_dio_configure(port, mask, DIO_MODE_OUT);
        dio_write_mask[port] |= mask;

        if(pIdx->sidx.Type == objPinPNP)
        {
            hal_dio_reset(port, mask);
            dio_status[port] &= ~mask;
        }
        else    // NPN
        {
            hal_dio_set(port, mask);
            dio_status[port] |= mask;
        }
    }
    else if(pIdx->sidx.Place == objDin)
    {
        pIdx->cbPoll = &dioPollOD;
        dio_read_mask[port] |= mask;

        if(pIdx->sidx.Type == objPinPNP)
        {
            hal_dio_configure(port, mask, DIO_MODE_IN_PD);
            dio_state_flag[port] &= ~mask;
            dio_status[port] &= ~mask;
        } // else NPN
        else
        {
            hal_dio_configure(port, mask, DIO_MODE_IN_PU);
            dio_state_flag[port] |= mask;
            dio_status[port] |= mask;
        }
    }
#ifdef EXTAIN_USED
    else if(pIdx->sidx.Place == objAin)
    {
        hal_dio_configure(port, mask, DIO_MODE_AIN);
        dio_write_mask[port] |= mask;
        return MQTTSN_RET_ACCEPTED;
    }
#endif  //  EXTAIN_USED

    pIdx->cbRead = &dioReadOD;
    pIdx->cbWrite = &dioWriteOD;
    return MQTTSN_RET_ACCEPTED;
}

void dioDeleteOD(subidx_t * pSubidx)
{
    uint16_t base = pSubidx->Base;
    uint8_t port = dioBase2Port(base);
    DIO_PORT_TYPE mask = dioBase2Mask(base);
  
    dio_write_mask[port] &= ~mask;
    dio_read_mask[port] &= ~mask;

    hal_dio_configure(port, mask, DIO_MODE_IN_FLOAT);
}

void dioProc(void)
{
    uint8_t port;
    DIO_PORT_TYPE state, mask;
  
    for(port = 0; port < EXTDIO_MAXPORT_NR; port++)
    {
        mask = dio_read_mask[port];
        if(mask != 0)
        {
            state = hal_dio_read(port) & mask;
      
            DIO_PORT_TYPE maskp = 1;
      
            while(maskp)
            {
                DIO_PORT_TYPE maski = mask & maskp;
                if(maski)
                {
                    if((dio_state_flag[port] ^ state) & maski)
                    {
                        dio_state_flag[port] &= ~maski;
                        dio_state_flag[port] |= state & maski;
                    }
                    else
                    {
                        DIO_PORT_TYPE staterd =  ((dio_status[port] ^ state) & maski);
                        if(staterd)
                        {
                            dio_status[port] ^= staterd;
                            dio_change_flag[port] |= staterd;
                        }
                    }
                }
                maskp <<= 1;
            }
        }
    
        mask = dio_write_mask[port];
        if(mask != 0)
        {
            state = dio_status[port] & mask;
            if(state)
                hal_dio_set(port, state);
        
            state = ~dio_status[port] & mask;
            if(state)
                hal_dio_reset(port, state);
        }
    }
}
#endif    //  EXTDIO_USED
