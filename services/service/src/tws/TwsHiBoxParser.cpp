/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <map>
#include <dlfcn.h>
#include <charconv>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstring>

#include "TwsDefines.h"
#include "TwsService.h"
#include "TwsHiBoxParser.h"
#include "nearlink_common_event_helper.h"
#include "ASCService.h"
#include "CdsmService.h"
#include "SleInterfaceManager.h"
#include "DeviceBatteryManager.h"
#include "qosm_audio_dfx.h"

namespace OHOS {
namespace Nearlink {

namespace { /* 当前文件涉及变量定义 */
constexpr uint8_t TWS_HIBOX_RSP_FIX_HEAD = 0xF3;
constexpr uint8_t TWS_HIBOX_ERRCODE_MSG_TYPE = 0x7F;     /* 回复错误码时，TLV消息类型 */

/** echo 3,2 **/
constexpr uint8_t TWS_ROLE_SWITCH_TO_PRIMARY = 0x01;       /* echo 3,2 角色切换到主 */
constexpr uint8_t TWS_ROLE_SWITCH_TO_SECONDARY = 0x02;        /* echo 3,2 角色切换到副 */

/** echo 4,3 **/
constexpr uint8_t TWS_AT_CMD_STR_MAX_LEN = 230;
constexpr uint8_t TWS_AT_CMD_TYPE_REPORT = 0x00;                 /* AT指令类型：外设信息上报 */
constexpr uint8_t TWS_AT_CMD_TYPE_BATTERY_REPORT = 0x01;         /* AT指令类型：电量上报 */

/** echo 4,4 **/
constexpr uint8_t TWS_RSP_AUDIO_HEADSET_EXCEP = 0x01;

/* echo 5,a */
constexpr uint8_t TWS_QUERY_BUSINESS_RSP_MSG_TYPE = 0x02;        /* 响应消息Type */

/* 电池电量相关定义 */
const std::string TWS_AT_CMD_BATTERY_STR = "AT+HUAWEIBATTERY";           /* 电量弹框AT指令 */
const std::string TWS_ATC_CMD_UPDATE_BATTERY = "AT+UPDATEHUAWEIBATTERY"; /* 更新电池电量AT指令 */
const std::string TWS_AT_CMD_CLOSE_BATTERY = "AT+CLOSEHUAWEIBATTERY";    /* 关闭电量弹框AT指令 */

constexpr uint8_t TWS_AT_CMD_BAT_KV_PAIR_NUM = 2;                  /* 电池信息长度：键（1Byte）值（1Byte） */
constexpr uint8_t TWS_AT_CMD_BAT_KV_PAIR_MAX = 13;                 /* AT指令携带的最多的数据对 */
constexpr uint8_t TWS_AT_CMD_BAT_KEY_INDEX = 1;                    /* 电池信息键索引（第1字节） */
constexpr uint8_t TWS_AT_CMD_BAT_VALUE_INDEX = 2;                  /* 电池信息值索引（第2字节） */

/* 设备信息相关定义 */
const std::string TWS_AT_CMD_DEVICE_INFO = "AT+XSHUAWEIF";         /* 上报vendor私有特性：设备信息 */
const std::string TWS_AT_CMD_VENDOR_DEVICE_INFO_PREFIX = "4,0";    /* 私有特性：设备信息特性值为4，后固定跟0，标识长度 */

/* 设备类型信息类型，0-b */
const char TWS_DEVINFO_DATA_TYPE_LEN = '0';                 /* 0: 后面所有数据的长度 */
const char TWS_DEVINFO_DATA_TYPE_DEVICE_NAME = '1';
const char TWS_DEVINFO_DATA_TYPE_DEVICE_DATE = '2';
const char TWS_DEVINFO_DATA_TYPE_VENDOR_ID_SRC = '3';
const char TWS_DEVINFO_DATA_TYPE_VENDOR_ID = '4';
const char TWS_DEVINFO_DATA_TYPE_PRODUCT_ID = '5';
const char TWS_DEVINFO_DATA_TYPE_VERSION_ID = '6';
const char TWS_DEVINFO_DATA_TYPE_NEW_MODEL_ID = '7';
const char TWS_DEVINFO_DATA_TYPE_MODEL_ID = '8';
const char TWS_DEVINFO_DATA_TYPE_DEVICE_CLASS = '9';
const char TWS_DEVINFO_DATA_TYPE_DEVICE_TYPE = 'a';
const char TWS_DEVINFO_DATA_TYPE_ICON_ID = 'b';

const int DEX_STRING_TO_INT = 10;

/* Vendor电量私有服务数据类型 */
enum class TwsVendorBatterEnum : uint8_t {
    TWS_VENDOR_BATTERY_NORMAL_LEVEL = 1,        /* Battery Level, Normal device Battery */
    TWS_VENDOR_BATTERY_LEFT,                    /* 2: Left Ear Battery Level */
    TWS_VENDOR_BATTERY_LEFT_CHARGE,             /* 3: Left Ear Charge State */
    TWS_VENDOR_BATTERY_RIGHT,                   /* 4: Right Ear Charge Level */
    TWS_VENDOR_BATTERY_RIGHT_CHARGE,            /* 5: Right Ear Charge State */
    TWS_VENDOR_BATTERY_BOX,                     /* 6: Box Battery Level */
    TWS_VENDOR_BATTERY_BOX_CHARGE,              /* 7: Box Battery State */
    TWS_VENDOR_BATTERY_EAR_ERRCODE,             /* 8: Ear Error Code */
    TWS_VENDOR_BATTERY_BOX_OPEN,                /* 9: Open box Reminder */
    TWS_VENDOR_BATTERY_LEFT_MODEL,              /* 10:  Left Ear Model Id */
    TWS_VENDOR_BATTERY_RIGHT_MODEL,             /* 11:  Right Ear Model Id */
    TWS_VENDOR_BATTERY_DIALOG,                  /* 12: 双耳入盒弹窗信息 */
    TWS_VENDOR_BATTERY_EAR_STATUS,              /* 13:  Ear status info */
};

enum class TwsHiBoxServiceId : uint8_t {
    SID_SEVICE_MGMT = 0x01, /* 服务能力管理 */
    SID_SCENE_MGMT,         /* 场景管理 */
    SID_HITWS_CTRL,         /* 双发控制消息，星闪待整改 */
    SID_DTS_MGMT,           /* 数据传输功能 */
    SID_PAIR_CONN,          /* 配对连接 */
    SID_ICARRY_MGMT,        /* 随身环 */
    SID_AUTO_PAIR,          /* 免按键配对 */
    SID_AUDIO_MGMT,         /* HDAP上行高清 */
    SID_DEVICE_FOUND,       /* 设备查找管 */
    SID_SERVICE_UHD,        /* UHD相关的参数管理 */
};

enum class TwsDtsCmdId : uint8_t {
    GET_DTS = 0x01,         /* 查询对端数据传输的能力 */
    WEAR_STATUS,            /* 外设主动上报佩戴检测的状态 */
    AT_CMD,                 /* AT指令交互 */
};

enum class TwsPairConnCmdId : uint8_t {
    AUDIO_SDP = 0x01,
    MOBILE_CONN_PARA = 0x02,
    CACEL_PAIR = 0x05,
    EARBUDS_NATURE = 0x08,
    QUERY_CONN = 0x0A,
    SECURITY_ADV = 0x0B,
    VENDOR_ACCOUNT_HASH = 0x0D,
    AUTO_CONN_SWITCH = 0x0E,
    PERIPHERAL_MAC_UPDATE = 0x10,
    NOTIFY_SLE_DISCONNECT_PROFILE = 0x11,
};

enum class TwsAudioManagerCmdId : uint8_t {
    PROFILE_STATE = 0x0B,
    OUTPUT_DATAPATH,
    HADP_CODEC_INFO = 0x0E,
};

enum class TwsPowerState : uint8_t {
    POWER_ON = 0x00,
    POWER_OFF = 0x01,
};

enum class TwsAudioMgmtCmdId : uint8_t {
    HDAP_CODEC_REPORT = 1,  /* 上报HDAP支持能力 */
    HDAP_SET_CODEC,         /* 手机设置HDAP Codec能力 */
    START_HDAP,             /* 手机启动HDAP业务 */
    STOP_HDAP,              /* 手机暂停HDAP业务 */
    HDAP_KARAOKE_PARAM,     /* 手机发送HDAP K歌参数 */
    HDAP_VERSION,           /* HDAP版本协商 */
    AUDIO_RATE_PARAM,       /* 外设上报支持的codec码率 */
    AUDIO_PROFILE_STATE,    /* 耳机通知手机当前profile可用状态 */
    AUDIO_OUTPUT_DATAPATH,  /* 手机通知耳机出声通道选择操作以及业务类型 */
    AUDIO_CAP,              /* 手机通知外设黑白名单的能力协商 */
};

enum class TwsDtsMgmtCmdId :uint8_t {
    AUDIO_EXCEP = 0x04,
};

}

/* 单例实例化 */
TwsHiBoxParser &TwsHiBoxParser::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static TwsHiBoxParser hiBoxParserInstance;
    return hiBoxParserInstance;
}

TwsHiBoxParser::TwsHiBoxParser()
{
    /* 初始化默认数通能力 */
    dtsCap_ = { static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_VID),
        static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_WEAR_DETECT),
        static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_FOTA),
        static_cast<uint8_t>(TwsDtsCap::DTS_ABILITY_CHR) };

    /* echo指令处理回调列表 */
    // echo 1,2 能力协商
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY), &ParseAbilityBitMap);
    // echo 3,2 主副切换，双耳都会发，以最后一次的数据为准。
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), &ParseIsoHandover);
    // echo 4,1 数据传输能力，只有主耳会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), &ParseDtsCap);
    // echo 4,2 佩戴检测，只有主耳会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), &ParseWearStatus);
    // echo 4,4
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_HEADSET_EXCEPTION),
        &ParseAudioHeadsetExcep);
    // echo 5,8 左右耳属性，双耳都会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE), &ParseEarbudsNature);
    // echo 5,11 静默数传，断开应用profile，双耳都会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE),
        &ParseNotifySleDisconnectProfile);
    // echo 5,a
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), &ParseQueryBusiness);
    // echo 5,e
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), &ParseAutoConnectSwitch);
    // echo 8,A 音画同步，只有主耳会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_CAP_QUERY), &ParseAudioCap);

    // echo 8,B 双连接，只有主耳会发
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), &ParseProfileState);
    // echo 8,E 双耳录音
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_DUAL_REC_PARAM),
        &ParseDualRecCodecInfo);

    // echo 8,C 双连接强选，耳机回复
    recvMsgProc_.EnsureInsert(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_OUT_DATAPATH), &ParseOutDatapath);

    // echo 4,3 AT指令，只有主耳会发
    hiboxAtCmdProc_.EnsureInsert(TWS_AT_CMD_BATTERY_STR, &ParseAtCmdBatteryDialog);     /* 电量弹框 */
    hiboxAtCmdProc_.EnsureInsert(TWS_ATC_CMD_UPDATE_BATTERY, &ParseAtCmdUpdateBattery); /* 电量更新 */
    hiboxAtCmdProc_.EnsureInsert(TWS_AT_CMD_DEVICE_INFO, &ParseAtCmdUpdateDeviceInfo);  /* 设备信息 */
    hiboxAtCmdProc_.EnsureInsert(TWS_AT_CMD_CLOSE_BATTERY, &ParseAtCmdCloseBattery);    /* 关闭电量弹窗 */
}

/*******************************************************
 *                  发送消息（请求/响应）接口            *
 *******************************************************/

/* echo 4,4 发送手机下电信息 */
bool TwsHiBoxParser::SendRspPowerMessage(const RawAddress &peerAddr, uint8_t isPowerOff)
{
    uint16_t crcValue = 0;  /* 消息校验码 */
    uint8_t dataLen = sizeof(TwsMsgStreamHead) + sizeof(TwsTlvMsg) + sizeof(isPowerOff) + sizeof(crcValue);

    TwsMessage event(TWS_SERVICE_SSAP_SEND_DATA_EVENT);
    event.dev_ = peerAddr.GetAddress();
    event.dataStream_ = std::make_unique<uint8_t[]>(dataLen);
    event.streamLen_ = dataLen;

    TwsMsgStreamHead* rspMsg = reinterpret_cast<TwsMsgStreamHead*>(event.dataStream_.get());
    rspMsg->fixHead = TWS_HIBOX_RSP_FIX_HEAD;
    rspMsg->serviceId = static_cast<uint8_t>(TwsHiBoxServiceId::SID_DTS_MGMT);
    rspMsg->cmdId = static_cast<uint8_t>(TwsDtsMgmtCmdId::AUDIO_EXCEP);

    TwsTlvMsg *payload = reinterpret_cast<TwsTlvMsg*>(event.dataStream_.get() + sizeof(TwsMsgStreamHead));
    payload->msgType = TWS_RSP_AUDIO_HEADSET_EXCEP; /* T */
    payload->dataLen = sizeof(isPowerOff);                   /* L */
    payload->data[0] = isPowerOff;                           /* V */

    /* 计算消息crc值并拼接 */
    crcValue = CalcMessageCrc(reinterpret_cast<uint8_t *>(event.dataStream_.get()), dataLen - sizeof(crcValue));
    *reinterpret_cast<uint16_t *>(event.dataStream_.get() + dataLen - sizeof(crcValue)) = crcValue;

    /* 交由service调用动态库往外发送 */
    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN_RET(serviceInstance, false,
        "[Tws HiBox]:send data transfer request message fail,Tws HiBox instance invalid");
    serviceInstance->PostEvent(event);

    HILOGI("[Tws MsgProc]:send isPowerOff to peer addr:%{public}s,isPowerOff:%{public}u",
        GetEncryptAddr(event.dev_).c_str(), isPowerOff);
    return true;
}

/* echo 4,1 发送数传能力请求/响应消息 */
bool TwsHiBoxParser::SendDtsCapMessage(const RawAddress &peerAddr, bool isRespMsg)
{
    if (dtsCap_.size() == 0) {
        HILOGE("[Tws HiBox]:send data transfer request message fail,data empty.");
        return false;
    }

    int dtsCapMsgType = TWS_SERVICE_SEND_REQ_EVENT;
    uint8_t msgDirect = TWS_MSG_DIRECT_REQ;
    if (isRespMsg) {
        dtsCapMsgType = TWS_SERVICE_SEND_RSP_EVENT;
        msgDirect = TWS_MSG_DIRECT_RSP;
    }
    TwsMessage event(dtsCapMsgType);
    event.msgDirect_ = msgDirect;
    event.dev_ = peerAddr.GetAddress();
    event.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP);
    event.serviceDataLen_ = dtsCap_.size();
    event.serviceData_ = std::make_unique<uint8_t[]>(event.serviceDataLen_);
    (void)memcpy_s(event.serviceData_.get(), dtsCap_.size(), dtsCap_.data(), dtsCap_.size());

    /* 交给service调用动态库往外发送 */
    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN_RET(serviceInstance, false,
        "[Tws HiBox]:send data transfer request message fail,Tws HiBox instance invalid");
    serviceInstance->PostEvent(event);
    return true;
}

/* 统一消息发送接口 */
bool TwsHiBoxParser::SendMessageEntry(const RawAddress &peerAddr, uint8_t msgType, uint8_t *data,
    uint16_t dataLen, bool isRespMsg)
{
    NL_CHECK_RETURN_RET(dataLen != 0, false, "[Tws HiBox]:send query business message fail,data empty.");

    int dtsCapMsgType = TWS_SERVICE_SEND_REQ_EVENT;
    uint8_t msgDirect = TWS_MSG_DIRECT_REQ;
    if (isRespMsg) {
        dtsCapMsgType = TWS_SERVICE_SEND_RSP_EVENT;
        msgDirect = TWS_MSG_DIRECT_RSP;
    }
    TwsMessage event(dtsCapMsgType);
    event.msgDirect_ = msgDirect;
    event.dev_ = peerAddr.GetAddress();
    event.msgType_ = msgType;
    event.serviceDataLen_ = dataLen;
    event.serviceData_ = std::make_unique<uint8_t[]>(event.serviceDataLen_);
    (void)memcpy_s(event.serviceData_.get(), dataLen, data, dataLen);

    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN_RET(serviceInstance, false,
        "[Tws HiBox]:send message:%{public}u fail,Tws HiBox instance invalid.", msgType);
    serviceInstance->PostEvent(event);
    HILOGD("[Tws HiBox]:send message:0x%{public}02x(hibox) to peer addr:%{public}s",
        msgType, GET_ENCRYPT_ADDR(peerAddr));

    return true;
}

/* 对外封装接口 */
bool TwsHiBoxParser::SendMessage(const RawAddress &peerAddr, uint8_t msgType,
    uint8_t *data, uint16_t dataLen, bool isRespMsg)
{
    /* 找到主耳，只给主耳发消息 */
    RawAddress primary(peerAddr);
    TwsService *twsService = TwsService::GetService();
    if (twsService != nullptr) {
        primary = twsService->GetPrimaryAddr(peerAddr);
    }

    bool ret = false;
    switch (msgType) {
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP):
            ret = SendDtsCapMessage(primary, isRespMsg);
            break;
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_OUT_DATAPATH):
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS): /* 耳机抢占查询 */
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY): /* 发送能力协商 */
        case static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_VENDOR_ACCOUNT_HASH): /* 发送vendor账号hash */
            ret = SendMessageEntry(primary, msgType, data, dataLen, isRespMsg);
            break;
        default:
            HILOGE("[Tws HiBox]:send message failed,message type not support:%{public}u", msgType);
            break;
    }

    return ret;
}

/*******************************************************
 *                  接收消息（请求/响应）接口            *
 *******************************************************/

/* 处理接收到的echo 1,2消息，更新设备能力位图 */
void TwsHiBoxParser::ParseAbilityBitMap(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(ManufacturerAbilityInfo),
        "[Tws HiBox]:parse audio capability (1,2) failed, role change data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(ManufacturerAbilityInfo));

    /* 解引用指针转结构体赋值 */
    ManufacturerAbilityInfo data = *(reinterpret_cast<ManufacturerAbilityInfo*>(event.serviceData_.get()));

    RawAddress peerAddr(event.dev_);
    TwsClientData twsData(peerAddr);
    for (uint8_t i = 0; i < DEVICE_MANUFACTURER_ABILITY_LEN; ++i) {
        twsData.manufacturerAbility_[i] = data.manufacturerAbility[i];
    }

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:ParseEarbudsNature, Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_MANUFACTURER_ABILITY), twsData);
}

/* 处理接收到的echo 3,2消息，更新设备角色 */
void TwsHiBoxParser::ParseIsoHandover(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(TwsIsoHandover),
        "[Tws HiBox]:parse iso handover failed,role change data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(TwsIsoHandover));

    TwsIsoHandover data = *(reinterpret_cast<TwsIsoHandover*>(event.serviceData_.get()));   /* 解引用指针转结构体赋值 */
    NL_CHECK_RETURN(data.type == TWS_ROLE_SWITCH_TO_PRIMARY || data.type == TWS_ROLE_SWITCH_TO_SECONDARY,
        "[Tws HiBox]:iso handover,recv role change,data type invalid:%{public}u.", data.type);

    RawAddress peerAddr(event.dev_);
    TwsClientData isoHandoverData(peerAddr);
    if (data.type == TWS_ROLE_SWITCH_TO_PRIMARY) {
        isoHandoverData.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    }
    if (data.type == TWS_ROLE_SWITCH_TO_SECONDARY) {
        isoHandoverData.roleType_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY);
    }

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send encode request message fail,Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE), isoHandoverData);

    /* 原文回复 */
    TwsMessage sendRsp(TWS_SERVICE_SEND_RSP_EVENT);
    sendRsp.msgDirect_ = TWS_MSG_DIRECT_RSP;
    sendRsp.dev_ = peerAddr.GetAddress();
    sendRsp.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE);
    sendRsp.serviceDataLen_ = sizeof(TwsIsoHandover);
    sendRsp.serviceData_ = std::make_unique<uint8_t[]>(sendRsp.serviceDataLen_);
    (void)memcpy_s(sendRsp.serviceData_.get(), sendRsp.serviceDataLen_,
        reinterpret_cast<TwsIsoHandover*>(&data), sizeof(TwsIsoHandover));

    /* 交给service调用动态库往外发送 */
    twsService->PostEvent(sendRsp);
}

/* 处理接收到的echo 4,1消息，数通能力交互 */
void TwsHiBoxParser::ParseDtsCap(const TwsMessage &event)
{
    RawAddress peerAddr(event.dev_);
    TwsClientData dtsCap(peerAddr);
    dtsCap.devDtsCap_.insert(dtsCap.devDtsCap_.begin(),
        event.serviceData_.get(), event.serviceData_.get() + event.serviceDataLen_);

    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN(serviceInstance, "[Tws MsgProc]:send encode request message fail,Tws HiBox instance invalid!");

    TwsHiBoxParser::GetInstance().SendMessage(peerAddr,
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), nullptr, 0, true);
    serviceInstance->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_DATA_TRANSPORT_CAP), dtsCap);
}

/* 处理接收到的echo 4,2消息，穿戴检测 */
void TwsHiBoxParser::ParseWearStatus(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(TwsWearStateData),
        "[Tws HiBox]:parse wear status failed,data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(TwsWearStateData));

    RawAddress peerAddr(event.dev_);
    TwsWearStateData wearData = *(reinterpret_cast<TwsWearStateData*>(event.serviceData_.get()));

    TwsClientData wearState(peerAddr);
    wearState.wearData_ = wearData;
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send encode request message fail,Tws HiBox instance invalid!");

    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_WEAR_STATUS), wearState);

    /* 回复错误码 */
    SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_DTS_MGMT),
        static_cast<uint8_t>(TwsDtsCmdId::WEAR_STATUS), peerAddr,
        static_cast<uint8_t>(TwsErrCode::SUCCESS));
}

void TwsHiBoxParser::ParseEarbudsNature(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(EarbudsNature),
        "[Tws HiBox]:parse audio capability (5,8) failed, role change data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(EarbudsNature));

    EarbudsNature data = *(reinterpret_cast<EarbudsNature*>(event.serviceData_.get())); /* 解引用指针转结构体赋值 */
    HILOGD("[Tws HiBox]: nature=%{public}d", data.nature);

    RawAddress peerAddr(event.dev_);
    TwsClientData twsData(peerAddr);
    twsData.nature_ = data.nature;

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws HiBox]:ParseEarbudsNature, Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE), twsData);

    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "[Tws HiBox]:ParseEarbudsNature, asc instance invalid!");
    ascService->UpdateDeviceNature(peerAddr);

    /* 回复错误码 */
    SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_PAIR_CONN),
        static_cast<uint8_t>(TwsPairConnCmdId::EARBUDS_NATURE), peerAddr,
        static_cast<uint8_t>(TwsErrCode::SUCCESS));
}

/* 解析echo 5,a消息，抢占查询响应消息 */
void TwsHiBoxParser::ParseQueryBusiness(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.msgDirect_ == TWS_MSG_DIRECT_RSP && event.serviceDataLen_ == sizeof(QueryPairConnCfmResult),
        "[Tws MsgProc]:query business msg dir:%{public}u,len:%{public}u!", event.msgDirect_, event.serviceDataLen_);

    /* 只解析响应消息 */
    RawAddress peerAddr(event.dev_);
    QueryPairConnCfmResult queryRes = *(reinterpret_cast<QueryPairConnCfmResult *>(event.serviceData_.get()));

    NL_CHECK_RETURN(queryRes.msgType == TWS_QUERY_BUSINESS_RSP_MSG_TYPE,
        "[Tws MsgProc]:query business type,refuse code:%{public}u", queryRes.typeValue);

    TwsClientData bussiness(peerAddr);
    bussiness.captureRes_ = queryRes.typeValue;
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send bussiness update msg fail,Tws HiBox instance invalid!");

    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_QUERY_BUSINESS), bussiness);
}

/* 解析echo 5,e消息，自动连接开关值 */
void TwsHiBoxParser::ParseAutoConnectSwitch(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(MultConnAutoConnSwitch),
        "[Tws HiBox]:parse audio capability (5,e) failed,data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(MultConnAutoConnSwitch));

    MultConnAutoConnSwitch autoSwitch = *(reinterpret_cast<MultConnAutoConnSwitch *>(event.serviceData_.get()));
    TwsClientData autoConnSw(RawAddress(event.dev_));
    autoConnSw.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::ON);
    if (autoSwitch.autoConnSwitch == static_cast<uint8_t>(TwsAutoConnSwitch::OFF)) {
        autoConnSw.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);
    }

    TwsService *twsService = TwsService::GetService();
    RawAddress peerAddr(event.dev_);
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send bussiness update msg fail,Tws HiBox instance invalid!");

    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), autoConnSw);

    /* 回复错误码 */
    SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_PAIR_CONN),
        static_cast<uint8_t>(TwsPairConnCmdId::AUTO_CONN_SWITCH), peerAddr,
        static_cast<uint8_t>(TwsErrCode::SUCCESS));
}

void TwsHiBoxParser::ParseNotifySleDisconnectProfile(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(NotifyDisconnectProfile),
        "[Tws HiBox]:parse echo 5,11 failed, data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(NotifyDisconnectProfile));

    NotifyDisconnectProfile data = *(reinterpret_cast<NotifyDisconnectProfile*>(event.serviceData_.get()));
    HILOGI("[Tws HiBox]: notifyValue=%{public}d", data.notifyValue);

    SleInterfaceAdapter *adapter = SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE);
    RawAddress peerAddr(event.dev_);
    if (data.notifyValue == static_cast<uint8_t>(TwsProfileAction::DISCONNECT) &&
        adapter != nullptr) {
        adapter->DisconnectAllProfileForSilentPort(peerAddr); /* 通知适配层断开服务 */
    }

    /* 回复错误码 */
    SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_PAIR_CONN),
        static_cast<uint8_t>(TwsPairConnCmdId::NOTIFY_SLE_DISCONNECT_PROFILE), peerAddr,
        static_cast<uint8_t>(TwsErrCode::SUCCESS));

    TwsClientData notifyVal(RawAddress(event.dev_));
    notifyVal.notifyValue_ = data;
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:update notify value fail,Tws HiBox instance invalid!");

    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE),
        notifyVal);
}

void TwsHiBoxParser::ParseAudioCap(const TwsMessage &event)
{
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(TwsAudioCapsData),
        "[Tws MsgProc]:parse audio capability (8,a) fail,role change data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(TwsAudioCapsData));

    TwsAudioCapsData data = *(reinterpret_cast<TwsAudioCapsData*>(event.serviceData_.get())); /* 解引用指针转结构体赋值 */

    RawAddress peerAddr(event.dev_);
    TwsClientData twsData(peerAddr);
    NL_CHECK_RETURN(memcpy_s(&twsData.sleConfig_, sizeof(twsData.sleConfig_),
        &data.sleConfig, sizeof(data.sleConfig)) == EOK, "twsData.sleConfig_ memcpy_s failed");
    HILOGI("[Tws HiBox]:parse offset: %{public}d.", twsData.sleConfig_.offset);

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send encode request message fail,Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_CAP_QUERY), twsData);

    /* 原文回复 */
    TwsMessage sendRsp(TWS_SERVICE_SEND_RSP_EVENT);
    sendRsp.msgDirect_ = TWS_MSG_DIRECT_RSP;
    sendRsp.dev_ = peerAddr.GetAddress();
    sendRsp.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_CAP_QUERY);
    sendRsp.serviceDataLen_ = sizeof(TwsAudioCapsData);
    sendRsp.serviceData_ = std::make_unique<uint8_t[]>(sendRsp.serviceDataLen_);
    (void)memcpy_s(sendRsp.serviceData_.get(), sendRsp.serviceDataLen_,
        reinterpret_cast<TwsAudioCapsData*>(&data), sizeof(TwsAudioCapsData));

    /* 交给service调用动态库往外发送 */
    twsService->PostEvent(sendRsp);
}

/* 上报profile状态给框架，转换后透传 */
void TwsHiBoxParser::ParseProfileState(const TwsMessage &event)
{
    /* 解析HiBox数据 */
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(HiBoxNearlinkProfile),
        "[Tws MsgProc]:parse profile state(echo 8,b),len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(HiBoxNearlinkProfile));

    RawAddress peerAddr(event.dev_);
    HiBoxNearlinkProfile data = *(reinterpret_cast<HiBoxNearlinkProfile*>(event.serviceData_.get()));

    /* service数据更新和状态变化上报 */
    TwsClientData twsData(peerAddr);
    twsData.audioState_ = data;
    twsData.audioState_.isUserSelected = false;
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:ParseProfileState, Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), twsData);

    /* 回复错误码 */
    SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_AUDIO_MGMT),
        static_cast<uint8_t>(TwsAudioManagerCmdId::PROFILE_STATE), peerAddr,
        static_cast<uint8_t>(TwsErrCode::SUCCESS));
}

/* 上报profile状态给框架，转换后透传 */
void TwsHiBoxParser::ParseOutDatapath(const TwsMessage &event)
{
    /* 解析HiBox数据 */
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(HiboxRspCommonResult),
        "[Tws MsgProc]:parse out datapath(echo 8,c),len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(HiboxRspCommonResult));

    RawAddress peerAddr(event.dev_);
    HiboxRspCommonResult data = *(reinterpret_cast<HiboxRspCommonResult*>(event.serviceData_.get()));

    NL_CHECK_RETURN(data.rspResult == HIBOX_SUCCESS,
        "[Tws MsgProc]:out datapath(echo 8,c) rsp fail! rspResult=%{public}u", data.rspResult);

    /* service数据更新和状态变化上报 */
    TwsClientData twsData(peerAddr);
    for (uint16_t index = 0; index < static_cast<uint8_t>(TwsStreamType::STREAM_TYPE_MAX); index++) {
        twsData.audioState_.audioType[index] = static_cast<uint8_t>(TwsMediaState::ENABLE);
    }
    memset_s(twsData.audioState_.targetDevName, sizeof(twsData.audioState_.targetDevName),
        0, sizeof(twsData.audioState_.targetDevName));
    twsData.audioState_.isUserSelected = true;
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:ParseProfileState, Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE), twsData);
}

/* 收到高清录音codec信息回复对端 */
void TwsHiBoxParser::DualRecCodecInfoRsp(const RawAddress &peerAddr, DeviceDualRecParmInfo info)
{
    /* 原文回复 */
    TwsMessage sendRsp(TWS_SERVICE_SEND_RSP_EVENT);
    sendRsp.msgDirect_ = TWS_MSG_DIRECT_RSP;
    sendRsp.dev_ = peerAddr.GetAddress();
    sendRsp.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_DUAL_REC_PARAM);
    sendRsp.serviceDataLen_ = sizeof(DeviceDualRecParmInfo);
    sendRsp.serviceData_ = std::make_unique<uint8_t[]>(sendRsp.serviceDataLen_);
    if (memcpy_s(sendRsp.serviceData_.get(), sendRsp.serviceDataLen_, &info, sizeof(DeviceDualRecParmInfo))) {
        return;
    }
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws HiBox]:ParseProfileState, Tws HiBox instance invalid!");
    /* 交给service调用动态库往外发送 */
    twsService->PostEvent(sendRsp);
    HILOGI("[Tws HiBox]: HadpCodecInfoRsp rsp");
}

DeviceDualRecParmInfo TwsHiBoxParser::SetPeerDualRecCodecParamConfig(const HiBoxNearlinkDualRecCaps& data)
{
    DeviceDualRecParmInfo rspInfo;
    (void)memset_s(&rspInfo, sizeof(DeviceDualRecParmInfo), 0x00, sizeof(DeviceDualRecParmInfo));
    // 当前本端仅支持L2HC_VOICE 免去协商
    AscCodecIdKey localCodec = ASC_L2HC_VOICE_CODEC;
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN_RET(ascService, rspInfo, "[Tws HiBox]:asc instance invalid!");
    for(int i = 0; i < data.codecNum; i++) {
        DeviceDualRecCapInfo reqInfo = data.param[i];
        AscCodecIdKey peerCodec = {
            .codecId = reqInfo.codecId,
            .companyId = reqInfo.companyId,
            .vendorId = reqInfo.vendorId,
            .version = reqInfo.version,
        };
        HILOGI("[Tws HiBox]: codecId %{public}d, companyId %{public}d vendorId %{public}d",
            peerCodec.codecId, peerCodec.companyId, peerCodec.vendorId);
        if (ascService->IsEqualCodec(localCodec, peerCodec)) {
            rspInfo.codecId = peerCodec.codecId;
            rspInfo.companyId = peerCodec.companyId;
            rspInfo.vendorId = peerCodec.vendorId;
            ascService->ProcessCodecFCVersion(peerCodec);
            rspInfo.version = peerCodec.version;
            rspInfo.sampleRate = ascService->SelectCodecSampleRatePolicyInDualRec(peerCodec, reqInfo.sampleRate);
            rspInfo.depth = ascService->SelectCodecBitDepthPolicyInDualRec(peerCodec, reqInfo.depth);
            rspInfo.channel = ascService->SelectCodecChannelPolicyInDualRec(peerCodec, reqInfo.channel);
            rspInfo.frame = ASC_L2HC_FRAME_10MS;
            // 双耳录音上行：96kbps codec_1 L2HC_VOICE, 下行：96/48kbps codec_2 L2HC
            (void)memcpy_s(&rspInfo.upBitrate, sizeof(rspInfo.upBitrate),
                &L2HC_VOICE_BPS_S_96, DUAL_REC_CODEC_UP_BITRATE_BIT_LEN);
            HILOGI("[Tws HiBox]: peer have match codec");
            break;
        }
    }
    return rspInfo;
}

/* 上报高清录音codec信息 */
void TwsHiBoxParser::ParseDualRecCodecInfo(const TwsMessage &event)
{
    HILOGD("[Tws HiBox] enter");
    RawAddress peerAddr(event.dev_);
    /* 解析HiBox数据 */
    if (event.serviceDataLen_ != sizeof(HiBoxNearlinkDualRecCaps)) {
        HILOGE("[Tws HiBox]:parse echo 8,E len invalid:%{public}u/%{public}u.",
            event.serviceDataLen_, sizeof(HiBoxNearlinkDualRecCaps));
        return;
    }
    HiBoxNearlinkDualRecCaps data = *(reinterpret_cast<HiBoxNearlinkDualRecCaps*>(event.serviceData_.get()));
    // 校验编解码数量有效性
    if (data.codecNum < 1 || data.codecNum > DEVICE_DUAL_REC_CODEC_MAX_NUM) {
        HILOGE("[Tws HiBox]: parse echo 8,E codecNum:%{public}d ", data.codecNum);
        return;
    }
    DeviceDualRecParmInfo info = SetPeerDualRecCodecParamConfig(data);
    HILOGI("[Tws HiBox]: codecNum:%{public}d, codecId:%{public}d, companyId:%{public}d, vendorId:%{public}d,"
        "version:%{public}d, sampleRate:%{public}d, depth:%{public}d, channel:%{public}d,"
        "frame:%{public}d,", data.codecNum, info.codecId, info.companyId, info.vendorId, info.version,
        info.sampleRate, info.depth, info.channel, info.frame);
    DualRecCodecInfoRsp(peerAddr, info);
}

/* 分割字符串 */
std::vector<std::string> TwsHiBoxParser::SplitValueByDelim(const std::string &commandValue, const std::string &delim)
{
    std::vector<std::string> result {};
    std::string inputStr = commandValue;

    size_t pos = inputStr.find(delim);
    while (pos != std::string::npos) {
        std::string subStr = inputStr.substr(0, pos);
        result.push_back(subStr);
        inputStr.erase(0, pos + 1);
        pos = inputStr.find(delim);
    }
    result.push_back(inputStr);
    return result;
}

template <typename T>
bool TwsHiBoxParser::ConvertStrToNumber(const std::string &src, T &ret, int base)
{
    std::string str;
    for (char c : src) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            str.push_back(c);
        }
    }
    std::from_chars_result res = std::from_chars(str.data(), str.data() + str.size(), ret, base);
    NL_CHECK_RETURN_RET(res.ec == std::errc{} && res.ptr == str.data() + str.size(), false,
        "FromString failed, error string is %{public}s", str.c_str());
    return true;
}

/* CRC16: x^16 + x^15 + x^2 + 1 */
const uint16_t CRC16_POLY = 0x8005;
const uint16_t CRC_RETRIEVE = 0x8000;
uint16_t TwsHiBoxParser::CalcMessageCrc(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0x0000;

    for (uint16_t i = 0; i < len; i++) {
        crc = crc ^ (*buf++ << 8); /* Shift left 8 bit */

        for (uint16_t j = 0; j < 8; j++) { // 8: crc check bit
            if (crc & CRC_RETRIEVE) { /* Retrieve the most significant bit */
                crc = (crc << 1) ^ CRC16_POLY;
            } else {
                crc <<= 1;
            }
        }

        crc &= 0xFFFF; /* 0xffff:高位清零 */
    }

    return (crc);
}

bool TwsHiBoxParser::AtCmdStartWith(const std::string &str, const std::string &prefix)
{
    NL_CHECK_RETURN_RET(str.length() >= prefix.length(), false, "[Tws HiBox]: AtCmdStartWith failed.");
    return str.substr(0, prefix.length()).compare(prefix) == 0;
}

bool TwsHiBoxParser::SendRemoteInfo(const RawAddress &peerAddr, const std::string &value)
{
    TwsMessage event(TWS_SERVICE_SEND_REMOTE_INFO_EVENT);
    event.dev_ = peerAddr.GetAddress();
    char *charStr = new (std::nothrow) char[value.length() + 1];
    NL_CHECK_RETURN_RET(charStr, false, "[Tws MsgProc]:send remote info fail,memory alloc failed");
    errno_t ret = strncpy_s(charStr, value.length() + 1, value.c_str(), value.length());
    if (ret != EOK) {
        HILOGE("[Tws MsgProc]:send remote info fail,strncpy_s ret:%{public}d", ret);
        delete[] charStr;
        return false;
    }
    event.arg2M = charStr;
    TwsService *twsService = TwsService::GetService();
    if (twsService == nullptr) {
        HILOGE("[Tws MsgProc]:send remote info fail,tws service invalid!");
        delete[] charStr;
        return false;
    }
    twsService->PostEvent(event);
    return true;
}

/* 电量弹窗 */
bool TwsHiBoxParser::ParseAtCmdBatteryDialog(const RawAddress &peerAddr, const std::string &atCmdParam)
{
    std::string value = "+HUAWEIBATTERY=" + atCmdParam;
    BatteryInfo batteryInfo {};
    NL_CHECK_RETURN_RET(ParserAtCmdVendorBattery(peerAddr, atCmdParam, batteryInfo), false,
        "ParserAtCmdVendorBattery failed");
    DeviceBatteryManager::GetInstance().PublishBatteryLevel(peerAddr, batteryInfo);
    NL_CHECK_RETURN_RET(SendRemoteInfo(peerAddr, value), false,
        "[Tws MsgProc]:battery dialog send fail,dev:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
    return true;
}

/* 关闭电量弹窗 */
bool TwsHiBoxParser::ParseAtCmdCloseBattery(const RawAddress &peerAddr, const std::string &atCmdParam)
{
    std::string value = "+CLOSEHUAWEIBATTERY=" + atCmdParam;
    NL_CHECK_RETURN_RET(SendRemoteInfo(peerAddr, value), false,
        "[Tws MsgProc]:close battery send fail,dev:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
    return true;
}

/* 更新电量信息 */
bool TwsHiBoxParser::ParseAtCmdUpdateBattery(const RawAddress &peerAddr, const std::string &atCmdParam)
{
    std::string value = "+UPDATEHUAWEIBATTERY=" + atCmdParam;
    BatteryInfo batteryInfo {};
    NL_CHECK_RETURN_RET(ParserAtCmdVendorBattery(peerAddr, atCmdParam, batteryInfo), false,
        "ParserAtCmdVendorBattery failed");
    DeviceBatteryManager::GetInstance().PublishBatteryLevel(peerAddr, batteryInfo);
    NL_CHECK_RETURN_RET(SendRemoteInfo(peerAddr, value), false,
        "[Tws MsgProc]:update battery send fail,dev:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
    return true;
}

/* 将数据写入结构体 */
void TwsHiBoxParser::UpdateVendorBatteryInfo(BatteryInfo &dataBlock, uint8_t type, uint32_t value)
{
    /* 数据类型和变量值偏移 */
    /* key：数据类型   value: 变量偏移 */
    const std::map<const TwsVendorBatterEnum, uint8_t> vendorBatter = {
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_NORMAL_LEVEL, offsetof(BatteryInfo, devBattery_)   },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_LEFT,         offsetof(BatteryInfo, leftBattery_)  },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_LEFT_CHARGE,  offsetof(BatteryInfo, leftCharge_)  },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_RIGHT,        offsetof(BatteryInfo, rightBattery_) },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_RIGHT_CHARGE, offsetof(BatteryInfo, rightCharge_) },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_BOX,          offsetof(BatteryInfo, boxBattery_)   },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_BOX_CHARGE,   offsetof(BatteryInfo, boxCharge_)   },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_EAR_ERRCODE,  offsetof(BatteryInfo, earErrCode_)  },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_BOX_OPEN,     offsetof(BatteryInfo, boxOpen_)     },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_LEFT_MODEL,   offsetof(BatteryInfo, leftModel_)   },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_RIGHT_MODEL,  offsetof(BatteryInfo, rightModel_)  },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_DIALOG,       offsetof(BatteryInfo, dialogState_) },
        { TwsVendorBatterEnum::TWS_VENDOR_BATTERY_EAR_STATUS,   offsetof(BatteryInfo, earStatus_)   },
    };

    auto item = vendorBatter.find(static_cast<TwsVendorBatterEnum>(type));
    NL_CHECK_RETURN(item != vendorBatter.end(), "[Tws MsgProc]:Vendor battery data type invalid:%{public}u", type);

    uint8_t offset = item->second;
    *reinterpret_cast<uint8_t*>(reinterpret_cast<char*>(&dataBlock) + offset) = value;
}

bool TwsHiBoxParser::ParserAtCmdVendorBattery(const RawAddress &peerAddr, const std::string &atCmdParam,
    BatteryInfo &batteryInfo)
{
    std::vector<std::string> values = SplitValueByDelim(atCmdParam, ",");
    NL_CHECK_RETURN_RET(values.size() != 0, false,
        "[Tws HiBox]:parse battery info at cmd failed,atcmd value parse result empty.");

    /* AT值长度检查，键值数,键1,值1 */
    uint8_t kvPairNum = 0;
    NL_CHECK_RETURN_RET(ConvertStrToNumber(values[0], kvPairNum, DEX_STRING_TO_INT), false,
        "[Tws HiBox]:parse vendor battery info at cmd failed,kvpair num invalid:%{public}s.", values[0].c_str());
    // 校验长度：kvPairNum * (key + value) + kvPairNum(1) 并检查上限
    NL_CHECK_RETURN_RET(kvPairNum * TWS_AT_CMD_BAT_KV_PAIR_NUM + 1 == values.size() &&
        kvPairNum <= TWS_AT_CMD_BAT_KV_PAIR_MAX, false,
        "[Tws HiBox]:parse vendor battery info at cmd failed,kvpair len invalid:%{public}u/%{public}u.",
        kvPairNum, values.size());

    uint8_t indicatorType = 0;
    uint32_t indicatorValue = 0xFF;
    for (uint8_t i = 0; i < kvPairNum; i++) {
        std::string keyType = values[i * TWS_AT_CMD_BAT_KV_PAIR_NUM + TWS_AT_CMD_BAT_KEY_INDEX];
        std::string value = values[i * TWS_AT_CMD_BAT_KV_PAIR_NUM + TWS_AT_CMD_BAT_VALUE_INDEX];
        NL_CHECK_RETURN_RET(ConvertStrToNumber(keyType, indicatorType, DEX_STRING_TO_INT), false,
            "[Tws HiBox]:parse vendor battery info at cmd failed,key type invalid:%{public}s.", keyType.c_str());
        NL_CHECK_RETURN_RET(ConvertStrToNumber(value, indicatorValue, DEX_STRING_TO_INT), false,
            "[Tws HiBox]:parse vendor battery info at cmd failed,value invalid:%{public}s.", value.c_str());

        /* 把数据保存到结构体中 */
        UpdateVendorBatteryInfo(batteryInfo, indicatorType, indicatorValue);
    }
    return true;
}

/* 通用接口：发送错误码（包含成功）给对端设备 */
bool TwsHiBoxParser::SendErrorCodeResp(const uint8_t serviceId, const uint8_t cmdId,
    const RawAddress &peerAddr, uint8_t errCode)
{
    uint16_t crcValue = 0;  /* 消息校验码 */
    uint8_t dataLen = sizeof(TwsMsgStreamHead) + sizeof(TwsEncodeMsg) + sizeof(errCode) + sizeof(crcValue);

    TwsMessage event(TWS_SERVICE_SSAP_SEND_DATA_EVENT);
    event.dev_ = peerAddr.GetAddress();
    event.dataStream_ = std::make_unique<uint8_t[]>(dataLen);
    event.streamLen_ = dataLen;

    TwsMsgStreamHead* rspMsg = reinterpret_cast<TwsMsgStreamHead*>(event.dataStream_.get());
    rspMsg->fixHead = TWS_HIBOX_RSP_FIX_HEAD;
    rspMsg->serviceId = serviceId;
    rspMsg->cmdId = cmdId;

    TwsEncodeMsg *payload = reinterpret_cast<TwsEncodeMsg*>(event.dataStream_.get() + sizeof(TwsMsgStreamHead));
    payload->msgType = TWS_HIBOX_ERRCODE_MSG_TYPE; /* T */
    payload->dataLen = sizeof(errCode);                   /* L */
    payload->data[0] = errCode;                           /* V 0:错误码仅填充第一个字节 */

    /* 计算消息crc值并拼接 */
    crcValue = CalcMessageCrc(reinterpret_cast<uint8_t *>(event.dataStream_.get()), dataLen - sizeof(crcValue));
    *reinterpret_cast<uint16_t *>(event.dataStream_.get() + dataLen - sizeof(crcValue)) = crcValue;

    /* 交由service调用动态库往外发送 */
    TwsService *serviceInstance = TwsService::GetService();
    NL_CHECK_RETURN_RET(serviceInstance, false,
        "[Tws HiBox]:send data transfer request message fail,Tws HiBox instance invalid.");
    serviceInstance->PostEvent(event);

    HILOGD("[Tws MsgProc]:send errcode to peer addr:%{public}s,errcode:%{public}u",
        GetEncryptAddr(event.dev_).c_str(), errCode);
    return true;
}

/* 根据结构体解析TLV数据 */
void TwsHiBoxParser::ParseDeviceInfo(const uint8_t dataType, const std::string &data, TwsDeviceDatas &devInfo)
{
    switch (dataType) {
        case TWS_DEVINFO_DATA_TYPE_DEVICE_NAME:
            devInfo.deviceName = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_DEVICE_DATE:
            devInfo.timeStamp = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_VENDOR_ID_SRC:
            devInfo.vendorIdSrc = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_VENDOR_ID:
            devInfo.vendorId = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_PRODUCT_ID:
            devInfo.productId = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_VERSION_ID:
            devInfo.version = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_NEW_MODEL_ID:
            devInfo.newModelId = ConvertToUpper(data);
            break;
        case TWS_DEVINFO_DATA_TYPE_MODEL_ID:
            devInfo.modelId = ConvertToUpper(data);
            break;
        case TWS_DEVINFO_DATA_TYPE_DEVICE_CLASS:
            devInfo.devClass = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_DEVICE_TYPE:
            devInfo.devType = data;
            break;
        case TWS_DEVINFO_DATA_TYPE_ICON_ID:
            devInfo.iconId = ConvertToUpper(data);
            break;
        default:
            break;
    }
}

/* AT指令：AT+XSHUAWEIF，跟随设备信息特性参数 */
bool TwsHiBoxParser::ParseAtCmdUpdateDeviceInfo(const RawAddress &peerAddr, const std::string &atCmdParam)
{
    if (!AtCmdStartWith(atCmdParam, TWS_AT_CMD_VENDOR_DEVICE_INFO_PREFIX)) {
        HILOGW("[Tws HiBox]:Device info cmd not support:%{public}s", atCmdParam.c_str());
        return false;
    }

    TwsDeviceDatas devInfo = { };
    uint32_t offset = 0;
    std::string atParamData = atCmdParam.substr(TWS_AT_CMD_VENDOR_DEVICE_INFO_PREFIX.size() - 1);
    uint32_t atParamSize = atParamData.size();
    while (offset < atParamSize) {
        // Type字段：1个字符
        NL_CHECK_RETURN_RET(offset + sizeof(uint8_t) <= atParamSize, false,
            "[Tws HiBox]:tlv data length invalid,check data type failed.");
        uint8_t type = atParamData[offset++];
        // Length字段：2个字符
        NL_CHECK_RETURN_RET(offset + sizeof(uint16_t) <= atParamSize, false,
            "[Tws HiBox]:tlv data length invalid,check data length failed.");

        // 计算length长度
        std::string lengthStr = atParamData.substr(offset, 2);
        uint16_t length = 0;
        NL_CHECK_RETURN_RET(ConvertStrToNumber(lengthStr, length, DEX_STRING_TO_INT), false,
            "[Tws HiBox]:tlv data length invalid,convert length failed.");
        offset += sizeof(uint16_t);

        // Value字段
        NL_CHECK_RETURN_RET(offset + length <= atParamSize, false,
            "[Tws HiBox]:tlv data length invalid,check data value failed.");

        std::string value = atParamData.substr(offset, length);
        offset += length;

        if (type == TWS_DEVINFO_DATA_TYPE_LEN) {
            continue;
        }

        ParseDeviceInfo(type, value, devInfo);
    }

    TwsClientData devInfoData(peerAddr);
    devInfoData.subCmd_ = static_cast<uint8_t>(TwsAtCmdType::DEVICE_INFO);
    devInfoData.devInfo_ = devInfo;

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN_RET(twsService, false, "[Tws MsgProc]:update device info fail,Tws HiBox instance invalid!");
    twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD), devInfoData);
    return true;
}

/* 处理接收到的echo 4,3消息，AT指令透传 */
uint8_t TwsHiBoxParser::ParseAtCmdEntry(const TwsMessage &event)
{
    TwsAtCmdData *atData = reinterpret_cast<TwsAtCmdData *>(event.serviceData_.get());
    NL_CHECK_RETURN_RET((event.serviceDataLen_ != 0) && (event.serviceDataLen_ <= (TWS_AT_CMD_STR_MAX_LEN + 1)) &&
        atData && event.serviceDataLen_ >= sizeof(atData->atCmdType), static_cast<uint8_t>(TwsErrCode::PARAM_INVALID),
        "[Tws HiBox]:parse at cmd failed,param invalid,data len:%{public}u.", event.serviceDataLen_);

    uint16_t atCmdLen = event.serviceDataLen_ - sizeof(atData->atCmdType);
    std::string atCmd(reinterpret_cast<char*>(atData->atCmdStr), atCmdLen);
    NL_CHECK_RETURN_RET(atData->atCmdType == TWS_AT_CMD_TYPE_REPORT ||
        atData->atCmdType == TWS_AT_CMD_TYPE_BATTERY_REPORT, static_cast<uint8_t>(TwsErrCode::PARAM_INVALID),
        "[Tws HiBox]:parse at cmd failed,at cmd type:%{public}u not support.", atData->atCmdType);

    /* 根据关键字(=)分割AT指令字符串 */
    size_t pos = atCmd.find("=");
    NL_CHECK_RETURN_RET(pos != std::string::npos, static_cast<uint8_t>(TwsErrCode::PARAM_INVALID),
        "[Tws HiBox]:parse at cmd failed,format invalid,no =.");
    std::string atCmdType = atCmd.substr(0, pos);
    std::string atCmdValue = atCmd.substr(pos + 1); /* 跳过等号，取后面的参数 */
    NL_CHECK_RETURN_RET(!atCmdType.empty() && !atCmdValue.empty(), static_cast<uint8_t>(TwsErrCode::CMD_NOT_SUPPORT),
        "[Tws HiBox]:parse at cmd failed,atcmd type or value empty.");

    bool procRet = false;
    auto runCallBack = [&event, &atCmdValue, &procRet](std::string atCmd, TwsServiceAtCmdProc atCmdProc) {
        NL_CHECK_RETURN(atCmdProc, "[Tws HiBox]:parse at cmd failed,atcmd:%{public}s no callback.", atCmd.c_str());
        procRet = atCmdProc(RawAddress(event.dev_), atCmdValue);
    };
    NL_CHECK_RETURN_RET(hiboxAtCmdProc_.GetValueAndOpt(atCmdType, runCallBack) && procRet,
        static_cast<uint8_t>(TwsErrCode::CMD_NOT_SUPPORT),
        "[Tws HiBox]:cmd type:%{public}s not support or invalid cmd.", atCmdType.c_str());

    HILOGI("[Tws HiBox]:at cmd parse ok: %{public}s", atCmd.c_str());
    return static_cast<uint8_t>(TwsErrCode::SUCCESS);
}

/* 处理接收到的echo 4,4消息，异常上报 */
void TwsHiBoxParser::ParseAudioHeadsetExcep(const TwsMessage &event)
{
    HILOGI("[TwsHiBoxParser]: Update client data of echo4,4.");
    NL_CHECK_RETURN(event.serviceDataLen_ == sizeof(AudioExceptionData),
        "[Tws MsgProc]:parse audio capability (4,4) fail,role change data len invalid:%{public}u/%{public}u.",
        event.serviceDataLen_, sizeof(AudioExceptionData));

    AudioExceptionData data = *(reinterpret_cast<AudioExceptionData*>(event.serviceData_.get())); /* 解引用指针转结构体赋值 */
    RawAddress peerAddr(event.dev_);
    TwsClientData twsData(peerAddr);
    twsData.audioExcep_ = data;

    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws MsgProc]:send encode request message fail,Tws HiBox instance invalid!");
    uint8_t isPowerOff = static_cast<uint8_t>(TwsPowerState::POWER_OFF);
    bool started = false;
    uint32_t ret = QOSM_AudioDfxGetDspStatus(&started);
    if (ret != 0) {
        HILOGE("[TwsHiBoxParser]: QOSM_AudioDfxGetDspStatus failed, ret:%{public}u", ret);
        SendRspPowerMessage(peerAddr, isPowerOff);
        return;
    }
    /* 如果3s内手机dsp未发生过下电，且目前不处于下电状态 */
    if (started == true) {
        isPowerOff = static_cast<uint8_t>(TwsPowerState::POWER_ON);
        twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_HEADSET_EXCEPTION), twsData);
    }
    SendRspPowerMessage(peerAddr, isPowerOff);
}

/* 对外接口：接收到数据，调用service接口进行处理 */
bool TwsHiBoxParser::RecvMessage(const TwsMessage &event)
{
    /* AT指令在下层分发 */
    if (event.msgType_ == static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AT_CMD)) {
        uint8_t hiboxRet = ParseAtCmdEntry(event);
        SendErrorCodeResp(static_cast<uint8_t>(TwsHiBoxServiceId::SID_DTS_MGMT),
            static_cast<uint8_t>(TwsDtsCmdId::AT_CMD), RawAddress(event.dev_), hiboxRet);
        return true;
    }

    /* 解码后的消息处理 */
    auto runCallBack = [&event](uint8_t, TwsServiceDymLibMsgProc cbkFunc) {
        NL_CHECK_RETURN(cbkFunc, "[Tws HiBox]:recv hibox msg not support:%{public}u.", event.msgType_);
        cbkFunc(event);
    };
    bool ret = recvMsgProc_.GetValueAndOpt(event.msgType_, runCallBack);
    if (!ret && (event.msgDirect_ == TWS_MSG_DIRECT_REQ)) {
        HILOGE("[Tws HiBox]:recv hibox msg type not suppport:%{public}u", event.msgType_);
        return false;
    }

    return true;
}

std::string TwsHiBoxParser::ConvertToUpper(const std::string& input)
{
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return result;
}

} // namespace Sle
} // namespace OHOS