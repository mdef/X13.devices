/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

// Global configuration settings

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// MQTT-SN Section
#define MQTTSN_MSG_SIZE         48                // Size of payload(base for all buffers)

// Object Dictionary
#define OD_MAX_INDEX_LIST       16                // Size of identificators list
#define OD_DEV_SWVERSH          '4'               // Software Version
#define OD_DEV_SWVERSM          '0'
#define OD_DEV_SWVERSL          'a'               // Alfa

#define RF_BASE_FREQ            868300000UL

#define EXTDIO_USED             1
//#define EXTAIN_USED             1
//#define PLC_USED                1

#if   (defined CFG_S2EN10)
#include "HAL/HWconfig_S2En10.h"
#elif (defined CFG_S2ES10)
#include "HAL/HWconfig_S2ES10.h"
#elif (defined CFG_S2SN10)
#include "HAL/HWconfig_S2Sn10.h"
#elif (defined CFG_S3EN10)
#include "HAL/HWconfig_S3En10.h"
#elif (defined CFG_S3SN10)
#include "HAL/HWconfig_S3Sn10.h"
#elif (defined CFG_A1EN11)
#include "HAL/HWconfig_A1En11.h"
#elif (defined CFG_A1CN11)
#include "HAL/HWconfig_A1Cn11.h"
#elif (defined CFG_A1SC11)
#include "HAL/HWconfig_A1SC11.h"
#elif (defined CFG_A1SN11)
#include "HAL/HWconfig_A1Sn11.h"
#elif (defined CFG_A3CN11)
#include "HAL/HWconfig_A3Cn11.h"
#elif (defined CFG_A3SC11)
#include "HAL/HWconfig_A3SC11.h"
#elif (defined CFG_A3SN11)
#include "HAL/HWconfig_A3Sn11.h"
#elif (defined CFG_A4SN11)
#include "HAL/HWconfig_A4Sn11.h"
#else
#error Undefined configuration
#endif  //  Configuration

#include "mqTypes.h"
#include "mqttsn.h"
#include "ObjDict.h"

#endif  //  _CONFIG_H
