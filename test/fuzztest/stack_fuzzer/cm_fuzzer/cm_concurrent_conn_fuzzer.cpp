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
#include "cm_concurrent_conn_fuzzer.h"
#include <cstddef>
#include <type_traits>
#include "fuzzer/FuzzedDataProvider.h"
#include "cm.h"
#include "securec.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_timer.h"
#include "sdf_worker.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_logic_link_api.h"
#include "cm_api.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_fuzzer_util.h"
// DLI 相关头文件
#include "dli_event_struct.h"
#include "dli_errno.h"

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{
}

namespace OHOS {
    void FuzzCmCCDirectConnAddApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        CM_DirectConnAddrParam_S param = {};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.frameType = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_DirectConnectAdd(moduleId, &param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_DirectConnectAdd failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCDirectConnRmvApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        uint8_t discReason = 0;
        CM_DirectConnAddrParam_S param = {};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        discReason = provider.ConsumeIntegral<uint8_t>();
        CM_DirectConnectAdd(moduleId, &param);
        moduleId = (moduleId % 2 == 0 ? 0x10 : 0x21);
        CM_DirectConnectRemove(moduleId, &param.addr, discReason);
    }

    void FuzzCmCCBgConnAddApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        const uint8_t testMaxAddrArrCount = 255;
        uint8_t moduleId = 0;
        uint8_t addrArrCount = 0;
        moduleId = provider.ConsumeIntegral<uint8_t>();
        addrArrCount = provider.ConsumeIntegral<uint8_t>() % testMaxAddrArrCount;
        CM_BgConnAddrParam_S *addrArr = (CM_BgConnAddrParam_S *)SDF_MemZalloc(addrArrCount * sizeof(CM_BgConnAddrParam_S));
        CM_CHECK_RETURN(addrArr != NULL, "malloc failed");
        for (uint8_t i = 0; i < addrArrCount; i++) {
            (addrArr + i)->addr.type = provider.ConsumeIntegral<uint8_t>();
            for (uint8_t j = 0; j < SLE_ADDR_LEN; j++) {
                (addrArr + i)->addr.addr[j] = provider.ConsumeIntegral<uint8_t>();
            }
            (addrArr + i)->isBypass = provider.ConsumeBool();
        }
        uint32_t ret = CM_BackgroundConnectAdd(moduleId, addrArrCount, addrArr);
        SDF_MemFree(addrArr);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectAdd failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCBgConnRmvApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        SLE_Addr_S addr = {0};
        moduleId = provider.ConsumeIntegral<uint8_t>();
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_BackgroundConnectRemove(moduleId, &addr);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectRemove failed, ret:%d", ret);
            return;
        }
    }

    void FuzzCmCCBgConnClearApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t moduleId = 0;
        moduleId = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_BackgroundConnectClear(moduleId);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_BackgroundConnectClear failed, ret:%d", ret);
            return;
        }
    }
}

static void CM_FuzzerChannelState(CM_TransChannelStateList_S *state)
{
}

static void CM_FuzzerSSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
}

static void CM_FuzzerAdapt_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
}

static void CM_FuzzerHADM_CMReadRemoteCsCbk(CM_LogicLinkRemoteFeatures_S *param)
{
}

static void CM_FuzzerConnRemoteParamUpdateReqCbk(CM_ConnectRemoteUpdateParamReq_S *param)
{
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
    CM_LOGI("enter");
}

static void CM_FuzzerConnUpdateParamReqCbk(CM_ConnectUpdateParamRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

static void CM_FuzzerConnSetPhyReqCbk(CM_SetPhyRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

static void CM_FuzzerReadRmtFeatVerCbk(CM_ReadRemoteFeatureVersionRsp_S *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("param is nullptr");
        return;
    }
}

uint32_t FuzzCmConcurrentConnInit(void)
{
    int32_t ret = CM_Init();
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_Init failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LOGI("cm init success");
    CM_ConnectCbks_S cmCbks = { 0 };
    cmCbks.connRemoteUpdateParamReqCbk = CM_FuzzerConnRemoteParamUpdateReqCbk;
    cmCbks.connUpdateParamCbk = CM_FuzzerConnUpdateParamReqCbk;
    cmCbks.setPhyCbk = CM_FuzzerConnSetPhyReqCbk;
    cmCbks.readRemoteFeatureVersionCbk = CM_FuzzerReadRmtFeatVerCbk;
    ret = CM_RegConnectCbks(&cmCbks);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegConnectCbks failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S listener = { 0 };
    listener.moduleId = CM_MODULE_SSAP;
    listener.logicLinkCbk = CM_FuzzerSSAP_CMLogicLinkCbk;
    ret = CM_RegLogicLinkListener(&listener);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S cmLinkCbk = {0};
    cmLinkCbk.moduleId = CM_MODULE_HADM;
    cmLinkCbk.remoteFeaturesCbk = CM_FuzzerHADM_CMReadRemoteCsCbk;
    ret = CM_RegLogicLinkListener(&cmLinkCbk);
    if (CM_RegLogicLinkListener(&cmLinkCbk) != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_LogicLinkCbks_S cmAdptCbk = {0};
    cmAdptCbk.moduleId = CM_MODULE_ADPT;
    cmAdptCbk.logicLinkCbk = CM_FuzzerAdapt_CMLogicLinkCbk;
    ret = CM_RegLogicLinkListener(&cmAdptCbk);
    if (CM_RegLogicLinkListener(&cmAdptCbk) != CM_SUCCESS) {
        CM_LOGE("CM_RegLogicLinkListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    CM_TransChannelCbk cbk = CM_FuzzerChannelState;
    ret = CM_RegTransChannelListener(cbk);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegTransChannelListener failed, ret:%d", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

void FuzzCmConcurrentConnApi(uint8_t *data, size_t size)
{
    OHOS::FuzzCmCCDirectConnAddApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCDirectConnRmvApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnAddApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnRmvApi(static_cast<uint8_t *>(data), size);
    OHOS::FuzzCmCCBgConnClearApi(static_cast<uint8_t *>(data), size);
}

void FuzzCmConcurrentConnDeInit(void)
{
    CM_DeInit();
}