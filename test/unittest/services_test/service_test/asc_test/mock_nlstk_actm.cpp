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

// 测试目的：NLSTK Actm C-API 级 mock，补齐栈边界成功路径（权威账 §10 路径一"修 mock"）。
// 背景：nl_asc_service_test 被测对象是 ASCService 状态机，但产品码
// ASCService::OpenStream/StartStream/StopStream/ReleaseStream/CreateStream/DisconnProcAction
// （services/service/src/asc/ASCService.cpp:2548/:2596/:2651/:3042/:3318/:376）会同步调用
// SleASC 适配层的 NLSTK_Actm* 真栈 C API（services/service/src/adapter/SleASC.cpp）。
// 裸进程下真栈未初始化，这些接口同步返回错误码，产品码按设计正确传播失败：
// 早退不执行 SetASCStatus/SetProcessingStreamType，或经 ReportAudioControlCompleteSub
// （ASCService.cpp:1319）把状态机回退到 NL_SLE_ASC_CREATED——造成 11 个存量行为断言失败。
// 本 mock 令栈边界全部返回 NLSTK_ERRCODE_SUCCESS，使被测状态机逻辑走到成功分支，
// 用例原期望保持不变（沿用 ccp_test/mock_nlstk_ccp_ccs_server.cpp 先例）。
// 链接抢占：本文件编入测试可执行文件后，链接器以可执行文件内定义优先解析
// SleASC.cpp 的上述符号引用，libnearlink_stack.so 的真实现不再被调用。
#include "actm_api.h"

#include "log.h"
#include "nlstk_public_define_ext.h"

uint32_t NLSTK_ActmReadRemoteProp(SLE_Addr_S *addr)
{
    HILOGI("[Actm Mocker] NLSTK_ActmReadRemoteProp");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmConfigAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmConfigAudioStream");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmStartAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmStartAudioStream");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmOpenAudioStream(SLE_Addr_S *addr, NLSTK_ActmOpenParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmOpenAudioStream");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmChangeAudioStream(SLE_Addr_S *addr, NLSTK_ActmChangeParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmChangeAudioStream op %{public}u", param->op);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmReleaseAudioStream(SLE_Addr_S *addr, NLSTK_ActmReleaseParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmReleaseAudioStream");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmSetDirection(SLE_Addr_S *addr, uint8_t direction)
{
    HILOGI("[Actm Mocker] NLSTK_ActmSetDirection %{public}u", direction);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmDisconnect(SLE_Addr_S *addr)
{
    HILOGI("[Actm Mocker] NLSTK_ActmDisconnect");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmUpdateBitrate(SLE_Addr_S *addr, NLSTK_ActmBitrateParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmUpdateBitrate");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmCreateStream(SLE_Addr_S *addr, NLSTK_ActmStreamParam_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmCreateStream pointType %{public}u", param->pointType);
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_ActmRecvAutoRateMsg(SLE_Addr_S *addr, NLSTK_ActmAutoRateRecvMsg_S *param)
{
    HILOGI("[Actm Mocker] NLSTK_ActmRecvAutoRateMsg");
    return NLSTK_ERRCODE_SUCCESS;
}
