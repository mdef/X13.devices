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

#define EXTDIO_USED             1

//#include "HAL/HWconfig_S2Sn10.h"
//#include "HAL/HWconfig_S2En10.h"
//#include "HAL/HWconfig_S2ES10.h"
//#include "HAL/HWconfig_S3Sn10.h"
#include "HAL/HWconfig_S3En10.h"

#include "mqTypes.h"
#include "mqttsn.h"
#include "ObjDict.h"

#endif  //  _CONFIG_H
