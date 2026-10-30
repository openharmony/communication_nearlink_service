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
#ifndef TEST_FUZZTEST_CM_FUZZER_UTIL_H
#define TEST_FUZZTEST_CM_FUZZER_UTIL_H

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"

template <typename T1, typename T2, typename T3>
inline void CM_FUZZER_DATA_OFFSET(T1 &_value, T2 _data, T3 &_offset)
{
    _value = (*static_cast<T1 *>(static_cast<void *>(_data + _offset)));
    _offset += sizeof(T1);
}

void MockDliCmdExecuteCbk(DLI_ExecuteCmdRetParam *cmdParam);

template <typename T>
inline void CmFuzzerSleCompleteEvt(uint16_t cmdOpcode, uint16_t cbkCmdOpcode, T &eventParameter)
{
    uint8_t size = (std::is_same<decltype(eventParameter), std::nullptr_t>::value) ? 0 : sizeof(T);
    void *eventP = (size == 0) ? NULL : &eventParameter;
    DLI_CommandComplete cmdCompleteEvt = { 0 };
    cmdCompleteEvt.cmdOpcode = cmdOpcode;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = cbkCmdOpcode,
        .size = size,
        .eventParameter = eventP,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerGenDifferentAddress(SLE_Addr_S *addr, uint32_t handle);

void CmFuzzerSleConnectCompleteEvt(uint16_t handle);

// 与 CmFuzzerSleConnectCompleteEvt 的区别：完成事件携带调用方指定的对端地址（覆盖正常直连分支），
// status 注入到 DLI_ConnectionCompleteEvt.status（非 0 走建链失败路径）
void CmFuzzerSleConnectCompleteEvtWithAddr(uint16_t handle, const SLE_Addr_S *addr, uint8_t status);

// SetMcs 完成事件回注：CM_SleSetMcsProc 从 context->connHandle 取 lcid（cmdRes 为空），
// 必须构造携带真实 connHandle 的 DLI_ConnCbkContext
void CmFuzzerSetMcsEvt(uint16_t connHandle, uint8_t status);

void CmFuzzerReadRemoteVersionCompleteEvt(DLI_ReadRemoteVersionEvt *evt);

void CmFuzzerReadRemoteFeaturesCompleteEvt(DLI_ReadRemoteFeatsEvt *evt);

void CmFuzzerSleDisconnectEvt(uint16_t handle, uint8_t reason);

void CmFuzzerSleConnectParamUpdateEvt(DLI_ConnectionUpdateCmpEvt *evt);

void CmFuzzerSleConnectRemoteParamUpdateReqEvt(DLI_RemoteConnParamReqEvt *evt);

void CmFuzzerSetPhyEvt(DLI_SetPhyEvt *evt);

/* ---- PR-10 DLI 帧事件时序注入族（经 CM_COMMON 适配器表/CM 命令表真实分发）---- */
// 反注册时按 module 清表槽（cm_dli_mocker.c 的 DLI_CmdCbkUnReg 转调）
void CmFuzzerUtilClearModuleTable(ModuleType module);

// 带 status/自定义 context 的分发底座
void CmFuzzerDispatchCmdStatus(uint16_t cbkOpcode, void *evt, uint32_t evtSize, uint8_t status, uint16_t connHandle,
    void *rawContext);

// 完整版连接完成事件：role 区分 G/T 节点，connCompleteType 区分主动(SCAN)/被动(ADV)，
// evt status 非 0 走建链失败路径
void CmFuzzerSleConnectCompleteEvtEx(uint16_t handle, const SLE_Addr_S *addr, uint8_t status, uint8_t role,
    uint8_t connCompleteType);

// 连接命令级错误注入（DISALLOWED 忽略/TIMEOUT 取消连接中链路/其他错误直接返回）
void CmFuzzerSleConnectErrEvt(uint8_t dliStatus);

// 断开事件变体：cmdOpcode 传 DLI_CMD_ERROR_EVT 追加注入命令错误分支
void CmFuzzerSleDisconnectEvtEx(uint16_t handle, uint8_t reason, uint16_t cmdOpcode);

// CM 表通用事件注入（ACB subrate/高功率/RxDataFilter/DataLenChange/RSSI 等）
void CmFuzzerGenericCmEvt(uint16_t cbkOpcode, void *evt, uint32_t evtSize, uint8_t status, uint16_t connHandle);

// SET_PEER_DEV_TYPE 专用（context 直接是 SLE_Addr_S*）
void CmFuzzerPeerDevTypeEvt(const SLE_Addr_S *addr, uint8_t status);

#endif // TEST_FUZZTEST_CM_FUZZER_UTIL_H
