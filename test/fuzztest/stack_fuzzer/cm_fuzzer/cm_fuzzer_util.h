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

void CmFuzzerReadRemoteVersionCompleteEvt(DLI_ReadRemoteVersionEvt *evt);

void CmFuzzerReadRemoteFeaturesCompleteEvt(DLI_ReadRemoteFeatsEvt *evt);

void CmFuzzerSleDisconnectEvt(uint16_t handle, uint8_t reason);

void CmFuzzerSleConnectParamUpdateEvt(DLI_ConnectionUpdateCmpEvt *evt);

void CmFuzzerSleConnectRemoteParamUpdateReqEvt(DLI_RemoteConnParamReqEvt *evt);

void CmFuzzerSetPhyEvt(DLI_SetPhyEvt *evt);

#endif // TEST_FUZZTEST_CM_FUZZER_UTIL_H
