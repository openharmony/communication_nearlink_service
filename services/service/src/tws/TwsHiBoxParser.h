/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef TWS_HIBOX_PARSER_H
#define TWS_HIBOX_PARSER_H

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <mutex>
#include <queue>

#include "SleInterfaceProfileTws.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "context.h"
#include "log_util.h"
#include "DeviceBatteryManager.h"

namespace OHOS {
namespace Nearlink {

class TwsHiBoxParser {
public:
    explicit TwsHiBoxParser();
    ~TwsHiBoxParser() = default;

    static TwsHiBoxParser &GetInstance();

    /* 发送消息接口 */
    bool SendMessage(const RawAddress &peerAddr, uint8_t msgType, uint8_t* data, uint16_t dataLen, bool isRespMsg);

    /* 统一消息发送接口 */
    bool SendMessageEntry(const RawAddress &peerAddr, uint8_t msgType, uint8_t *data, uint16_t dataLen, bool isRespMsg);

    /* 接收消息接口 */
    bool RecvMessage(const TwsMessage &event);

private:
    std::vector<uint8_t> dtsCap_;           /* 手机侧数据传输能力列表 */

    std::queue<TwsMessage> waitRspQueue_;   /* 待确认方案：发送请求给对端时，需要等待对端响应消息的队列 */

    /* 指令回调类型定义 */
    using TwsServiceDymLibMsgProc = void (*)(const TwsMessage &event);                               /* echo指令回调 */
    using TwsServiceAtCmdProc = bool (*)(const RawAddress &peerAddr, const std::string &atCmdParam); /* AT指令回调函数 */

    template <typename T>
    static bool ConvertStrToNumber(const std::string &src, T &ret, int base);
    static bool AtCmdStartWith(const std::string &str, const std::string &prefix);
    static uint16_t CalcMessageCrc(uint8_t *buf, uint16_t len);

    /* 公共接口：上报框架统一入口 */
    static bool SendRemoteInfo(const RawAddress &peerAddr, const std::string &value);

    /* 消息发送接口 */
    // echo 4,4
    static bool SendRspPowerMessage(const RawAddress &peerAddr, uint8_t isPowerOff);
    // echo 4,1
    bool SendDtsCapMessage(const RawAddress &peerAddr, bool isRespMsg);

    /* 接收消息解析接口 */
    // echo指令回调列表
    NearlinkSafeMap<uint8_t, TwsServiceDymLibMsgProc> recvMsgProc_ {};
    // echo 1,2
    static void ParseAbilityBitMap(const TwsMessage &event);
    // echo 3,2
    static void ParseIsoHandover(const TwsMessage &event);
    // echo 4,1
    static void ParseDtsCap(const TwsMessage &event);
    // echo 4,2
    static void ParseWearStatus(const TwsMessage &event);
    // echo 5,8
    static void ParseEarbudsNature(const TwsMessage &event);
    /* echo 5,11 */
    static void ParseNotifySleDisconnectProfile(const TwsMessage &event);
    // echo 5,a
    static void ParseQueryBusiness(const TwsMessage &event);
    //  echo 5,e
    static void ParseAutoConnectSwitch(const TwsMessage &event);
    /* echo 8,a */
    static void ParseAudioCap(const TwsMessage &event);
    /* echo 8,b */
    static void ParseProfileState(const TwsMessage &event);
    /* echo 8,c */
    static void ParseOutDatapath(const TwsMessage &event);
    /* echo 8,E */
    static void ParseDualRecCodecInfo(const TwsMessage &event);
    static DeviceDualRecParmInfo SetPeerDualRecCodecParamConfig(const HiBoxNearlinkDualRecCaps& data);

    /* echo 4,3 AT指令解析相关 */
    // AT指令回调列表
    NearlinkSafeMap<std::string, TwsServiceAtCmdProc> hiboxAtCmdProc_ {};
    // 公共接口
    static std::vector<std::string> SplitValueByDelim(const std::string &commandValue, const std::string &delim);
    static bool SendErrorCodeResp(const uint8_t serviceId, const uint8_t cmdId,
        const RawAddress &peerAddr, uint8_t errCode);

    // 对端AT指令处理
    // echo 4,3总入口
    uint8_t ParseAtCmdEntry(const TwsMessage &event);
    static bool ParseAtCmdUpdateDeviceInfo(const RawAddress &peerAddr, const std::string &atCmdParam);
    static bool ParseAtCmdBatteryDialog(const RawAddress &peerAddr, const std::string &atCmdParam);
    static bool ParseAtCmdUpdateBattery(const RawAddress &peerAddr, const std::string &atCmdParam);
    static bool ParseAtCmdCloseBattery(const RawAddress &peerAddr, const std::string &atCmdParam);
    static bool ParserAtCmdVendorBattery(const RawAddress &peerAddr, const std::string &atCmdParam,
        BatteryInfo &batteryInfo);

    static void UpdateVendorBatteryInfo(BatteryInfo &dataBlock, uint8_t type, uint32_t value);
    static void ParseDeviceInfo(const uint8_t dataType, const std::string &data, TwsDeviceDatas &devInfo);

    static std::string ConvertToUpper(const std::string &input);

    // echo 4,4
    static void ParseAudioHeadsetExcep(const TwsMessage &event);
    // echo 8,E rsp
    static void DualRecCodecInfoRsp(const RawAddress &peerAddr, DeviceDualRecParmInfo info);
};

} // namespace Sle
} // namespace OHOS
#endif /* TWS_HIBOX_PARSER_H */