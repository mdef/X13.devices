/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

#ifndef _MQTTSN_H
#define _MQTTSN_H

typedef enum e_MQTTSN_POLL_STATUS
{
  MQTTSN_POLL_STAT_NOP = 0,
#ifdef ASLEEP
  MQTTSN_POLL_STAT_ASLEEP,
  MQTTSN_POLL_STAT_AWAKE,
#endif  //  ASLEEP
  MQTTSN_POLL_STAT_CONNECTED,
  MQTTSN_POLL_STAT_DISCONNECTED
}MQTTSN_POLL_STATUS_t;

// Current Status
typedef enum e_MQTTSN_STATUS
{
  MQTTSN_STATUS_DISCONNECTED = 0,
  MQTTSN_STATUS_SEARCHGW,
  MQTTSN_STATUS_OFFLINE,
  MQTTSN_STATUS_PRE_CONNECT,
  MQTTSN_STATUS_CONNECT,
#ifdef ASLEEP
  MQTTSN_STATUS_POST_CONNECT,
  MQTTSN_STATUS_PRE_ASLEEP,
  MQTTSN_STATUS_ASLEEP,
  MQTTSN_STATUS_AWAKE,
  MQTTSN_STATUS_POST_AWAKE,
#endif  //  ASLEEP
}e_MQTTSN_STATUS_t;

void MQTTSN_Init(void);
e_MQTTSN_STATUS_t MQTTSN_GetStatus(void);
void MQTTSN_EnqueueMsg(MQ_MSG_t * pMqMsg);

/*
MQTTS_POLL_STATUS_t MQTTS_Poll(void);
void MQTTS_Parser(MQ_t * pBuf);
void MQTTS_ParserBroadcast(MQ_t * pBuf);
bool MQTTS_DataRdy(void);
MQ_t * MQTTS_Get(void);
void MQTTS_SendMsg(e_MQTTS_MSGTYPE_t MsgType, uint8_t Flags, uint16_t TopicId);
*/
/*
#ifdef ASLEEP
void mqtts_set_TASleep(uint16_t tasleep);
#endif  //  ASLEEP
void MQTTS_Publish(uint16_t TopicID, uint8_t Flags);
void MQTTS_Subscribe(uint8_t Flags, uint8_t Size, uint8_t * ipBuf);
void MQTTS_Register(uint16_t TopicID, uint8_t Size, uint8_t * ipBuf);
*/

#endif
