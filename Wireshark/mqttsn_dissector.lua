--  Copyright (c) 2011-2014 <comparator@gmx.de>
--  This file is part of the X13.Home project.
--  http://X13home.org
--  http://X13home.net
--  http://X13home.github.io/
--
--  BSD New License
--  See LICENSE file for license details.
--
--  MQTTS-SN Parser
--  Please add to end of file "$WireasharkHome/init.lua" follow lines
--    MQTTSN_PLUGIN_PATH = "DISK:\\Path\\To_The_Plugin\\"
--    dofile(MQTTSN_PLUGIN_PATH.."mqttsn_dissector.lua")
--

p_mqttsn = Proto("mqttsn", "MQTT-SN")    -- Определение нового протокола

-- Текстовые отображения некоторых типов данных
packettypes =   { [0] = "Advertise", "SEARCHGW","GWINFO","Undef_03","CONNECT","CONNACK","WILLTOPICREQ","WILLTOPIC",
                        "WILLMSGREQ","WILLMSG","REGISTER","REGACK","PUBLISH","PUBACK","PUBCOMP","PUBREC",
                        "PUBREL","Undef_11","SUBSCRIBE","SUBACK","UNSUBSCRIBE","UNSUBACK","PINGREQ","PINGRESP",
                        "DISCONNECT","Undef_19","WILLTOPICUPD","WILLTOPICRESP","WILLMSGUPD","WILLMSGRESP", "Undef_1E", "Undef_1F",
                        "Undef_20","Undef_21","Undef_22","Undef_23","Undef_24","Undef_25","Undef_26","Undef_27",
                        "Undef_28","Undef_29","Undef_2A","Undef_2B","Undef_2C","Undef_2D","Undef_2E","Undef_2F",
                        "Undef_30","Undef_31","Undef_32","Undef_33","Undef_34","Undef_35","Undef_36","Undef_37",
                        "Undef_38","Undef_39","Undef_3A","Undef_3B","Undef_3C","Undef_3D","Undef_3E","Undef_3F",
                        "Undef_40","Undef_41","Undef_42","Undef_43","Undef_44","Undef_45","Undef_46","Undef_47",
                        "Undef_48","Undef_49","Undef_4A","Undef_4B","Undef_4C","Undef_4D","Undef_4E","Undef_4F",
                        "Undef_50","Undef_51","Undef_52","Undef_53","Undef_54","Undef_55","Undef_56","Undef_57",
                        "Undef_58","Undef_59","Undef_5A","Undef_5B","Undef_5C","Undef_5D","Undef_5E","Undef_5F",
                        "Undef_60","Undef_61","Undef_62","Undef_63","Undef_64","Undef_65","Undef_66","Undef_67",
                        "Undef_68","Undef_69","Undef_6A","Undef_6B","Undef_6C","Undef_6D","Undef_6E","Undef_6F",
                        "Undef_70","Undef_71","Undef_72","Undef_73","Undef_74","Undef_75","Undef_76","Undef_77",
                        "Undef_78","Undef_79","Undef_7A","Undef_7B","Undef_7C","Undef_7D","Undef_7E","Undef_7F",
                        "Undef_80","Undef_81","Undef_82","Undef_83","Undef_84","Undef_85","Undef_86","Undef_87",
                        "Undef_88","Undef_89","Undef_8A","Undef_8B","Undef_8C","Undef_8D","Undef_8E","Undef_8F",
                        "Undef_90","Undef_91","Undef_92","Undef_93","Undef_94","Undef_95","Undef_96","Undef_97",
                        "Undef_98","Undef_99","Undef_9A","Undef_9B","Undef_9C","Undef_9D","Undef_9E","Undef_9F",
                        "Undef_A0","Undef_A1","Undef_A2","Undef_A3","Undef_A4","Undef_A5","Undef_A6","Undef_A7",
                        "Undef_A8","Undef_A9","Undef_AA","Undef_AB","Undef_AC","Undef_AD","Undef_AE","Undef_AF",
                        "Undef_B0","Undef_B1","Undef_B2","Undef_B3","Undef_B4","Undef_B5","Undef_B6","Undef_B7",
                        "Undef_B8","Undef_B9","Undef_BA","Undef_BB","Undef_BC","Undef_BD","Undef_BE","Undef_BF",
                        "Undef_C0","Undef_C1","Undef_C2","Undef_C3","Undef_C4","Undef_C5","Undef_C6","Undef_C7",
                        "Undef_C8","Undef_C9","Undef_CA","Undef_CB","Undef_CC","Undef_CD","Undef_CE","Undef_CF",
                        "Undef_D0","Undef_D1","Undef_D2","Undef_D3","Undef_D4","Undef_D5","Undef_D6","Undef_D7",
                        "Undef_D8","Undef_D9","Undef_DA","Undef_DB","Undef_DC","Undef_DD","Undef_DE","Undef_DF",
                        "Undef_E0","Undef_E1","Undef_E2","Undef_E3","Undef_E4","Undef_E5","Undef_E6","Undef_E7",
                        "Undef_E8","Undef_E9","Undef_EA","Undef_EB","Undef_EC","Undef_ED","Undef_EE","Undef_EF",
                        "Undef_F0","Undef_F1","Undef_F2","Undef_F3","Undef_F4","Undef_F5","Undef_F6","Undef_F7",
                        "Undef_F8","Undef_F9","Undef_FA","Undef_FB","Undef_FC","Undef_FD","Forward","Undef_FF"}

packetreturn =  { [0] = "Ok", "congestion", "invalid topic ID", "not supported" }
packetbool  =   { [0] = "False",  "True" }
packetqos   =   { [0] = "QOS0", "QOS1", "QOS2", "QOS-1" }
packettopicid = { [0] = "Normal", "Pre Defined", "Short", "NOT USED" }

-- Заголовки протокола
local mqttsn_len        = ProtoField.uint8("mqttsn.len",        "Len",            base.DEC)
local mqttsn_type       = ProtoField.uint8("mqttsn.type",       "Type",           base.DEC, packettypes)
local mqttsn_m          = ProtoField.bytes("mqttsn.m",          "RAW")

local mqttsn_gateid     = ProtoField.uint8("mqttsn.m.gateid",   "Gateway ID",     base.DEC)
local mqttsn_gwaddr     = ProtoField.bytes("mqttsn.m.gwaddr",    "Gateway Address")

local mqttsn_radius     = ProtoField.uint8("mqttsn.m.radius",   "Radius",         base.DEC)
local mqttsn_protid     = ProtoField.uint8("mqttsn.m.protid",   "Protocol ID",    base.DEC)
local mqttsn_duration   = ProtoField.uint16("mqttsn.m.duration","Duration",       base.DEC)
local mqttsn_clientid   = ProtoField.string("mqttsn.m.clientid","ClientID",       base.STRING)
local mqttsn_topicname  = ProtoField.string("mqttsn.m.topicname","TopicName",     base.STRING)

local mqttsn_flag       = ProtoField.uint8("mqttsn.m.flags",    "Flags",          base.HEX)
local mqttsn_flag_dup   = ProtoField.uint8("mqttsn.m.flags.dup", "Dup",           base.DEC, packetbool, 0x80)
local mqttsn_flag_qos   = ProtoField.uint8("mqttsn.m.flags.qos", "QOS",           base.DEC, packetqos,  0x60)
local mqttsn_flag_retain = ProtoField.uint8("mqttsn.m.flags.retain", "Retain",    base.DEC, packetbool, 0x10)
local mqttsn_flag_will  = ProtoField.uint8("mqttsn.m.flags.will", "Will",         base.DEC, packetbool, 0x08)
local mqttsn_flag_clrs  = ProtoField.uint8("mqttsn.m.flags.clrs", "Clear Session",base.DEC, packetbool, 0x04)
local mqttsn_flag_tid   = ProtoField.uint8("mqttsn.m.flags.tid", "Topic ID",      base.DEC, packettopicid, 0x03)

local mqttsn_topicid    = ProtoField.uint16("mqttsn.m.topicid", "TopicID",        base.DEC)
local mqttsn_msgid      = ProtoField.uint16("mqttsn.m.msgid",   "MsgId",          base.DEC)
local mqttsn_data       = ProtoField.bytes("mqttsn.m.data",     "Data")
local mqttsn_return     = ProtoField.uint8("mqttsn.m.return",   "Return",         base.DEC, packetreturn)

local mqttsn_wnodeid    = ProtoField.bytes("mqttsn.m.wnodeid",  "Wireless Node ID")

-- Регистрируем все поля протокола
p_mqttsn.fields = { mqttsn_len, mqttsn_type, mqttsn_m, 
                    mqttsn_gateid, mqttsn_gwaddr, mqttsn_radius, mqttsn_protid, mqttsn_duration,
                    mqttsn_clientid, mqttsn_topicname, mqttsn_flag, mqttsn_flag_dup, mqttsn_flag_qos,
                    mqttsn_flag_retain, mqttsn_flag_will, mqttsn_flag_clrs, mqttsn_flag_tid,
                    mqttsn_topicid, mqttsn_msgid, mqttsn_data, mqttsn_return, mqttsn_wnodeid
                  }

-- Собственно функция диссектора для протокола MQTTSN
function p_mqttsn.dissector(buf, pinfo, tree)
    local m_len = buf:len()
    if m_len < 2 then return end

    pinfo.cols.protocol = p_mqttsn.name
    
    local m_pnt = 0
    local length
    local type_str
    
    while m_pnt < m_len do
      length = buf(m_pnt,1):uint()
      type_str = mqttsn_parse_message(buf(m_pnt, length), tree)
      m_pnt = m_pnt + length
    end
    
    pinfo.cols.info = type_str                -- в колонке Info будет отображаться тип пакета

end

function mqttsn_parse_message(buf, tree)
    subtree = tree:add(p_mqttsn, buf())       -- создаем поддерево

    subtree:add(mqttsn_len, buf(0,1))         -- начинаем добавлять поля
    subtree:add(mqttsn_type,  buf(1,1))

    local length = buf(0,1):uint()
    local msg_type = buf(1,1):uint()

    local type_str ="unknown"

    if msg_type == 0 and length == 5 then                           -- Advertise message
      subtree:add(mqttsn_gateid,    buf(2,1))
      subtree:add(mqttsn_duration,  buf(3,2))
      type_str = "Advertise, Gateway ID: " .. buf(2,1):uint() .. " Duration: " .. buf(3,2):uint()
    elseif msg_type == 1 and length == 3 then                       -- Search Gateway request
      subtree:add(mqttsn_radius,    buf(2,1))
      type_str = "Search Gateway"
    elseif msg_type == 2 and length > 2 then                        -- Gateway Info
      subtree:add(mqttsn_gateid,    buf(2,1))
      if length > 3 then subtree:add(mqttsn_gwaddr, buf(2,length - 2)) end
      type_str = "Gateway Info"
    elseif msg_type == 4 and length > 6 then                        -- parse connect request
      mqtts_expand_flags(subtree, buf, 2)
      subtree:add(mqttsn_protid,    buf(3,1))
      subtree:add(mqttsn_duration,  buf(4,2))
      subtree:add(mqttsn_clientid,  buf(6, length - 6))
      type_str = "Connect: " .. buf(6, length - 6):string()
    elseif msg_type == 5  and length == 3 then                      -- connack answer
      subtree:add(mqttsn_return, buf(2,1))
      if buf(2,1):uint() == 0 then type_str = "ConnAck" else type_str = "ConnAck, Error: " .. buf(2,1):uint() end
    elseif msg_type == 10 and length > 6 then                       -- parse register request
      subtree:add(mqttsn_topicid,   buf(2,2))
      subtree:add(mqttsn_msgid,     buf(4,2))
      subtree:add(mqttsn_topicname, buf(6,length - 6))
      type_str = "Register: " .. buf(6,length - 6):string() .. " MsgId: " .. buf(4,2):uint()
    elseif msg_type == 11 and length == 7 then                      -- parse RegAck answer
      subtree:add(mqttsn_topicid,   buf(2,2))
      subtree:add(mqttsn_msgid,     buf(4,2))
      subtree:add(mqttsn_return,    buf(6,1))
      if buf(6,1):uint() == 0 then 
        type_str = "RegAck, MsgId: " .. buf(4,2):uint() .. " TopicId: " .. buf(2,2):uint()
      else 
        type_str = "RegAck Error: " .. buf(6,1):uint() .. " MsgId: " .. buf(4,2):uint()
      end
    elseif msg_type == 12 and length > 7  then                      -- parse publish
      mqtts_expand_flags(subtree, buf, 2)
      subtree:add(mqttsn_topicid,   buf(3,2))
      subtree:add(mqttsn_msgid,     buf(5,2))
      subtree:add(mqttsn_data,      buf(7, length - 7))
      type_str = "Publish, TopicID: " .. buf(3,2):uint() .. " Data: " .. buf(7, length - 7)
    elseif msg_type == 13 and length == 7  then                     -- parse puback
      subtree:add(mqttsn_topicid,   buf(2,2))
      subtree:add(mqttsn_msgid,     buf(4,2))
      subtree:add(mqttsn_return,    buf(6,1))
      if buf(6,1):uint() == 0 then  
        type_str = "PubAck, TopicID: " .. buf(2,2):uint()
      else 
        type_str = "PubAck, Error: " .. buf(6,1):uint() .. " TopicID: " .. buf(2,2):uint()
      end
    elseif msg_type == 18 and length > 5 then                       -- parse subscribe request
      mqtts_expand_flags(subtree, buf, 2)
      subtree:add(mqttsn_msgid,     buf(3,2))
      subtree:add(mqttsn_topicname, buf(5,length - 5))
      type_str = "Subscribe, MsgID: " .. buf(3,2):uint() .. " Topic: " .. buf(5,length - 5):string()
    elseif msg_type == 19 and length == 8 then                      -- parse suback answer
      mqtts_expand_flags(subtree, buf, 2)
      subtree:add(mqttsn_topicid,   buf(3,2))
      subtree:add(mqttsn_msgid,     buf(5,2))
      subtree:add(mqttsn_return,    buf(7,1))
      if buf(7,1):uint() == 0 then
        type_str = "SubAck, TopicID: " .. buf(3,2):uint() .. " MsgId: " .. buf(5,2):uint()
      else
        type_str = "SubAck, Error: " .. buf(7,1):uint() .. " TopicID: " .. buf(3,2):uint() .. " MsgId: " .. buf(5,2):uint()
      end
    elseif msg_type == 22 and length == 2 then type_str = "Ping request"
    elseif msg_type == 23 and length == 2 then type_str = "Ping response"
    elseif msg_type == 24 and length == 2 then type_str = "Disconnect request"
    elseif msg_type == 0xFE then type_str = "Forward Message"
      subtree:add(mqttsn_radius, buf(2,1))
      subtree:add(mqttsn_wnodeid, buf(3, length - 3))
    elseif length > 2 then
      subtree:add(mqttsn_m,  buf(2,length - 2))
    end
    return type_str
end

function mqtts_expand_flags(subtree, buf, pos)
  ftree = subtree:add(mqttsn_flag,  buf(pos,1))
  ftree:add(mqttsn_flag_dup,        buf(pos,1))
  ftree:add(mqttsn_flag_qos,        buf(pos,1))
  ftree:add(mqttsn_flag_retain,     buf(pos,1))
  ftree:add(mqttsn_flag_will,       buf(pos,1))
  ftree:add(mqttsn_flag_clrs,       buf(pos,1))
  ftree:add(mqttsn_flag_tid,        buf(pos,1))
end

-- регистрируем диссектор на UDP порт 1883
local udp_dissector_table = DissectorTable.get("udp.port")
udp_dissector_table:add(1883, p_mqttsn)
