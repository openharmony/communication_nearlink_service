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
#ifndef HID_CLIENT_API_H
#define HID_CLIENT_API_H

#include "hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册人机交互服务profile客户端回调
 *
 * 该函数用于注册人机交互服务profile客户端回调函数，之后profile可通过回调通知上层处理操作或事件。
 *
 * @param[in] clientCallback 回调函数结构体
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidRegClientCbk(HidClientCallBack_S *clientCallback);

/**
 * @brief 连接人机交互设备
 *
 * 该函数用于对人机交互设备进行HID profile连接。
 *
 * HID profile连接包含CM链路的连接、读取初始需要的属性、以及订阅客户端属性配置描述符等。
 *
 * @param[in] addr 对端设备地址
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidConnect(SLE_Addr_S *addr);

/**
 * @brief 断开人机交互设备连接
 *
 * 该函数用于对人机交互设备进行HID profile断连。
 *
 * @param[in] addr 对端设备地址
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidDisconnect(SLE_Addr_S *addr);

/**
 * @brief 获取人机交互服务信息
 *
 * 该函数用于在profile连接成功后，上层主动获取连接获得的对端HID属性信息
 *
 * 获得的对端HID属性信息包括解析出的报告信息、类型和格式描述、工作状态指示
 *
 * @param[in] addr 对端设备地址
 * @param[out] info 对端HID属性信息
 * @param[out] freeFunc 对端HID属性信息的析构函数
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 * @example
 * // 示例用法
 * HidInformation_S *info = NULL;
 * HidFreeFunc freeFunc = NULL;
 * uint32_t result = HidGetInformation(addr, &info, &freeFunc);
 * // 业务处理逻辑
 * if (freeFunc != NULL) {
      freeFunc(info);
 * }
 */
NLSTK_Errcode_E HidGetInformation(SLE_Addr_S *addr, HidInformation_S **info, HidFreeFunc *freeFunc);

/**
 * @brief 读取人机交互服务属性
 *
 * 该函数用于读取人机交互设备的指定属性值。
 *
 * 读取属性的方式由模块内部具体实现决定。
 *
 * @param[in] addr 对端设备地址
 * @param[in] type 要读取的属性类型
 * @param[in] reportIdAndType 报告标识和报告类型，当读取的属性类型为报告信息时有效
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidReadProperty(SLE_Addr_S *addr, HidPropertyType_E type, HidReportIdAndType_S *reportIdAndType);

/**
 * @brief 写入人机交互服务属性
 *
 * 该函数用于向人机交互设备写入指定属性值。
 *
 * 写入属性的方式由模块内部具体实现决定。
 *
 * @param[in] addr 对端设备地址
 * @param[in] type 要写入的属性类型
 * @param[in] value 要写入的属性值，当属性类型为报告信息时，实际写入的属性值为HidReportInfo_S中的reportInfoValue部分
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidWriteProperty(SLE_Addr_S *addr, HidPropertyType_E type, void *value);

/**
 * @brief 获取设备连接状态
 *
 * 该函数用于获取人机交互设备连接状态。
 *
 * @param[in] addr 对端设备地址
 * @param[out] state 设备连接状态
 *
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 */
NLSTK_Errcode_E HidGetConnectState(SLE_Addr_S *addr, uint8_t *state);

/**
 * @brief 获取已连接设备列表
 *
 * 该函数用于获取已连接设备地址列表。
 *
 * @param[out] addrs 已连接设备地址列表
 * @param[out] num 已连接设备数量
 * @param[out] freeFunc 设备地址列表的析构函数
 * @return NLSTK_Errcode_E
 * - NLSTK_ERRCODE_SUCCESS = 0: 成功
 * - 其他值: 错误码（如 NLSTK_ERRCODE_PARAM_ERR, NLSTK_ERRCODE_TIMEOUT 等）
 *
 * @example
 * // 示例用法
 * SLE_Addr_S *addrs = NULL;
 * size_t num = 0;
 * HidFreeFunc freeFunc = NULL;
 * NLSTK_Errcode_E result = HidGetConnectedDevice(&addrs, &num, &freeFunc);
 * // 业务处理逻辑
 * if (freeFunc != NULL) {
 *    freeFunc(addrs);
 * }
 */
NLSTK_Errcode_E HidGetConnectedDevice(SLE_Addr_S **addrs, size_t *num, HidFreeFunc *freeFunc);


#ifdef __cplusplus
}
#endif

#endif