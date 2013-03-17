/*
Copyright (c) 2011-2013 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.github.com

BSD New License
See LICENSE.txt file for license details.
*/

// TWI Driver Bosh - BMP085, Pressure & Temperature

// Outs
//  Tw30464     - Temperature       0,1�C
//  Td30464     - Pressure          0,01 hPa(mBar)

#define BMP085_ADDR                 0x77

#define BMP085_CHIP_ID              0x55
// Register definitions
#define BMP085_PROM_START__ADDR     0xAA
#define BMP085_CHIP_ID_REG          0xD0
#define BMP085_CTRL_MEAS_REG        0xF4
#define BMP085_ADC_OUT_MSB_REG      0xF6
//
#define BMP085_T_MEASURE            0x2E    // temperature measurent 
#define BMP085_P_MEASURE            0x34    // pressure measurement

#define SMD500_PARAM_MG             3038    // calibration parameter
#define SMD500_PARAM_MH            -7357    // calibration parameter
#define SMD500_PARAM_MI             3791    // calibration parameter

#define BMP085_OSS                  0       // oversampling settings [0 - Low Power / 3 - Ultra High resolution]

#define BMP085_P_MIN_DELTA          20
#define BMP085_T_MIN_DELTA          1

typedef struct
{
    // Calibration parameters
    int16_t  ac1;
    int16_t  ac2; 
    int16_t  ac3; 
    uint16_t ac4;
    uint16_t ac5;
    uint16_t ac6;
    int16_t  b1; 
    int16_t  b2;
    int16_t  mb;
    int16_t  mc;
    int16_t  md;
}s_BMP085_CALIB_t;

static s_BMP085_CALIB_t bmp085_calib;

static uint8_t bmp085_stat;

static int32_t  bmp085_b5;              // Compensation parameter
static uint16_t bmp085_oldtemp = 0;
static uint32_t bmp085_oldpress = 0;

static uint8_t twi_BMP085_Read(subidx_t * pSubidx, uint8_t *pLen, uint8_t *pBuf) 
{
    if(pSubidx->Base & 1)               // Read Pressure
    {
        *pLen = 4;
        *(uint32_t *)pBuf = bmp085_oldpress;
    }
    else                                // Read Temperature
    {
        *pLen = 2;
        *(uint16_t *)pBuf = bmp085_oldtemp;
    }
    return MQTTS_RET_ACCEPTED;
}

static uint8_t twi_BMP085_Write(subidx_t * pSubidx, uint8_t Len, uint8_t *pBuf)
{
    if(pSubidx->Base & 1)               // Renew Pressure
        bmp085_oldpress = *(uint32_t *)pBuf;
    else                                // Renew Temperature
        bmp085_oldtemp = *(uint16_t *)pBuf;
    return MQTTS_RET_ACCEPTED;
}

static uint8_t twi_BMP085_Pool1(subidx_t * pSubidx)
{
    uint16_t ut;
    int32_t x1,x2;

    if(twim_access & (TWIM_ERROR | TWIM_RELEASE))   // Bus Error, or request to release bus
    {
        if(bmp085_stat != 0)
        {
            bmp085_stat = 0x40;
            if(twim_access & TWIM_RELEASE)
                twim_access = TWIM_RELEASE;
        }
        return 0;
    }

    if(twim_access & (TWIM_READ | TWIM_WRITE))      // Bus Busy
        return 0;
    
    switch(bmp085_stat)
    {
        case 0:
            if(twim_access & TWIM_BUSY)
                return 0;
            bmp085_stat = 1;
        case 1:             // Start dummy Conversion, Temperature
        case 3:             // Start Conversion, Temperature
            twim_buf[0] = BMP085_CTRL_MEAS_REG;
            twim_buf[1] = BMP085_T_MEASURE;
            twimExch_ISR(BMP085_ADDR, (TWIM_BUSY | TWIM_WRITE), 2, 0, (uint8_t *)twim_buf, NULL);
            break;
        // !! ut Conversion time 4,5 mS
        case 2:             // Get dummy ut
        case 4:             // Get ut
            twim_buf[0] = BMP085_ADC_OUT_MSB_REG;            // Select ADC out register
            twimExch_ISR(BMP085_ADDR, (TWIM_BUSY | TWIM_WRITE | TWIM_READ), 1, 2, 
                                                                        (uint8_t *)twim_buf, NULL);
            break;
        case 5:             // Get uncompensated temperature, and normalize
            ut = ((uint16_t)twim_buf[0]<<8) | twim_buf[1];

            // Calculate temperature
            x1 = (((int32_t)ut - bmp085_calib.ac6) * bmp085_calib.ac5) >> 15;
            x2 = ((int32_t)bmp085_calib.mc << 11) / (x1 + bmp085_calib.md);
            bmp085_b5 = x1 + x2;

            ut = (bmp085_b5 + 8)>>4;
            
            x1 = ut > bmp085_oldtemp ? ut - bmp085_oldtemp : bmp085_oldtemp - ut;

            if(x1 > BMP085_T_MIN_DELTA)
            {
                bmp085_stat++;
                bmp085_oldtemp = ut;
                return 1;
            }
            break;
        case 6:             // Start dummy conversion, Pressure
        case 9:             // Start conversion, Pressure
            twim_buf[0] = BMP085_CTRL_MEAS_REG;
            twim_buf[1] = BMP085_P_MEASURE + (BMP085_OSS<<6);
            twimExch_ISR(BMP085_ADDR, (TWIM_BUSY | TWIM_WRITE), 2, 0, (uint8_t *)twim_buf, NULL);
            break;
        // !! up Conversion time on ultra high resolution (BMP085_OSS = 3) 25,5 mS
        case 8:             // Get dummy up
        case 11:            // Get up
            twim_buf[0] = BMP085_ADC_OUT_MSB_REG;            // Select ADC out register
            twimExch_ISR(BMP085_ADDR, (TWIM_BUSY | TWIM_WRITE | TWIM_READ), 1, 3,
                                                                        (uint8_t *)twim_buf, NULL);
            break;
    }
    bmp085_stat++;
    return 0;
}

static uint8_t twi_BMP085_Pool2(subidx_t * pSubidx)
{
    uint32_t up, b4, b7;
    int32_t b6, x1, x2, x3, b3, p;

    if(bmp085_stat == 13)
    {
        bmp085_stat++;
        up = (((uint32_t)twim_buf[0]<<16) | ((uint32_t)twim_buf[1]<<8) |
                       ((uint32_t)twim_buf[2]))>>(8-BMP085_OSS);
        twim_access = 0;    // Bus Free
        b6 = bmp085_b5 - 4000;
        //  calculate B3
        x1 = (b6 * b6)>>12;
        x1 *= bmp085_calib.b2;
        x1 >>= 11;
        x2 = bmp085_calib.ac2 * b6;
        x2 >>= 11;
        x3 = x1 + x2;
        b3 = (((((int32_t)bmp085_calib.ac1) * 4 + x3) << BMP085_OSS) + 2) >> 2;
        // calculate B4
        x1 = (bmp085_calib.ac3 * b6) >> 13;
        x2 = (bmp085_calib.b1 * ((b6*b6) >> 12)) >> 16;
        x3 = ((x1 + x2) + 2) >> 2;
        b4 = (bmp085_calib.ac4 * (uint32_t)(x3 + 32768)) >> 15;
        b7 = ((uint32_t)(up - b3) * (50000>>BMP085_OSS));
        if(b7 < 0x80000000)
            p = (b7 << 1) / b4;
        else
            p = (b7 / b4) << 1;
        x1 = (p >> 8);
        x1 *= x1;
        x1 = (x1 * SMD500_PARAM_MG) >> 16;
        x2 = (p * SMD500_PARAM_MH) >> 16;
        p += (x1 + x2 + SMD500_PARAM_MI) >> 4;

        up = p > bmp085_oldpress ? p - bmp085_oldpress : bmp085_oldpress - p;
        if(up > BMP085_P_MIN_DELTA)
        {
            bmp085_oldpress = p;
            return 1;
        }
    }
    return 0;
}

static uint8_t twi_BMP085_Config(void)
{
    uint8_t reg = BMP085_CHIP_ID_REG;
    if((twimExch(BMP085_ADDR, (TWIM_READ | TWIM_WRITE), 1, 1, &reg) != 
                                                                TW_SUCCESS) ||  // Communication error
       (reg != BMP085_CHIP_ID))                                                 // Bad device ID
        return 0;

    // readout bmp085 calibparam structure
    bmp085_calib.ac1 = BMP085_PROM_START__ADDR;
    twimExch(BMP085_ADDR, (TWIM_READ | TWIM_WRITE), 1, 22, (uint8_t *)&bmp085_calib);

    bmp085_stat = 0;
    bmp085_oldtemp = 0;
    bmp085_oldpress = 0;

    //parameters AC1-AC6
    bmp085_calib.ac1 = SWAPWORD(bmp085_calib.ac1);
    bmp085_calib.ac2 = SWAPWORD(bmp085_calib.ac2);
    bmp085_calib.ac3 = SWAPWORD(bmp085_calib.ac3);
    bmp085_calib.ac4 = SWAPWORD(bmp085_calib.ac4);
    bmp085_calib.ac5 = SWAPWORD(bmp085_calib.ac5);
    bmp085_calib.ac6 = SWAPWORD(bmp085_calib.ac6);
    //parameters B1,B2
    bmp085_calib.b1 =  SWAPWORD(bmp085_calib.b1);
    bmp085_calib.b2 =  SWAPWORD(bmp085_calib.b2);
    //parameters MB,MC,MD
    bmp085_calib.mb =  SWAPWORD(bmp085_calib.mb);
    bmp085_calib.mc =  SWAPWORD(bmp085_calib.mc);
    bmp085_calib.md =  SWAPWORD(bmp085_calib.md);

    // Register variables
    indextable_t * pIndex1;
    pIndex1 = getFreeIdxOD();
    if(pIndex1 == NULL)
        return 0;

    pIndex1->cbRead  =  &twi_BMP085_Read;
    pIndex1->cbWrite =  &twi_BMP085_Write;
    pIndex1->cbPool  =  &twi_BMP085_Pool1;
    pIndex1->sidx.Place = objTWI;               // Object TWI
    pIndex1->sidx.Type =  objInt16;             // Variables Type -  UInt16
    pIndex1->sidx.Base = (BMP085_ADDR<<8);      // Device addr

    indextable_t * pIndex2;
    pIndex2 = getFreeIdxOD();
    if(pIndex2 == NULL)
    {
        pIndex1->Index = 0xFFFF;                // Free Index
        return 0;
    }

    pIndex2->cbRead  =  &twi_BMP085_Read;
    pIndex2->cbWrite =  &twi_BMP085_Write;
    pIndex2->cbPool  =  &twi_BMP085_Pool2;
    pIndex2->sidx.Place = objTWI;               // Object TWI
    pIndex2->sidx.Type =  objUInt32;            // Variables Type -  UInt32
    pIndex2->sidx.Base = (BMP085_ADDR<<8) + 1;  // Device addr

    return 2;
}