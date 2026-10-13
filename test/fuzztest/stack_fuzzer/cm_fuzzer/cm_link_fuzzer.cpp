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
#include "fuzzer/FuzzedDataProvider.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_worker.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_dyn_trans_channel_api.h"
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
        FuzzedDataProvider provider(data, size);
        DLI_ReadLocalFeatsEvt fevt = { 0 };
        for (uint8_t i = 0; i < sizeof(fevt.feats) / sizeof(fevt.feats[0]); i++) {
            fevt.feats[i] = provider.ConsumeIntegral<uint8_t>();
        }
        CmFuzzerSleCompleteEvt(DLI_READ_LOCAL_SUPPORT_FEATS, DLI_CBK_READ_LOCAL_FEATURE, fevt);
    }

    void CM_ConnectEstablishReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_ConnectEstablishReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectEstablishReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
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
        FuzzedDataProvider provider(data, size);
        DLI_ReadRemoteVersionEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.version = provider.ConsumeIntegral<uint8_t>();
        evt.companyIdentifier = provider.ConsumeIntegral<uint16_t>();
        evt.subversion = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerReadRemoteVersionCompleteEvt(&evt);
        DLI_ReadRemoteFeatsEvt fevt = { 0 };
        fevt.status = provider.ConsumeIntegral<uint8_t>();
        fevt.connHandle = provider.ConsumeIntegral<uint16_t>();
        for (uint32_t i = 0; i < (sizeof(fevt.feats) / sizeof(fevt.feats[0])); i++) {
            fevt.feats[i] = provider.ConsumeIntegral<uint8_t>();
        }
        CmFuzzerReadRemoteFeaturesCompleteEvt(&fevt);
        DLI_DataLenChangeEvt devt = { 0 };
        devt.connHandle = provider.ConsumeIntegral<uint16_t>();
        devt.maxTxOctets = provider.ConsumeIntegral<uint16_t>();
        devt.maxRxOctets = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleCompleteEvt(DLI_SET_DATA_LEN, DLI_CBK_SET_DATA_LEN, devt);
    }

    void CM_ConnectSetParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectSetParamReq_S param = {0};
        param.bitFrameType = provider.ConsumeIntegral<uint8_t>();
        param.enableFilterPolicy = provider.ConsumeIntegral<uint8_t>();
        param.initiatingPhys = provider.ConsumeIntegral<uint8_t>();
        param.gtNegotiate = provider.ConsumeIntegral<uint8_t>();
        param.scanInterval = provider.ConsumeIntegral<uint16_t>();
        param.scanWindow = provider.ConsumeIntegral<uint16_t>();
        param.minInterval = provider.ConsumeIntegral<uint16_t>();
        param.maxInterval = provider.ConsumeIntegral<uint16_t>();
        param.timeout = provider.ConsumeIntegral<uint16_t>();
        uint32_t ret = CM_ConnectSetParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
    }

    void CM_ConnectRemoteUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_RemoteConnParamReqEvt revt = { 0 };
        revt.connHandle = provider.ConsumeIntegral<uint16_t>();
        revt.connIntervalMin = provider.ConsumeIntegral<uint16_t>();
        revt.connIntervalMax = provider.ConsumeIntegral<uint16_t>();
        revt.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        revt.eventInterval = provider.ConsumeIntegral<uint16_t>();
        revt.maxLatency = provider.ConsumeIntegral<uint16_t>();
        revt.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        revt.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        revt.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleConnectRemoteParamUpdateReqEvt(&revt);
    }

    typedef struct {
        uint8_t status;
        uint16_t connHandle;         /*!< 连接句柄 */
    } DLI_RemoteConnParamReqReplyReturnParam;

    void CM_ConnectRemoteConnectionParamRequestsReplyFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DLI_RemoteConnParamReqReplyReturnParam revt = { 0 };
        revt.status = provider.ConsumeIntegral<uint8_t>();
        revt.connHandle = provider.ConsumeIntegral<uint16_t>();
        CmFuzzerSleCompleteEvt(DLI_CONNECTION_PARAM_REQ_REPLY, DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, revt);
    }

    void CM_ConnectUpdateParamReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ConnectUpdateParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.intervalMin = provider.ConsumeIntegral<uint16_t>();
        param.intervalMax = provider.ConsumeIntegral<uint16_t>();
        param.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        param.eventInterval = provider.ConsumeIntegral<uint16_t>();
        param.maxLatency = provider.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        param.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        param.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_ConnectUpdateParamReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectSetParamReq failed, ret:%d", ret);
            return;
        }
        DLI_ConnectionUpdateCmpEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.connInterval = provider.ConsumeIntegral<uint16_t>();
        evt.txRxInterval = provider.ConsumeIntegral<uint8_t>();
        evt.eventInterval = provider.ConsumeIntegral<uint16_t>();
        evt.maxLatency = provider.ConsumeIntegral<uint16_t>();
        evt.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        evt.systemTimeUnit = provider.ConsumeIntegral<uint8_t>();
        evt.txRxFlag = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleConnectParamUpdateEvt(&evt);
    }

    void CM_ConnectReleaseReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_DisconnectParamReq_S param = {};
        param.addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        SLE_Addr_S addr = {0};
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        param.discReason = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_ConnectReleaseReq(&param);
        if (ret != CM_SUCCESS) {
            CM_LOGE("CM_ConnectReleaseReq failed, ret:%d", ret);
            return;
        }
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
        uint16_t reason = provider.ConsumeIntegral<uint16_t>();
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
        FuzzedDataProvider provider(data, size);
        CM_ReadAcceptFilterListSize();
        DLI_ReadBufSizeEvt evt = { 0 };
        evt.acbTxDataLen = provider.ConsumeIntegral<uint16_t>();
        evt.acbTxDataNum = provider.ConsumeIntegral<uint8_t>();
        evt.icbTxDataLen = provider.ConsumeIntegral<uint16_t>();
        evt.icbTxDataNum = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSleCompleteEvt(DLI_READ_ACCESS_FILTER_LIST_SIZE, DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, evt);
    }

    void CM_AddDeviceToAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_AddDeviceToAcceptFilterList(&addr, false);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_AddDeviceToAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST, DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, evt);
    }

    void CM_RemoveDeviceFromAcceptFilterListFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_RemoveDeviceFromAcceptFilterList(&addr);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_RemoveDeviceFromAcceptFilterList failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST, DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, evt);
    }

    void CM_SetPhyFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetPhyReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.txFormat = provider.ConsumeIntegral<uint8_t>();
        param.rxFormat = provider.ConsumeIntegral<uint8_t>();
        param.txPhy = provider.ConsumeIntegral<uint8_t>();
        param.rxPhy = provider.ConsumeIntegral<uint8_t>();
        param.txPilotDensity = provider.ConsumeIntegral<uint8_t>();
        param.rxPilotDensity = provider.ConsumeIntegral<uint8_t>();
        param.gFeedback = provider.ConsumeIntegral<uint8_t>();
        param.tFeedback = provider.ConsumeIntegral<uint8_t>();
        uint32_t ret = CM_SetPhy(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetPhy failed, ret:%d", ret);
        DLI_SetPhyEvt evt = { 0 };
        evt.status = provider.ConsumeIntegral<uint8_t>();
        evt.connHandle = provider.ConsumeIntegral<uint16_t>();
        evt.txFormat = provider.ConsumeIntegral<uint8_t>();
        evt.rxFormat = provider.ConsumeIntegral<uint8_t>();
        evt.txPhy = provider.ConsumeIntegral<uint8_t>();
        evt.rxPhy = provider.ConsumeIntegral<uint8_t>();
        evt.txPilotDensity = provider.ConsumeIntegral<uint8_t>();
        evt.rxPilotDensity = provider.ConsumeIntegral<uint8_t>();
        evt.gFeedback = provider.ConsumeIntegral<uint8_t>();
        evt.tFeedback = provider.ConsumeIntegral<uint8_t>();
        CmFuzzerSetPhyEvt(&evt);
    }

    void CM_SetHostChannelClassificationFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetChannelMapReq_S param = {0};
        for (uint32_t i = 0; i < (sizeof(param.channelMap) / sizeof(param.channelMap[0])); i++) {
            param.channelMap[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_SetHostChannelClassification(&param);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_SetHostChannelClassification failed, ret:%d", ret);
        std::nullptr_t evt = nullptr;
        CmFuzzerSleCompleteEvt(DLI_SET_HOST_CHANNEL_CLASSIFICATION, DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, evt);
    }

    void CM_GetLogicLinkByAddrFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = { 0 };
        CM_LogicLink_S logicLink = { 0 };
        addr.type = provider.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        uint32_t ret = CM_GetLogicLinkByAddr(&addr, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByAddr failed, ret:%d", ret);
    }

    void CM_GetLogicLinkByLcidFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = 0;
        CM_LogicLink_S logicLink = { 0 };
        lcid = provider.ConsumeIntegral<uint16_t>();
        uint32_t ret = CM_GetLogicLinkByLcid(lcid, &logicLink);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "CM_GetLogicLinkByLcid failed, ret:%d", ret);
    }

    void CM_SetLogicLinkDeviceTypeFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = 0;
        uint8_t deviceType = 0;
        lcid = provider.ConsumeIntegral<uint16_t>();
        deviceType = provider.ConsumeIntegral<uint8_t>();
        CM_SetLogicLinkDeviceType(lcid, deviceType);
    }

    void CM_SetACBSubrateFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.subrate = provider.ConsumeIntegral<uint16_t>();
        CM_InnerSetACBSubrate(&param);
    }

    static uint32_t FuzzEnableConnHighPowerImpl(DLI_EnableConnHighPowerParam *param)
    {
        (void)param;
        return CM_SUCCESS;
    }

    static uint32_t FuzzSetACBSubrateImpl(DLI_ACBSubrateParam *dliParam)
    {
        (void)dliParam;
        return CM_SUCCESS;
    }

    static void FuzzDynTransEstablishRspCbkImpl(const CM_DynTransChanEstablishParamRsp_S *param)
    {
        (void)param;
    }

    static void FuzzDynTransReleaseRspCbkImpl(const CM_DynTransChanReleaseParamRsp_S *param)
    {
        (void)param;
    }

    static void FuzzDynTransStatusIndicationCbkImpl(const CM_DynTransChanStatusIndicationRsp_S *param)
    {
        (void)param;
    }

    static bool FuzzDynTransEstablishedCheckCbkImpl(const CM_DynTransChanEstablishedCheckParam_S *param)
    {
        (void)param;
        return true;
    }

    void CM_EnableRealConnHighPowerFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_EnableConnHighPowerReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        param.enable = provider.ConsumeBool();
        param.powerLevel = provider.ConsumeIntegral<uint8_t>();
        (void)CM_EnableRealConnHighPower(&param, FuzzEnableConnHighPowerImpl);
    }

    void CM_SetRealACBSubrateFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_SetACBSubrateParam param = {};
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.subrate = provider.ConsumeIntegral<uint16_t>();
        param.onlySubrate = provider.ConsumeBool();
        param.subrateMax = provider.ConsumeIntegral<uint16_t>();
        param.maxLatency = provider.ConsumeIntegral<uint16_t>();
        param.continuationNum = provider.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
        (void)CM_SetRealACBSubrate(&param, FuzzSetACBSubrateImpl);
    }

    void CM_GetRssiFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_ReadRemoteRssiReq_S param = {0};
        param.lcid = provider.ConsumeIntegral<uint16_t>();
        (void)CM_GetRssi(&param);
    }

    void CM_GetLcidByConnHandleFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t connHandle = provider.ConsumeIntegral<uint16_t>();
        (void)CM_GetLcidByConnHandle(connHandle);
    }

    void CM_RegDynTransChannelCbksFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        (void)provider;
        CM_DynTransChannelCbks_S cbks = {0};
        cbks.establishRspCbk = FuzzDynTransEstablishRspCbkImpl;
        cbks.releaseRspCbk = FuzzDynTransReleaseRspCbkImpl;
        cbks.statusIndicationCbk = FuzzDynTransStatusIndicationCbkImpl;
        cbks.establishedCheckCbk = FuzzDynTransEstablishedCheckCbkImpl;
        (void)CM_RegDynTransChannelCbks(&cbks);
    }

    void CM_DynTransChannelEstablishReqFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_DynTransChannelEstablishParamReq_S param = {0};
        param.version = provider.ConsumeIntegral<uint8_t>();
        param.localIndex = provider.ConsumeIntegral<uint16_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            param.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
        }
        param.srcPort = provider.ConsumeIntegral<uint16_t>();
        param.dstPort = provider.ConsumeIntegral<uint16_t>();
        param.slqi = provider.ConsumeIntegral<uint8_t>();
        param.extension = nullptr;
        param.expectedTransportMode = provider.ConsumeIntegral<uint8_t>();
        param.exclusive = provider.ConsumeBool();
        param.frameType = static_cast<CM_TransConnFrameType_E>(
            provider.ConsumeIntegral<uint8_t>() % CM_RADIO_FRAME_TYPE_END);
        (void)CM_DynTransChannelEstablishReq(&param);
    }

    void CM_SetRemoteFeatureFuzz(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        NLSTK_RemoteFeatureInfo_S info = {0};
        info.lcid = provider.ConsumeIntegral<uint16_t>();
        for (int i = 0; i < NLSTK_REMOTE_FEATURE_LEN; i++) {
            info.features[i] = provider.ConsumeIntegral<uint8_t>();
        }
        (void)CM_SetRemoteFeature(&info);
    }

    void FuzzCmLinkSupplementApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        CM_EnableRealConnHighPowerFuzz(data, size);
        CM_SetRealACBSubrateFuzz(data, size);
        CM_GetRssiFuzz(data, size);
        CM_GetLcidByConnHandleFuzz(data, size);
        CM_RegDynTransChannelCbksFuzz(data, size);
        CM_DynTransChannelEstablishReqFuzz(data, size);
        CM_SetRemoteFeatureFuzz(data, size);
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
    OHOS::FuzzCmLinkSupplementApi(static_cast<uint8_t *>(data), size);
}

void FuzzCmLinkDeInit(void)
{
    CM_DeInit();
}