/*
Copyright (c) 2011-2013 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.github.com

BSD New License
See LICENSE.txt file for license details.
*/

// TWI Driver LM75, Temperature

// Outs
// Tw(addr | pos) ADC Register (Temp * 256)
//  18432
//  18688 - 18689
//  ...
//  20224 - 20231

#include "../../config.h"

#if (defined EXTDIO_USED) && (defined TWI_USED) && (defined TWI_USE_LM75)

#include "../twim.h"
#include "twiDriver_LM75.h"

//#define LM75_T_MIN_DELTA            63          // use hysteresis for temperature

extern volatile uint8_t twim_access;           // access mode & busy flag

// Process variables
static uint8_t  lm75_stat[LM75_MAX_DEV];
static int16_t  lm75_oldVal[LM75_MAX_DEV];
uint8_t lm75_buf[2];

uint8_t twi_lm75_Read(subidx_t * pSubidx, uint8_t *pLen, uint8_t *pBuf)
{
    *pLen = 2;
/*
    // Return T 0.1�C
    int32_t temp = lm75_oldVal[pSubidx->Base & (LM75_MAX_DEV - 1)];
    temp *= 5;
//    temp /= 128;
    temp >>= 7;
    *(uint16_t *)pBuf = temp & 0xFFFF;
*/
    *(uint16_t *)pBuf = lm75_oldVal[pSubidx->Base & (LM75_MAX_DEV - 1)];
    return MQTTS_RET_ACCEPTED;
}

uint8_t twi_lm75_Pool(subidx_t * pSubidx, uint8_t sleep)
{
#ifdef ASLEEP
  if(sleep != 0)
  {
    lm75_stat[base] = (0xFF-(POOL_TMR_FREQ/2));
    return 0;
  }
#endif  //  ASLEEP

  uint8_t base = pSubidx->Base & (LM75_MAX_DEV - 1);
  uint16_t val;
  uint8_t  tmp;
  tmp = lm75_stat[base];

  if(twim_access & TWIM_ERROR)   // Bus Error
  {
    if(tmp < 3)
        lm75_stat[base] = 3;
    return 0;
  }

  if(tmp == 0)
  {
    if(twim_access == 0)
    {
      lm75_stat[base] = 1;
      lm75_buf[0] = LM75_REG_TEMP;
      twimExch_ISR(pSubidx->Base>>8, (TWIM_WRITE | TWIM_READ), 1, 2, lm75_buf, NULL);
      return 0;
    }
  }
  else if(tmp == 1)
  {
    val = ((uint16_t)lm75_buf[0]<<8) | (lm75_buf[1]);
    lm75_stat[base]++;
#if (defined LM75_T_MIN_DELTA) && (LM75_T_MIN_DELTA > 0)
    if((val > lm75_oldVal[base] ? val - lm75_oldVal[base] : lm75_oldVal[base] - val) > LM75_T_MIN_DELTA)
#else
    if(val != lm75_oldVal[base])
#endif  //  LM75_T_MIN_DELTA
    {
      lm75_oldVal[base] = val;
      return 1;
    }
  }

  lm75_stat[base]++;
  return 0;
}

uint8_t twi_LM75_Config(void)
{
    uint8_t addr = LM75_START_ADDR;
    uint8_t pos = 0;
    
    indextable_t * pIndex;

    while((addr <= LM75_STOP_ADDR) && (pos < LM75_MAX_DEV))
    {
        lm75_buf[0] = LM75_REG_CONF;
        lm75_buf[1] = 0;    

        if(twimExch(addr, TWIM_WRITE, 2, 0, lm75_buf) == TW_SUCCESS)
        {
            lm75_stat[pos] = 0x80;
            lm75_oldVal[pos] = 0;

            // Register variable
            pIndex = getFreeIdxOD();
            if(pIndex == NULL)
                break;
            
            pIndex->cbRead  =  &twi_lm75_Read;
            pIndex->cbWrite =  NULL;
            pIndex->cbPool  =  &twi_lm75_Pool;
            pIndex->sidx.Place = objTWI;                   // Object TWI
            pIndex->sidx.Type =  objInt16;                 // Variables Type -  UInt16
            pIndex->sidx.Base = ((uint16_t)addr<<8) | pos;

            pos++;
        }
        addr++;
    }
    return pos;
}

#endif  //  (defined EXTDIO_USED) && (defined TWI_USED) && (defined TWI_USE_LM75)
