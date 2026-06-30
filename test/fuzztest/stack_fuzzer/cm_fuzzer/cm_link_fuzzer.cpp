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
#include "cm_link_fuzzer.h"
#include <cstddef>
#include <type_traits>
#include "securec.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_worker.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_logic_link_api.h"
#include "cm_api.h"
#include "cm_inner_api.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_fuzzer_util.h"
// DLI 相关头文件
#include "dli_event_struct.h"
#include "dli_errno.h"

typedef struct {
    uint8_t status;
    uint16_t connHandle;         /*!< 连接句柄 */
} DLI_RemConnParamReqReplyReturnParam;

namespace OHOS {
    void CM_ConnectReadLocalFeaturesFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(DLI_ReadLocalFeatsEvt)), "Input size error.");
        DLI_ReadLocalFeatsEvt fevt = { 0 };
        uint32_t offset = 0;
        for (uint8_t i = 0; i < sizeof(fevt.feats) / sizeof(fevt.feats[0]); i++) {
            CM_FUZZER_DATA_OFFSET(fevt.feats[i], data, offset);
        }
        CmFuzzerSleCompleteEvt(DLI_READ_LOCAL_SUPPORT_FEATS, DLI_CBK_READ_LOCAL_FEATURE, fevt);
    }

    void CM_ConnectEstablishReqFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(CM_ConnectParamReq_S) + sizeof(uint16_t)), "Input size error.");
        CM_ConnectParamReq_S param = {0};
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(param.version, data, offset);
        CM_FUZZER_DATA_OFFSET(param.localIndex, data, offset);
        CM_FUZZER_DATA_OFFSET(param.addr, data, offset);
        uint32_t ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle;
        CM_FUZZER_DATA_OFFSET(handle, data, offset);
        CmFuzzerSleConnectCompleteEvt(handle);
        ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        CmFuzzerSleConnectCompleteEvt(handle);
    }

    void CM_ConnectReadRemoteFeturesAndVersionFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(DLI_ReadRemoteVersionEvt) + sizeof(DLI_ReadRemoteFeatsEvt) +
            sizeof(DLI_DataLenChangeEvt)), "Input size error.");
        uint32_t offset = 0;
        DLI_ReadRemoteVersionEvt evt = { 0 };
        CM_FUZZER_DATA_OFFSET(evt.status, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.connHandle, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.version, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.companyIdentifier, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.subversion, data, offset);
        CmFuzzerReadRemoteVersionCompleteEvt(&evt);
        DLI_ReadRemoteFeatsEvt fevt = { 0 };
        CM_FUZZER_DATA_OFFSET(fevt.status, data, offset);
        CM_FUZZER_DATA_OFFSET(fevt.connHandle, data, offset);
        for (uint32_t i = 0; i < (sizeof(fevt.feats) / sizeof(fevt.feats[0])); i++) {
            CM_FUZZER_DATA_OFFSET(fevt.feats[i], data, offset);
        }
        CmFuzzerReadRemoteFeaturesCompleteEvt(&fevt);
        DLI_DataLenChangeEvt devt = { 0 };
        CM_FUZZER_DATA_OFFSET(devt.connHandle, data, offset);
        CM_FUZZER_DATA_OFFSET(devt.maxTxOctets, data, offset);
        CM_FUZZER_DATA_OFFSET(devt.maxRxOctets, data, offset);
        CmFuzzerSleCompleteEvt(DLI_SET_DATA_LEN, DLI_CBK_SET_DATA_LEN, devt);
    }

    void CM_ConnectSetParamReqFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(CM_ConnectSetParamReq_S), "Input size error.");
        CM_ConnectSetParamReq_S param = {0};
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(param.enableFilterPolicy, data, offset);
        CM_FUZZER_DATA_OFFSET(param.gtNegotiate, data, offset);
        CM_FUZZER_DATA_OFFSET(param.scanInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(param.scanWindow, data, offset);
        CM_FUZZER_DATA_OFFSET(param.minInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(param.maxInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(param.timeout, data, offset);
        uint32_t ret = CM_ConnectSetParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
    }

    void CM_ConnectRemoteUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(DLI_RemoteConnParamReqEvt)), "Input size error.");
        DLI_RemoteConnParamReqEvt revt = { 0 };
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(revt.connHandle, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.connIntervalMin, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.connIntervalMax, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.txRxInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.eventInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.maxLatency, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.supervisionTimeout, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.systemTimeUnit, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.txRxFlag, data, offset);
        CmFuzzerSleConnectRemoteParamUpdateReqEvt(&revt);
    }

    typedef struct {
        uint8_t status;
        uint16_t connHandle;         /*!< 连接句柄 */
    } DLI_RemoteConnParamReqReplyReturnParam;

    void CM_ConnectRemoteConnectionParamRequestsReplyFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(DLI_RemoteConnParamReqReplyReturnParam)), "Input size error.");
        DLI_RemoteConnParamReqReplyReturnParam revt = { 0 };
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(revt.status, data, offset);
        CM_FUZZER_DATA_OFFSET(revt.connHandle, data, offset);
        CmFuzzerSleCompleteEvt(DLI_CONNECTION_PARAM_REQ_REPLY, DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, revt);
    }

    void CM_ConnectUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(CM_ConnectUpdateParamReq_S) + sizeof(DLI_ConnectionUpdateCmpEvt)),
            "Input size error.");
        CM_ConnectUpdateParamReq_S param = {0};
        uint32_t offset = 0;

        CM_FUZZER_DATA_OFFSET(param.localIndex, data, offset);
        CM_FUZZER_DATA_OFFSET(param.addr, data, offset);
        CM_FUZZER_DATA_OFFSET(param.intervalMin, data, offset);
        CM_FUZZER_DATA_OFFSET(param.intervalMax, data, offset);
        CM_FUZZER_DATA_OFFSET(param.txRxInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(param.eventInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(param.maxLatency, data, offset);
        CM_FUZZER_DATA_OFFSET(param.supervisionTimeout, data, offset);
        CM_FUZZER_DATA_OFFSET(param.systemTimeUnit, data, offset);
        CM_FUZZER_DATA_OFFSET(param.txRxFlag, data, offset);
        uint32_t ret = CM_ConnectUpdateParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
        DLI_ConnectionUpdateCmpEvt evt = { 0 };
        CM_FUZZER_DATA_OFFSET(evt.status, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.connHandle, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.connInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.txRxInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.eventInterval, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.maxLatency, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.supervisionTimeout, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.systemTimeUnit, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.txRxFlag, data, offset);
        CmFuzzerSleConnectParamUpdateEvt(&evt);
    }

    void CM_ConnectReleaseReqFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(CM_DisconnectParamReq_S) +
            sizeof(uint16_t) + sizeof(uint8_t), "Input size error.");
        uint32_t offset = 0;
        CM_DisconnectParamReq_S param = {};
        CM_FUZZER_DATA_OFFSET(param.addr, data, offset);
        SLE_Addr_S addr = {0};
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        CM_FUZZER_DATA_OFFSET(param.discReason, data, offset);
        uint32_t ret = CM_ConnectReleaseReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectReleaseReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle;
        uint16_t reason;
        CM_FUZZER_DATA_OFFSET(handle, data, offset);
        CM_FUZZER_DATA_OFFSET(reason, data, offset);
        CmFuzzerSleDisconnectEvt(handle, reason);
    }

    void CM_ConnectCancelFuzz(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        uint32_t ret = CM_ConnectCancelReq();
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_ConnectCancelReq failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_CREATE_CONNECTION_CANCEL, DLI_CBK_CONNECT_CANCEL, evt);
    }

    void CM_ClearAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        CM_ClearAcceptFilterList();
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_CLEAR_ACCESS_FILTER_LIST, DLI_CBK_CLEAR_ACCESS_FLT_LIST, evt);
    }

    void CM_ReadAcceptFilterListSizeFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(DLI_ReadBufSizeEvt), "Input size error.");
        uint32_t offset = 0;
        CM_ReadAcceptFilterListSize();
        DLI_ReadBufSizeEvt evt = { 0 };
        CM_FUZZER_DATA_OFFSET(evt.acbTxDataLen, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.acbTxDataNum, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.icbTxDataLen, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.icbTxDataNum, data, offset);
        CmFuzzerSleCompleteEvt(DLI_READ_ACCESS_FILTER_LIST_SIZE, DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, evt);
    }

    void CM_AddDeviceToAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(SLE_Addr_S), "Input size error.");
        SLE_Addr_S addr = {0};
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(addr, data, offset);
        uint32_t ret = CM_AddDeviceToAcceptFilterList(&addr, false);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_AddDeviceToAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST, DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, evt);
    }

    void CM_RemoveDeviceFromAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(SLE_Addr_S), "Input size error.");
        SLE_Addr_S addr = {0};
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(addr, data, offset);
        uint32_t ret = CM_RemoveDeviceFromAcceptFilterList(&addr);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_RemoveDeviceFromAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST, DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, evt);
    }

    void CM_SetPhyFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= (sizeof(CM_SetPhyReq_S) + sizeof(DLI_SetPhyEvt)), "Input size error.");
        CM_SetPhyReq_S param = {0};
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(param.lcid, data, offset);
        CM_FUZZER_DATA_OFFSET(param.txFormat, data, offset);
        CM_FUZZER_DATA_OFFSET(param.rxFormat, data, offset);
        CM_FUZZER_DATA_OFFSET(param.txPhy, data, offset);
        CM_FUZZER_DATA_OFFSET(param.rxPhy, data, offset);
        CM_FUZZER_DATA_OFFSET(param.txPilotDensity, data, offset);
        CM_FUZZER_DATA_OFFSET(param.rxPilotDensity, data, offset);
        CM_FUZZER_DATA_OFFSET(param.gFeedback, data, offset);
        CM_FUZZER_DATA_OFFSET(param.tFeedback, data, offset);
        uint32_t ret = CM_SetPhy(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetPhy failed, ret:%d", ret);
        DLI_SetPhyEvt evt = { 0 };
        CM_FUZZER_DATA_OFFSET(evt.status, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.connHandle, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.txFormat, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.rxFormat, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.txPhy, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.rxPhy, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.txPilotDensity, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.rxPilotDensity, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.gFeedback, data, offset);
        CM_FUZZER_DATA_OFFSET(evt.tFeedback, data, offset);
        CmFuzzerSetPhyEvt(&evt);
    }

    void CM_SetHostChannelClassificationFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(CM_SetChannelMapReq_S), "Input size error.");
        CM_SetChannelMapReq_S param = {0};
        uint32_t offset = 0;
        for (uint32_t i = 0; i < (sizeof(param.channelMap) / sizeof(param.channelMap[0])); i++) {
            CM_FUZZER_DATA_OFFSET(param.channelMap[i], data, offset);
        }
        uint32_t ret = CM_SetHostChannelClassification(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetHostChannelClassification failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_SET_HOST_CHANNEL_CLASSIFICATION, DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, evt);
    }

    void CM_GetLogicLinkByAddrFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(SLE_Addr_S), "Input size error.");
        SLE_Addr_S addr = { 0 };
        CM_LogicLink_S logicLink = { 0 };
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(addr, data, offset);
        uint32_t ret = CM_GetLogicLinkByAddr(&addr, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByAddr failed, ret:%d", ret);
    }

    void CM_GetLogicLinkByLcidFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(uint16_t), "Input size error.");
        uint16_t lcid = 0;
        CM_LogicLink_S logicLink = { 0 };
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(lcid, data, offset);
        uint32_t ret = CM_GetLogicLinkByLcid(lcid, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByLcid failed, ret:%d", ret);
    }

    void CM_SetLogicLinkDeviceTypeFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(uint16_t) + sizeof(uint8_t), "Input size error.");
        uint16_t lcid = 0;
        uint8_t deviceType = 0;
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(lcid, data, offset);
        CM_FUZZER_DATA_OFFSET(deviceType, data, offset);
        CM_SetLogicLinkDeviceType(lcid, deviceType);
    }

    void CM_SetACBSubrateFuzz(uint8_t *data, size_t size)
    {
        CM_CHECK_RETURN(size >= sizeof(CM_SetACBSubrateInnerParam), "Input size error.");
        CM_SetACBSubrateInnerParam param;
        uint32_t offset = 0;
        CM_FUZZER_DATA_OFFSET(param, data, offset);
        CM_InnerSetACBSubrate(&param);
    }

    void FuzzCmApi(uint8_t *data, size_t size)
    {
        CM_ConnectReadLocalFeaturesFuzz(data, size);
        CM_ConnectEstablishReqFuzz(data, size);
        CM_ConnectReadRemoteFeturesAndVersionFuzz(data, size);
        CM_GetLogicLinkByAddrFuzz(data, size);
        CM_GetLogicLinkByLcidFuzz(data, size);
        CM_ConnectSetParamReqFuzz(data, size);
        CM_ConnectUpdateParamReqFuzz(data, size);
        CM_ConnectRemoteUpdateParamReqFuzz(data, size);
        CM_ConnectRemoteConnectionParamRequestsReplyFuzz(data, size);
        CM_ConnectReleaseReqFuzz(data, size);
        CM_ConnectCancelFuzz(data, size);
        CM_ReadAcceptFilterListSizeFuzz(data, size);
        CM_ClearAcceptFilterListFuzz(data, size);
        CM_AddDeviceToAcceptFilterListFuzz(data, size);
        CM_RemoveDeviceFromAcceptFilterListFuzz(data, size);
        CM_SetPhyFuzz(data, size);
        CM_SetHostChannelClassificationFuzz(data, size);
        CM_GetLogicLinkByAddrFuzz(data, size);
        CM_GetLogicLinkByLcidFuzz(data, size);
        CM_SetLogicLinkDeviceTypeFuzz(data, size);
        CM_SetACBSubrateFuzz(data, size);
    }
}

static void CM_FuzzerChannelState(CM_TransChannelStateList_S *state)
{
}

static void CM_FuzzerSSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
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

static void CM_FuzzerConnCancelCbk(uint8_t *param)
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

uint32_t FuzzCmLinkInit(void)
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
    cmCbks.connCancelCbk = CM_FuzzerConnCancelCbk;
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
    CM_TransChannelCbk cbk = CM_FuzzerChannelState;
    ret = CM_RegTransChannelListener(cbk);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RegConnectCbks failed, ret:%d", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

void FuzzCmLinkApi(uint8_t *data, size_t size)
{
    OHOS::FuzzCmApi(static_cast<uint8_t *>(data), size);
}

void FuzzCmLinkDeInit(void)
{
    CM_DeInit();
}