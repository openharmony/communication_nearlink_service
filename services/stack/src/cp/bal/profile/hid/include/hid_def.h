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
#ifndef HID_DEF_H
#define HID_DEF_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

// 人机交互服务profile的连接状态，这个枚举用来通知上层人机交互服务profile的连接状态变化
typedef enum {
    HID_CONNECTING,
    HID_CONNECTED,
    HID_DISCONNECTING,
    HID_DISCONNECTED,
} HidConnectState_E;

// 人机交互服务属性的类型，这个枚举用来指示读、写、通知指令或回调作用的对应属性类型
typedef enum {
    HID_TYPE_AND_FORMAT_DESC,
    HID_WORK_STATUS_INDICATION,
    HID_REPORT_INDEX_INFO,
    HID_INPUT_REPORT_INFO,
    HID_OUTPUT_REPORT_INFO,
    HID_FEATURE_REPORT_INFO,
    HID_PROPERTY_TYPE_BUFF,
} HidPropertyType_E;

// 人机交互服务报告的类型标识，这个枚举用来指示数据字段reportType
typedef enum {
    HID_INVALID_REPORT_TYPE = 0x0,
    HID_INPUT_REPORT_TYPE = 0x01,
    HID_OUTPUT_REPORT_TYPE = 0x02,
    HID_FEATURE_REPORT_TYPE = 0x03,
} HidReportType_E;

/**
 * @brief 属性变更通知回调函数
 *
 * 该回调函数用于处理服务端发来的的属性变更通知。
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] type 属性类型，标识发生变更的属性类型
 * @param[in] value 报告信息，标识发生变更的报告信息属性值
 */
typedef void (*HidNotifyPropertyCbk)(SLE_Addr_S *addr, HidPropertyType_E type, void *value);

/**
 * @brief 读取属性回调函数
 *
 * 该回调函数用于处理上层向服务端发送读取属性请求后的响应结果。
 *
 * 读取属性的方式由模块内部具体实现决定
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] type 属性类型，标识发送读取请求的属性类型
 * @param[in] value 属性值，标识读取到的对端属性值
 * @param[in] ret 结果码，标识发送读取请求的响应结果
 */
typedef void (*HidReadPropertyCbk)(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret);

/**
 * @brief 写入属性回调函数
 *
 * 该回调函数用于处理上层向服务端发送写入属性请求后的响应结果。
 *
 * 写入属性的方式由模块内部具体实现决定。
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] type 属性类型，标识发送写入请求的属性类型；支持写入的属性有：工作状态指示、输出报告信息、特性报告信息
 * @param[in] ret 结果码，标识发送写入请求的响应结果
 */
typedef void (*HidWritePropertyCbk)(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret);

/**
 * @brief profile连接状态变化回调函数
 *
 * 该回调函数用于通知上层HID连接状态变化。
 *
 * 若上层调用HidConnect时已连接或HidDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] state 连接状态，标识当前的连接状态
 * @param[in] preState 连接状态，标识上一个连接状态
 * @param[in] ret 结果码，标识状态改变的原因
 */
typedef void (*HidConnectStateChangeCbk)(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState,
                                         NLSTK_Errcode_E ret);

// HidClientCallBack_S结构体中的函数指针，用来设置各个操作或事件的回调函数
typedef struct {
    HidNotifyPropertyCbk notifyPropertyCbk;
    HidReadPropertyCbk readPropertyCbk;
    HidWritePropertyCbk writePropertyCbk;
    HidConnectStateChangeCbk connectStateChangeCbk;
} HidClientCallBack_S;

// HID_TYPE_AND_FORMAT_DESC对应的属性结构体
typedef struct {
    uint8_t type;               // 类型指示：当类型指示为0x00时，指示采用报告描述符指示的报告格式
    uint8_t *desc;              // 报告描述符，当类型指示为0x01或0x02时，该部分不存在
    uint16_t descLen;           // 报告描述符长度，当类型指示为0x01或0x02时，该部分无效
} HidTypeAndFormatDesc_S;

// HID_REPORT_INDEX_INFO对应的属性结构体
typedef struct {
    uint8_t reportId;           // 报告标识
    uint8_t reportType;         // 报告类型，其中0x01指示输入报告，0x02指示输出报告，0x03指示特性报告
    uint16_t reportHandle;      // 与报告标识和报告类型匹配的报告信息的句柄
    uint16_t reportSrcPort;     // 与报告标识和报告类型匹配的报告信息对应的数据面源端口
    uint16_t reportDestPort;    // 与报告标识和报告类型匹配的报告信息对应的数据面目的端口
} HidReportIndexInfo_S;

typedef struct {
    uint8_t reportId;                               // 报告标识
    uint8_t reportType;                             // 报告类型
} HidReportIdAndType_S;

// HID_INPUT_REPORT_INFO, HID_OUTPUT_REPORT_INFO, HID_FEATURE_REPORT_INFO对应的属性结构体，实际属性部分为reportInfoValue
typedef struct {
    HidReportIdAndType_S reportIdAndType;           // 报告标识和报告类型
    NLSTK_VariableData_S reportInfoValue;     // 报告数据值
} HidReportInfo_S;

// 当上层调用HidGetInformation时，返回的描述和报告信息结构体
typedef struct {
    HidTypeAndFormatDesc_S desc;    // 类型和格式描述
    HidReportInfo_S *reportInfo;    // 报告信息数组
    uint16_t reportNum;             // 报告信息数量
} HidInformation_S;

typedef void (*HidFreeFunc)(void *ptr);

// 人机交互服务属性的数据格式，这个结构体用来标识人机交互服务属性的数据格式
typedef struct {
    HidTypeAndFormatDesc_S desc;            // 类型和格式描述
    uint8_t workStateInd;                   // 工作状态指示
    HidReportIndexInfo_S reportInfo;        // 报告索引信息
    HidReportInfo_S inputReportInfo;        // 输入报告信息
    HidReportInfo_S outputReportInfo;       // 输出报告信息
    HidReportInfo_S featureReportInfo;       // 特性报告信息
} HidProperty_S;

#ifdef __cplusplus
}
#endif

#endif