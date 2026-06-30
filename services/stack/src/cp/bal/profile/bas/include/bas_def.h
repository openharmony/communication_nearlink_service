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
#ifndef BAS_DEF_H
#define BAS_DEF_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"
#include "nlstk_bas_def.h"

#ifdef __cplusplus
extern "C" {
#endif

// 电量信息管理服务属性的类型，这个枚举用来指示读、写、通知指令或回调作用的对应属性类型
typedef enum {
    BAS_BATTERY_PERCENTAGE,
    BAS_BATTERY_CAPACITY,
    BAS_BATTERY_CAPACITY_TOTAL,
    BAS_BATTERY_RATE_CAPACITY,
    BAS_REMAINING_WORKING_TIME,
    BAS_UNSUPPORTED_PROPERTY
} BasPropertyType_E;

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
typedef void (*OnBasGetPropertyCbk)(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret);

/**
 * @brief profile连接状态变化回调函数
 *
 * 该回调函数用于通知上层BAS连接状态变化。
 *
 * 若上层调用BasConnect时已连接或BasDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] curState 连接状态，标识当前的连接状态
 * @param[in] prevState 连接状态，标识上一个连接状态
 * @param[in] errNumb 结果码，标识状态改变的原因
 */
typedef void (*OnBasConnectStateChangeCbk)(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
                                         NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb);

/**
 * @brief 属性变化回调函数
 *
 * 该回调函数用于通知上层BAS属性变化。
 *
 * 若上层调用BasConnect时已连接或BasDisconnect已断连，则会触发该回调将最新的连接状态上报
 *
 * @param[in] addr 对端地址，标识对端设备地址
 * @param[in] type 属性类型，标识发送读取请求的属性类型
 * @param[in] value 属性值，标识读取到的对端属性值
 */
typedef void (*OnBasPropertyChangedCbk)(SLE_Addr_S *addr, BasPropertyType_E type, void *value);

// BasClientCallBack_S结构体中的函数指针，用来设置各个操作或事件的回调函数
typedef struct {
    OnBasGetPropertyCbk readPropertyCbk;
    OnBasConnectStateChangeCbk connectStateChangeCbk;
    OnBasPropertyChangedCbk propertyChangedCbk;
} BasClientCallBack_S;



#ifdef __cplusplus
}
#endif

#endif