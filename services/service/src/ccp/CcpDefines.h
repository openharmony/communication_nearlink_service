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
#ifndef CCS_DEFINES_H
#define CCS_DEFINES_H

#include <cstdint>
#include <vector>

namespace OHOS {
namespace Nearlink {

constexpr uint8_t MAX_USER_INFO_LEN = 255;
constexpr uint8_t CCP_INSTANCE_ID = 0x01;           // 此为通话控制服务中的媒体实例标识必选字段，当前协议并未明确规定使用，值为模块内开发自定义
constexpr int32_t INVALID_CCP_VOIP_ID = -1;
constexpr uint8_t NEARLINK_CCP_ID_MIN = 0x01;
constexpr uint8_t NEARLINK_CCP_ID_MAX = 0xff;
constexpr int32_t NEARLINK_CCP_VOIP_MAX = -1;
constexpr int32_t NEARLINK_CCP_VOIP_MIN = -255;
constexpr int32_t CALL_MANAGER_MAX_CALL_ID = 10;    // CallManager蜂窝对应的最大CallId是7，这里用10以下统一表示CallManager蜂窝通话
constexpr const char* CCP_SUPPORT_PROTOCOLS = "tel:";

enum class BitSet : uint8_t {
    RESET = 0x00,
    USE = 0x01,
};

/* 协议字段定义：呼入呼出信息属性中的“通话标记”字段 */
typedef union {
    struct {
        uint8_t callDirect : 1;           /* 通话方向 */
        uint8_t hideUserInfoInNet : 1;    /* 用户信息是否在网络侧隐藏 */
        uint8_t hideUserInfoInServer : 1; /* 用户信息是否在服务端隐藏 */
        uint8_t rsv : 5;                  /* 预留 */
    } bits;
    uint8_t value;
} CallFlagBits;

/* 协议字段定义：通话状态属性中的“通话状态”字段 */
enum class CallStatus {
    CALL_STATUS_INCOMING = 0x01,
    CALL_STATUS_OUTGOING,
    CALL_STATUS_ALERTING,
    CALL_STATUS_ACTIVE,
    CALL_STATUS_HOLDING_LOCAL,
    CALL_STATUS_HOLDING_REMOTE,
    CALL_STATUS_HOLDING_LOCAL_AND_REMOTE,
};

/* 协议字段定义：通话终止属性中的“终止原因”字段 */
enum class CallTerminateType {
    CALL_DISCONNECTED_BY_CLIENT = 0x01,
    CALL_DISCONNECTED_BY_SERVER,
    CALL_DISCONNECTED_BY_REMOTE,
    CALL_BUSY,
    CALL_NO_ANSWER,
    CALL_NO_NETWORK,
};

/* NLSTK_CcpCallInOutInfo_S */
class CallInOutInfoProp {
public:
    CallInOutInfoProp() = default;
    ~CallInOutInfoProp() = default;

    uint8_t callId = 0;                     /**< 通话标识 */
    uint8_t networkId = 0;                  /**< 网络标识 */
    uint8_t callFlag = 0;                   /**< 通话标记 */
    char userInfo[MAX_USER_INFO_LEN] = { 0 };  /**< 用户信息 */
    char userAlias[MAX_USER_INFO_LEN + 1] = { 0 }; /**< 用户别名 */
};

/* NLSTK_CcpCallStatues_S */
class TotalCallStateProp {
public:
    class SingleCallStateProp {
    public:
        SingleCallStateProp() = default;
        ~SingleCallStateProp() = default;

        uint8_t callId = 0;     /**< 通话标识 */
        uint8_t networkId = 0;  /**< 网络标识 */
        uint8_t callStatus = 0; /**< 通话状态 */
        uint8_t callFlag = 0;   /**< 通话标记 */
    };

    TotalCallStateProp() = default;
    ~TotalCallStateProp() = default;

    std::vector<SingleCallStateProp> callStateVec{};
};

class CallTerminateInfoProp {
public:
    CallTerminateInfoProp() = default;
    ~CallTerminateInfoProp() = default;

    uint8_t callId = 0;          /**< 通话标识 */
    uint8_t terminateReason = 0; /**< 终止原因 */
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  // CCS_DEFINES_H