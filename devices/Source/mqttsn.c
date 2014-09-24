/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

// MQTT-SN Library, Version 4.0 alfa

#include "config.h"

#define MQTTSN_DEF_KEEPALIVE     300    // Keep Alive Time - 5 Min
#define MQTTSN_DEF_NRETRY        3      // Number of retry's
#define MQTTSN_DEF_TCONNECT      10     // Time between Connect's
#define MQTTSN_DEF_TSGW          15     // Time between search gateway request
#define MQTTSN_DEF_TDISCONNECT   30     // Pause on Disconnect State

#define MQTTSN_DEF_PROTOCOLID    0x01

#define MQTTSN_MAX_RADIUS        3      // Hops to Gateway

#define POLL_TMR_FREQ           (configTICK_RATE_HZ / 8)    // 125 mS

#define MQTTSN_SIZEOF_MESSAGE_BUF 8

// Local Variables
typedef struct
{
  uint8_t                   GatewayAddr[sizeof(PHY1_ADDR_t)];   // Gateway Address
  uint8_t                   NodeAddrPhy1[sizeof(PHY1_ADDR_t)];
#ifdef PHY2_ADDR_t
  uint8_t                   NodeAddrPhy2[sizeof(PHY2_ADDR_t)];
#endif
  
  uint8_t                   GwId;                 // Unique Gateway ID
  uint8_t                   Radius;               // Broadcast Radius

  // Timeouts
  uint8_t                   pfCnt;
  uint16_t                  Tretry;
  uint8_t                   Nretry;               // Retry number
  
  e_MQTTSN_STATUS_t         Status;               // Actual status

  // Register / Subscribe / Publish FIFO
  MQ_MSG_t                  MsgBuf[MQTTSN_SIZEOF_MESSAGE_BUF];
  
  uint16_t                  MsgId;                // Outgoing message ID
}MQTTSN_VAR_t;

static MQTTSN_VAR_t  vMQTTSN;

////////////////////////////////////////////////////////////////////////
// Parse incoming messages

static bool mqttsn_pack_msgbuf(e_MQTTSN_MSGTYPE_t MsgType, uint16_t MsgId)
{
  uint8_t pos;
  for(pos = 0; pos < MQTTSN_SIZEOF_MESSAGE_BUF; pos++)
  {
    if((vMQTTSN.MsgBuf[pos].MsgType == MsgType) &&
       (vMQTTSN.MsgBuf[pos].MsgId == MsgId))
    {
      // Pack MessageBuf
      for(;pos < (MQTTSN_SIZEOF_MESSAGE_BUF - 1); pos++)
        memcpy(&vMQTTSN.MsgBuf[pos], &vMQTTSN.MsgBuf[pos + 1], sizeof(MQ_MSG_t));
      vMQTTSN.MsgBuf[MQTTSN_SIZEOF_MESSAGE_BUF - 1].MsgType = 0xFF;

      vMQTTSN.pfCnt = 0;
      vMQTTSN.Tretry = 0;
      vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
      return true;
    }
  }
  return false;
}

// Parse incoming messages from PHY1
static void mqttsn_parser_task(void *pvParameters)
{
  MQ_t * pPHY1outBuf;

  while(1)
  {
mqttsn_parser_task_lbl1:
    PHY1_Get(&pPHY1outBuf);

    e_MQTTSN_MSGTYPE_t MsgType = pPHY1outBuf->mq.MsgType;
    
    // Gateway Info message
    if(MsgType == MQTTSN_MSGTYP_GWINFO)
    {
      if(vMQTTSN.Status == MQTTSN_STATUS_SEARCHGW)
      {
        memcpy(vMQTTSN.GatewayAddr, pPHY1outBuf->phy1addr, sizeof(PHY1_ADDR_t));
        vMQTTSN.GwId = pPHY1outBuf->mq.gwinfo.GwId;
        vMQTTSN.Status = MQTTSN_STATUS_OFFLINE;
        vMQTTSN.Tretry = 0;
        vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
      }
    }
    // Disconnect Request
    else if(MsgType == MQTTSN_MSGTYP_DISCONNECT)
    {
      if(vMQTTSN.Status == MQTTSN_STATUS_SEARCHGW)
        vMQTTSN.Radius = 0;
      else
        vMQTTSN.Status = MQTTSN_STATUS_DISCONNECTED;

      vMQTTSN.pfCnt = 0;
      vMQTTSN.Tretry = 0;
    }
    // Message from gateway
    else if(memcmp(pPHY1outBuf->phy1addr, vMQTTSN.GatewayAddr, sizeof(PHY1_ADDR_t)) == 0)
    {
      switch(MsgType)
      {
        // Connack message
        case MQTTSN_MSGTYP_CONNACK:
          if(vMQTTSN.Status == MQTTSN_STATUS_OFFLINE)
          {
            if(pPHY1outBuf->mq.connack.ReturnCode == MQTTSN_RET_ACCEPTED)
            {
              vMQTTSN.Status = MQTTSN_STATUS_PRE_CONNECT;
              vMQTTSN.pfCnt = 0;
              vMQTTSN.Tretry = 0;
              vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
            }
          }
          // else
          // Message lost, broker - gateway Problems, Connected another Node with same Address.
          // Potential dangerous
          break;
        // Register Topic request
        case MQTTSN_MSGTYP_REGISTER:
          if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
          {
            pPHY1outBuf->mq.regack.ReturnCode = RegisterOD(&pPHY1outBuf->mq);
            pPHY1outBuf->Length = MQTTSN_SIZEOF_MSG_REGACK;
            pPHY1outBuf->mq.Length = MQTTSN_SIZEOF_MSG_REGACK;
            pPHY1outBuf->mq.MsgType = MQTTSN_MSGTYP_REGACK;
            PHY1_Send(pPHY1outBuf);
            goto mqttsn_parser_task_lbl1;
          }
          break;
        // RegAck Answer
        case MQTTSN_MSGTYP_REGACK:
          if(vMQTTSN.Status == MQTTSN_STATUS_PRE_CONNECT)
          {
            uint16_t MsgId = (pPHY1outBuf->mq.regack.MsgId[0]<<8) |
                              pPHY1outBuf->mq.regack.MsgId[1];
            if(mqttsn_pack_msgbuf(MQTTSN_MSGTYP_REGISTER, MsgId))
            {
              uint16_t index;
              if(pPHY1outBuf->mq.regack.ReturnCode == MQTTSN_RET_ACCEPTED)
                index = (pPHY1outBuf->mq.regack.TopicId[0]<<8) |
                         pPHY1outBuf->mq.regack.TopicId[1];
              else
                index = 0;
              RegAckOD(index);
            }
          }
          break;
        // Publish Topic request
        case MQTTSN_MSGTYP_PUBLISH:
          if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
          {
            uint8_t Flags = pPHY1outBuf->mq.publish.Flags;
            uint16_t TopicId = (pPHY1outBuf->mq.publish.TopicId[0]<<8) |
                               pPHY1outBuf->mq.publish.TopicId[1];
            uint16_t MsgId = (pPHY1outBuf->mq.publish.MsgId[0]<<8) |
                               pPHY1outBuf->mq.publish.MsgId[1];
            // Make PubAck message
            pPHY1outBuf->mq.puback.ReturnCode = WriteODpack(
                              TopicId,
                              Flags, 
                              (pPHY1outBuf->mq.Length - MQTTSN_SIZEOF_MSG_PUBLISH),
                              (uint8_t *)pPHY1outBuf->mq.publish.Data);
            if((Flags & MQTTSN_FL_QOS_MASK) == MQTTSN_FL_QOS1)           // Need Ack
            {
              pPHY1outBuf->Length = MQTTSN_SIZEOF_MSG_PUBACK;
              pPHY1outBuf->mq.Length = MQTTSN_SIZEOF_MSG_PUBACK;
              pPHY1outBuf->mq.MsgType = MQTTSN_MSGTYP_PUBACK;
            
              pPHY1outBuf->mq.puback.TopicId[0] = TopicId>>8;
              pPHY1outBuf->mq.puback.TopicId[1] = TopicId & 0xFF;
              pPHY1outBuf->mq.puback.MsgId[0] = MsgId>>8;
              pPHY1outBuf->mq.puback.MsgId[1] = MsgId & 0xFF;
              PHY1_Send(pPHY1outBuf);
              goto mqttsn_parser_task_lbl1;
            }
          }
          break;
        // PubAck Answer
        case MQTTSN_MSGTYP_PUBACK:
          if((vMQTTSN.Status == MQTTSN_STATUS_PRE_CONNECT) || 
             (vMQTTSN.Status == MQTTSN_STATUS_CONNECT))
          {
            uint16_t MsgId = (pPHY1outBuf->mq.puback.MsgId[0]<<8) |
                                pPHY1outBuf->mq.puback.MsgId[1];
            mqttsn_pack_msgbuf(MQTTSN_MSGTYP_PUBLISH, MsgId);
          }
          break;
        // SubAck answer
        case MQTTSN_MSGTYP_SUBACK:
          if(vMQTTSN.Status == MQTTSN_STATUS_PRE_CONNECT)
          {
            uint16_t MsgId = (pPHY1outBuf->mq.suback.MsgId[0]<<8) |
                              pPHY1outBuf->mq.suback.MsgId[1];
            if(mqttsn_pack_msgbuf(MQTTSN_MSGTYP_SUBSCRIBE, MsgId))
              vMQTTSN.Status = MQTTSN_STATUS_CONNECT;
          }
          break;
        // Ping Response
        case MQTTSN_MSGTYP_PINGRESP:
          if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
          {
            vMQTTSN.pfCnt = (POLL_TMR_FREQ - 1);
            vMQTTSN.Tretry = MQTTSN_DEF_KEEPALIVE;
            vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
          }
          break;
        case MQTTSN_MSGTYP_FORWARD:
          if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
          {
            uint8_t Length = pPHY1outBuf->mq.Length;
#ifdef PHY2_Send
            // Direction Gateway to PHY2
            if(Length == (MQTTSN_SIZEOF_MSG_FORWARD + sizeof(PHY2_ADDR_t)))
            {
              memcpy(pPHY1outBuf->phy2addr, pPHY1outBuf->mq.forward.wNodeID, sizeof(PHY2_ADDR_t));
              // truncate header
              pPHY1outBuf->Length -= Length;
              memcpy(&pPHY1outBuf->raw[0], &pPHY1outBuf->raw[Length], pPHY1outBuf->Length);
              PHY2_Send(pPHY1outBuf);
              goto mqttsn_parser_task_lbl1;
            }
            else
#endif  //  PHY2_Send
              // Direction: Gateway to Remote node on PHY1
            if(Length == (MQTTSN_SIZEOF_MSG_FORWARD + sizeof(PHY1_ADDR_t)))
            {
              memcpy(pPHY1outBuf->phy1addr, pPHY1outBuf->mq.forward.wNodeID, sizeof(PHY1_ADDR_t));
              // truncate header
              pPHY1outBuf->Length -= Length;
              memcpy(&pPHY1outBuf->raw[0], &pPHY1outBuf->raw[Length], pPHY1outBuf->Length);
              PHY1_Send(pPHY1outBuf);
              goto mqttsn_parser_task_lbl1;
            }
          }
          break;
        // Unknown message type
        default:
          break;
      }
    }
    else if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
    {
      // Gateway Info request on PHY1 from Remote Node
      if(MsgType == MQTTSN_MSGTYP_SEARCHGW)
      {
        if(pPHY1outBuf->mq.searchgw.Radius == (vMQTTSN.Radius + 1))
        {
          // Send Gateway Info message
          PHY1_ADDR_t s_addr = ADDR_BROADCAST_PHY1;
          memcpy(pPHY1outBuf->phy1addr, &s_addr, sizeof(PHY1_ADDR_t));
          pPHY1outBuf->Length = (MQTTSN_SIZEOF_MSG_GWINFO + sizeof(PHY1_ADDR_t));
          pPHY1outBuf->mq.Length = (MQTTSN_SIZEOF_MSG_GWINFO + sizeof(PHY1_ADDR_t));
          pPHY1outBuf->mq.MsgType = MQTTSN_MSGTYP_GWINFO;
          pPHY1outBuf->mq.gwinfo.GwId = vMQTTSN.GwId;
          memcpy(pPHY1outBuf->mq.gwinfo.GwAdd, vMQTTSN.NodeAddrPhy1, sizeof(PHY1_ADDR_t));
          PHY1_Send(pPHY1outBuf);
          goto mqttsn_parser_task_lbl1;
        }
      }
      else
      {
        // Forward message on PHY1 from Remote Node to Gateway
        uint8_t Length = pPHY1outBuf->Length;
        uint8_t Size = MQTTSN_SIZEOF_MSG_FORWARD + sizeof(PHY1_ADDR_t);
        uint8_t pos;
        if((Length + Size) <= sizeof(MQTTSN_MESSAGE_t))
        {
          for(pos = (Size + Length - 1); pos >= Size; pos--)
            pPHY1outBuf->raw[pos] = pPHY1outBuf->raw[pos - Size];

          // Make forward message
          pPHY1outBuf->Length += Size;
          pPHY1outBuf->mq.Length = Size;
          pPHY1outBuf->mq.MsgType = MQTTSN_MSGTYP_FORWARD;
          pPHY1outBuf->mq.forward.Ctrl = 0;   // ?? TTL
          memcpy(pPHY1outBuf->mq.forward.wNodeID, pPHY1outBuf->phy1addr, sizeof(PHY1_ADDR_t));
          memcpy(pPHY1outBuf->phy1addr, vMQTTSN.GatewayAddr, sizeof(PHY1_ADDR_t));
          PHY1_Send(pPHY1outBuf);
          goto mqttsn_parser_task_lbl1;
        }
      }
    }
    vPortFree(pPHY1outBuf);
  }
  vTaskDelete(NULL);
}

// Parse Incoming messages from PHY2
#ifdef PHY2_Send
static void mqttsn_forwarder_task(void *pvParameters)
{
  MQ_t        * pPHY2outBuf;
  
  while(1)
  {
mqttsn_forwarder_task_lbl1:
    PHY2_Get(&pPHY2outBuf);

    if(vMQTTSN.Status == MQTTSN_STATUS_CONNECT)
    {
      if(pPHY2outBuf->mq.MsgType == MQTTSN_MSGTYP_SEARCHGW)
      {
        if(pPHY2outBuf->mq.searchgw.Radius == vMQTTSN.Radius)
        {
          // Send Gateway Info message
          PHY2_ADDR_t s_addr = ADDR_BROADCAST_PHY2;
          memcpy(pPHY2outBuf->phy2addr, &s_addr, sizeof(PHY2_ADDR_t));
          uint8_t Length = MQTTSN_SIZEOF_MSG_GWINFO;

          pPHY2outBuf->mq.MsgType = MQTTSN_MSGTYP_GWINFO;
          pPHY2outBuf->mq.gwinfo.GwId = vMQTTSN.GwId;
          if(vMQTTSN.Radius > 0)
          {
            memcpy(pPHY2outBuf->mq.gwinfo.GwAdd, vMQTTSN.NodeAddrPhy2, sizeof(PHY2_ADDR_t));
            Length += sizeof(PHY2_ADDR_t);
          }

          pPHY2outBuf->Length = Length;
          pPHY2outBuf->mq.Length = Length;
          PHY2_Send(pPHY2outBuf);
          goto mqttsn_forwarder_task_lbl1;
        }
      }
      else  // Encapulate message to Forward Packet and send to Gateway
      {
        uint8_t Length = pPHY2outBuf->Length;
        uint8_t Size = MQTTSN_SIZEOF_MSG_FORWARD + sizeof(PHY2_ADDR_t);
        uint8_t pos;
        if((Length + Size) <= sizeof(MQTTSN_MESSAGE_t))
        {
          for(pos = (Size + Length - 1); pos >= Size; pos--)
            pPHY2outBuf->raw[pos] = pPHY2outBuf->raw[pos - Size];

          // Make forward message
          pPHY2outBuf->Length += Size;
          pPHY2outBuf->mq.Length = Size;
          pPHY2outBuf->mq.MsgType = MQTTSN_MSGTYP_FORWARD;
          pPHY2outBuf->mq.forward.Ctrl = 0;   // ?? TTL
          memcpy(pPHY2outBuf->mq.forward.wNodeID, pPHY2outBuf->phy2addr, sizeof(PHY2_ADDR_t));
          memcpy(pPHY2outBuf->phy1addr, vMQTTSN.GatewayAddr, sizeof(PHY1_ADDR_t));
          PHY1_Send(pPHY2outBuf);
          goto mqttsn_forwarder_task_lbl1;
        }
      }
    }
    vPortFree(pPHY2outBuf);
  }
  vTaskDelete(NULL);
}
#endif  //  PHY2_Send

// End parse incoming messages
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Start Poll Task

static void mqttsn_disconnect(void)
{
  uint8_t uTmp = sizeof(PHY1_ADDR_t);
  ReadOD(PHY1_GateId, MQTTSN_FL_TOPICID_PREDEF, &uTmp, (uint8_t *)vMQTTSN.GatewayAddr);
  ReadOD(PHY1_NodeId, MQTTSN_FL_TOPICID_PREDEF, &uTmp, (uint8_t *)vMQTTSN.NodeAddrPhy1);
#ifdef PHY2_ADDR_t
  uTmp = sizeof(PHY2_ADDR_t);
  ReadOD(PHY2_NodeId, MQTTSN_FL_TOPICID_PREDEF, &uTmp, (uint8_t *)vMQTTSN.NodeAddrPhy2);
#endif  //  PHY2_ADDR_t
  
  PHY1_ADDR_t addr_undef = ADDR_UNDEF_PHY1;
  if(memcmp(vMQTTSN.GatewayAddr, &addr_undef, sizeof(PHY1_ADDR_t)) == 0)
    vMQTTSN.Status = MQTTSN_STATUS_SEARCHGW;
  else
    vMQTTSN.Status = MQTTSN_STATUS_OFFLINE;

  vMQTTSN.GwId = 0;
  vMQTTSN.Radius = 0;
  vMQTTSN.pfCnt = 0;
  vMQTTSN.Tretry = 0;
  vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
  
  // Clear publish buffer
  for(uTmp = 0; uTmp < MQTTSN_SIZEOF_MESSAGE_BUF; uTmp++)
    vMQTTSN.MsgBuf[uTmp].MsgType = 0xFF;
    
  vMQTTSN.MsgId = 0;
}

// Send Search Gateway message
static void mqttsn_send_searchgw(void)
{
  if(vMQTTSN.Nretry > 0)
    vMQTTSN.Nretry--;
  else
  {
    if(vMQTTSN.Radius < MQTTSN_MAX_RADIUS)
    {
      vMQTTSN.Radius++;
      vMQTTSN.Nretry = (MQTTSN_DEF_NRETRY - 1);
    }
    else
    {
      vMQTTSN.Tretry = (MQTTSN_DEF_TDISCONNECT - MQTTSN_DEF_TSGW);
      vMQTTSN.Status = MQTTSN_STATUS_DISCONNECTED;
      return;
    }
  }
  vMQTTSN.Tretry = MQTTSN_DEF_TSGW;

  MQ_t * pSearchGW = pvPortMalloc(sizeof(MQ_t));
  if(pSearchGW == NULL)
    return;

  PHY1_ADDR_t dst_addr = ADDR_BROADCAST_PHY1;                // Broadcast
  memcpy(pSearchGW->phy1addr, &dst_addr, sizeof(PHY1_ADDR_t));
  pSearchGW->Length = MQTTSN_SIZEOF_MSG_SEARCHGW;
  pSearchGW->mq.Length = MQTTSN_SIZEOF_MSG_SEARCHGW;
  pSearchGW->mq.MsgType = MQTTSN_MSGTYP_SEARCHGW;
  pSearchGW->mq.searchgw.Radius = vMQTTSN.Radius;
  
  PHY1_Send(pSearchGW);
}

// Send Connect Message
static void mqttsn_send_connect(void)
{
  if(vMQTTSN.Nretry > 0)
    vMQTTSN.Nretry--;
  else
  {
    vMQTTSN.Radius = 0;
    vMQTTSN.Tretry = (MQTTSN_DEF_TSGW - MQTTSN_DEF_TCONNECT);
    vMQTTSN.Nretry = MQTTSN_DEF_NRETRY;
    vMQTTSN.Status = MQTTSN_STATUS_SEARCHGW;
    return;
  }
  vMQTTSN.Tretry = MQTTSN_DEF_TCONNECT;

  MQ_t * pConnect = pvPortMalloc(sizeof(MQ_t));
  if(pConnect == NULL)
    return;

  pConnect->mq.MsgType = MQTTSN_MSGTYP_CONNECT;
  pConnect->mq.connect.Flags = MQTTSN_FL_CLEANSESSION;
  pConnect->mq.connect.ProtocolId = MQTTSN_DEF_PROTOCOLID;
  pConnect->mq.connect.Duration[0] = (MQTTSN_DEF_KEEPALIVE>>8);
  pConnect->mq.connect.Duration[1] = (MQTTSN_DEF_KEEPALIVE & 0xFF);

  // Build Name
  uint8_t Length = MQTTSN_SIZEOF_CLIENTID;
  ReadOD(objNodeName, MQTTSN_FL_TOPICID_PREDEF, &Length, (uint8_t *)&pConnect->mq.connect.ClientId);
  if(Length < 2)  // Node Name not defined, use default name
  {
    uint8_t pos, ch;
    uint8_t * pBuf;
    pBuf = (uint8_t *)&pConnect->mq.connect.ClientId;
  
    Length = OD_DEV_TYP_LEN;
    ReadOD(objDeviceTyp, MQTTSN_FL_TOPICID_PREDEF, &Length, pBuf);
    pBuf += Length;
    *(pBuf++) = '_';
    Length++;

    for(pos = 0; pos < sizeof(PHY1_ADDR_t); pos++)
    {
      ch = vMQTTSN.NodeAddrPhy1[pos]>>4;
      if(ch > 0x09)
        ch += 0x37;
      else
        ch += 0x30;

      *pBuf = ch;
      pBuf++;
    
      ch = vMQTTSN.NodeAddrPhy1[pos] & 0x0F;
      if(ch > 0x09)
        ch += 0x37;
      else
        ch += 0x30;

      *pBuf = ch;
      pBuf++;
    }
    Length += (sizeof(PHY1_ADDR_t)*2);
  }

  Length += MQTTSN_SIZEOF_MSG_CONNECT;
  pConnect->Length = Length;
  pConnect->mq.Length = Length;
  memcpy(pConnect->phy1addr, vMQTTSN.GatewayAddr, sizeof(PHY1_ADDR_t));
  PHY1_Send(pConnect);
}

// Get new outgoing message ID
static uint16_t mqttsn_new_msgid(void)
{
  vMQTTSN.MsgId++;
  if(vMQTTSN.MsgId > 0xFFFE)
    vMQTTSN.MsgId = 1;
  return vMQTTSN.MsgId;
}

static void mqttsn_send_message(void)
{
  if(vMQTTSN.Nretry > 0)
    vMQTTSN.Nretry--;
  else
  {
    mqttsn_disconnect();
    return;
  }

  MQ_t * pMessage;
  uint8_t Length;
    
  uint8_t pos;
  for(pos = 0; pos < MQTTSN_SIZEOF_MESSAGE_BUF; pos++)
  {
    e_MQTTSN_MSGTYPE_t MsgType = vMQTTSN.MsgBuf[pos].MsgType;
    
    // Make Publish message
    if(MsgType == MQTTSN_MSGTYP_PUBLISH)
    {
      pMessage = pvPortMalloc(sizeof(MQ_t));
      if(pMessage == NULL)
        return;
        
      if(vMQTTSN.MsgBuf[pos].MsgId == 0)
        vMQTTSN.MsgBuf[pos].MsgId = mqttsn_new_msgid();
      else
        vMQTTSN.MsgBuf[pos].Flags |= MQTTSN_FL_DUP;

      pMessage->mq.MsgType = MQTTSN_MSGTYP_PUBLISH;
      pMessage->mq.publish.Flags = vMQTTSN.MsgBuf[pos].Flags;
      pMessage->mq.publish.TopicId[0] = vMQTTSN.MsgBuf[pos].TopicId>>8;
      pMessage->mq.publish.TopicId[1] = vMQTTSN.MsgBuf[pos].TopicId & 0xFF;
      pMessage->mq.publish.MsgId[0] = vMQTTSN.MsgBuf[pos].MsgId>>8;
      pMessage->mq.publish.MsgId[1] = vMQTTSN.MsgBuf[pos].MsgId & 0xFF;
      Length = (MQTTSN_MSG_SIZE - 5);
      ReadODpack(vMQTTSN.MsgBuf[pos].TopicId, vMQTTSN.MsgBuf[pos].Flags, &Length, pMessage->mq.publish.Data);
      Length += MQTTSN_SIZEOF_MSG_PUBLISH;
    }
    // Make Register message
    else if(MsgType == MQTTSN_MSGTYP_REGISTER)
    {
      pMessage = pvPortMalloc(sizeof(MQ_t));
      if(pMessage == NULL)
        return;

      pMessage->mq.MsgType = MQTTSN_MSGTYP_REGISTER;
      pMessage->mq.regist.TopicId[0] = vMQTTSN.MsgBuf[pos].TopicId>>8;
      pMessage->mq.regist.TopicId[1] = vMQTTSN.MsgBuf[pos].TopicId & 0xFF;
      if(vMQTTSN.MsgBuf[pos].MsgId == 0)
        vMQTTSN.MsgBuf[pos].MsgId = mqttsn_new_msgid();
      pMessage->mq.regist.MsgId[0] = vMQTTSN.MsgBuf[pos].MsgId>>8;
      pMessage->mq.regist.MsgId[1] = vMQTTSN.MsgBuf[pos].MsgId & 0xFF;
      Length = MakeTopicName(vMQTTSN.MsgBuf[pos].Flags, pMessage->mq.regist.TopicName);
      Length += MQTTSN_SIZEOF_MSG_REGISTER;
    }
    // Make Subscribe message
    else if(MsgType == MQTTSN_MSGTYP_SUBSCRIBE)
    {
      pMessage = pvPortMalloc(sizeof(MQ_t));
      if(pMessage == NULL)
        return;
        
      pMessage->mq.MsgType = MQTTSN_MSGTYP_SUBSCRIBE;
      if(vMQTTSN.MsgBuf[pos].MsgId == 0)
      {
        vMQTTSN.MsgBuf[pos].MsgId = mqttsn_new_msgid();
        pMessage->mq.subscribe.Flags = (MQTTSN_FL_QOS1 | MQTTSN_FL_TOPICID_NORM);
      }
      else
        pMessage->mq.subscribe.Flags = (MQTTSN_FL_DUP | MQTTSN_FL_QOS1 | MQTTSN_FL_TOPICID_NORM);
        
      pMessage->mq.subscribe.MsgId[0] = vMQTTSN.MsgBuf[pos].MsgId>>8;
      pMessage->mq.subscribe.MsgId[1] = vMQTTSN.MsgBuf[pos].MsgId & 0xFF;
      pMessage->mq.subscribe.Topic[0] = '#';
      Length = (MQTTSN_SIZEOF_MSG_SUBSCRIBE + 1);
    }
    // No messages, send PingReq
    else if(pos == 0)
    {
      pMessage = pvPortMalloc(sizeof(MQ_t));
      if(pMessage == NULL)
        return;

      Length = MQTTSN_SIZEOF_MSG_PINGREQ;
      pMessage->mq.MsgType = MQTTSN_MSGTYP_PINGREQ;
      vMQTTSN.Tretry = MQTTSN_DEF_KEEPALIVE;
    }
    else
      return;

    pMessage->Length = Length;
    pMessage->mq.Length = Length;
    memcpy(pMessage->phy1addr, vMQTTSN.GatewayAddr, sizeof(PHY1_ADDR_t));
    PHY1_Send(pMessage);
  }
}

static void mqttsn_poll_task(void *pvParameters)
{
  // Initialise the xLastWakeTime variable with the current time.
  TickType_t xLastWakeTime    = xTaskGetTickCount();
  const TickType_t xFrequency = POLL_TMR_FREQ;

  while(1)
  {
mqttsn_poll_task_lbl1:
    // Wait for the next cycle.
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    
    if(vMQTTSN.pfCnt)
    {
      vMQTTSN.pfCnt--;
      goto mqttsn_poll_task_lbl1;
    }
    vMQTTSN.pfCnt = (POLL_TMR_FREQ - 1);
    
    if(vMQTTSN.Tretry)
    {
      vMQTTSN.Tretry--;
      if(vMQTTSN.Tretry != 0)
        goto mqttsn_poll_task_lbl1;
    }
    
    switch(vMQTTSN.Status)
    {
      case MQTTSN_STATUS_DISCONNECTED:
        mqttsn_disconnect();
        break;
      case MQTTSN_STATUS_SEARCHGW:
        mqttsn_send_searchgw();
        break;
      case MQTTSN_STATUS_OFFLINE:
        mqttsn_send_connect();
        break;
      case MQTTSN_STATUS_PRE_CONNECT:
      case MQTTSN_STATUS_CONNECT:
        mqttsn_send_message();
        break;
      default:
        break;
    }
  }
  vTaskDelete(NULL);
}

// End Poll Task
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// API

// Initialise MQTTSN tasks
void MQTTSN_Init(void)
{
  mqttsn_disconnect();
  vMQTTSN.pfCnt  = (POLL_TMR_FREQ - 1);

  xTaskCreate(mqttsn_parser_task, "mq1", configMINIMAL_STACK_SIZE + 50, NULL, tskIDLE_PRIORITY + 1, NULL );
  xTaskCreate(mqttsn_poll_task, "mq2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
  
#ifdef PHY2_Send
  xTaskCreate(mqttsn_forwarder_task, "mq3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
#endif  //  PHY2_Send
}

// Get MQTTSN Status
e_MQTTSN_STATUS_t MQTTSN_GetStatus(void)
{
  return vMQTTSN.Status;
}

void MQTTSN_EnqueueMsg(MQ_MSG_t * pMqMsg)
{
  uint8_t pos;
  pMqMsg->MsgId = 0;
  for(pos = 0; pos < MQTTSN_SIZEOF_MESSAGE_BUF; pos++)
  {
    if(vMQTTSN.MsgBuf[pos].MsgType == 0xFF)
    {
      memcpy(&vMQTTSN.MsgBuf[pos], pMqMsg, sizeof(MQ_MSG_t));
      if(pos == 0)
      {
        vMQTTSN.pfCnt = 0;
        vMQTTSN.Tretry = 0;
      }
      return;
    }

    if(memcmp(&vMQTTSN.MsgBuf[pos], pMqMsg, sizeof(MQ_MSG_t) - sizeof(uint16_t)) == 0)
      break;
  }
}
