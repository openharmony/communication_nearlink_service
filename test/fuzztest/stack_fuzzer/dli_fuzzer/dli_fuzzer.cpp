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

#include "dli_fuzzer.h"

#include "fuzzer/FuzzedDataProvider.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_event.h"
#include "dli_connect_event.h"
#include "dli_hadm_event.h"
#include "dli_dev_discovery_event.h"
#include "dli_secu_event.h"
#include "dli_nbc_event.h"
#include "dli_reg_ext_func.h"
#include "sdf_mem.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_def.h"
#include "dli_thread.h"
#include "dli_secu_event.h"
#include "fuzztest_utils.h"
#include "SleDliThreadUtil.h"
#include "cp_worker.h"
#include "nlstk_schedule.h"

using namespace OHOS::Nearlink;

#define LEN_EXTEND_MUL 10
#define TEST_DEFAULT_NUM 30
#define DATA_INDEX_0 0
#define DATA_INDEX_1 1
#define DATA_INDEX_2 2
#define DATA_INDEX_3 3
#define DATA_VAL_8 8
#define DATA_VAL_16 16

void MockDliCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cfm_par)
{}

const DLI_CbkLineStru g_smCbkTable[] = {
    {DLI_CBK_SEND_CONTROLLER_DATA, MockDliCbk},
    {DLI_CBK_RECV_CONTROLLER_DATA, MockDliCbk},
    {DLI_CBK_READ_SUPPORT_CRYPTO_ALGO, MockDliCbk},
    {DLI_CBK_ENCRYPT_CHANGE, MockDliCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ, MockDliCbk},
};

const DLI_CbkLineStru g_devdCbkTable[] = {
    {DLI_CBK_SET_ADV_PARAMS, MockDliCbk},
    {DLI_CBK_SET_ADV_DATA, MockDliCbk},
    {DLI_CBK_ENABLE_ADV, MockDliCbk},
    {DLI_CBK_READ_ADV_DATA_LEN, MockDliCbk},
    {DLI_CBK_READ_ADV_SETS_NUM, MockDliCbk},
    {DLI_CBK_REMOVE_ADV, MockDliCbk},
    {DLI_CBK_ADV_TERMINATED, MockDliCbk},
    {DLI_CBK_SET_SCAN_PARAMS, MockDliCbk},
    {DLI_CBK_ENABLE_SCAN, MockDliCbk},
};

const DLI_CbkLineStru g_cmCbkTable[] = {
    {DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, MockDliCbk},
    {DLI_CBK_READ_LOCAL_FEATURE, MockDliCbk},
    {DLI_CBK_CONNECT, MockDliCbk},
    {DLI_CBK_DISCONNECT, MockDliCbk},
    {DLI_CBK_CLEAR_ACCESS_FLT_LIST, MockDliCbk},
    {DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, MockDliCbk},
    {DLI_CBK_CONNECT_CANCEL, MockDliCbk},
    {DLI_CBK_REMOTE_CONNECT_PARAM_REQ, MockDliCbk},
    {DLI_CBK_CONNECT_UPDATE, MockDliCbk},
    {DLI_CBK_READ_REMOTE_FEATURE, MockDliCbk},
    {DLI_CBK_READ_REMOTE_VERSION, MockDliCbk},
    {DLI_CBK_SET_DATA_LEN, MockDliCbk},
    {DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, MockDliCbk},
    {DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, MockDliCbk},
    {DLI_CBK_SET_PHY, MockDliCbk},
};

const DLI_CbkLineStru g_hadmCbkTable[] = {
    {DLI_CBK_READ_LOCAL_MEASURE_CAPS, MockDliCbk},
    {DLI_CBK_READ_REMOTE_MEASURE_CAPS, MockDliCbk},
    {DLI_CBK_SET_MEASURE_CONFIG_PARAM, MockDliCbk},
    {DLI_CBK_SET_MEASURE_EN, MockDliCbk},
    {DLI_CBK_MEASURE_IQ_REPORT, MockDliCbk},
};

typedef struct DLI_CmdEventMappingStru {
    uint16_t cmd;
    uint16_t event;
} DLI_CmdEventMappingStru;

const uint16_t event[] = {
    DLI_ENCRYPTION_PARAMETER_REQUEST_EVT,
    DLI_CONTROLLER_DATA_EVT,
    DLI_DATA_LENGTH_CHANGE_EVT,
    DLI_CONNECTION_COMPLETE_EVT,
    DLI_DISCONNECTION_COMPLETE_EVT,
    DLI_READ_REMOTE_FEATURES_COMPLETE_EVT,
    DLI_READ_REMOTE_VERSION_COMPLETE_EVT,
    DLI_CONNECTION_UPDATE_EVT,
    DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT,
    DLI_ACB_LOW_LATENCY_EN_EVT,
    DLI_READ_REMOTE_RSSI_EVT,
    DLI_SET_PHY_COMPLETE_EVT,
    DLI_SET_ACB_EVT_PARAM_EVT,
    DLI_ADVERTISING_REPORT_EVT,
    DLI_ADVERTISING_TERMINATED_EVT,
    DLI_READ_REMOTE_MEASURE_CAPS_STATUS_VENDOR_EVT,
    DLI_MEASURE_IQ_REPORT_VENDOR_EVT,
    DLI_CMD_ERROR_EVT,
    DLI_CMD_STATUS_EVT,
    DLI_NUMBER_OF_COMPLETED_PACKETS_EVT,
};


typedef struct DLI_EventCallbackInfo{
    size_t size;
    void (*cbk)(void *context, void *arg, uint32_t len, uint16_t evtOpcode);
} DLI_EventCallbackInfo;

static const DLI_EventCallbackInfo EventCbks[] = {
    // dli_connect_event
    {sizeof(DLI_DataLenChangeEvt), DLI_DataLengthChangeCbk},
    {sizeof(DLI_SetPhyEvt), DLI_SetPhyCbk},
    {sizeof(DLI_AcbLowLatencyEnableEvt), DLI_AcbLowLatencyEnableCbk},
    {sizeof(DLI_RemoteConnParamReqEvt), DLI_RemoteConnParamReqCbk},
    {sizeof(DLI_ConnectionCompleteEvt), DLI_ConnectionCbk},
    {sizeof(DLI_DisconnectEvt), DLI_DisconnectionCbk},
    {sizeof(DLI_ReadRemoteFeatsEvt), DLI_ReadRemoteFeaturesCbk},
    {sizeof(DLI_SetAcbEvtParamEvt), DLI_SetAcbEvtParamCbk},
    {sizeof(DLI_ConnectionUpdateCmpEvt), DLI_ConnectionUpdateCbk},
    {sizeof(DLI_ReadRemoteVersionEvt), DLI_ReadRemoteVersionCbk},
    {sizeof(DLI_FreqBandSwitchEvt), DLI_FreqBandSwitchCbk},
    {sizeof(DLI_ICBConnectReqEvt), DLI_IOBConnectReqCbk},
    {sizeof(DLI_ICBEstablishedEvt), DLI_IOBEstablishedCbk},
    {sizeof(DLI_IOBQualityReportEvt), DLI_IOBReportParamCbk},
    {sizeof(DLI_ICGLabelReportEvt), DLI_IOGLabelReportCbk},
    {sizeof(DLI_ICBParamUpdateEvt), DLI_IOGUpdateParamCbk},
    {sizeof(DLI_ICBConnectReqEvt), DLI_IMBConnectReqCbk},
    {sizeof(DLI_ICBEstablishedEvt), DLI_IMBEstablishedCbk},
    {sizeof(DLI_IOBQualityReportEvt), DLI_IMBReportParamCbk},
    {sizeof(DLI_ICGLabelReportEvt), DLI_IMGLabelReportCbk},
    {sizeof(DLI_ICBParamUpdateEvt), DLI_IMGUpdateParamCbk},
    {sizeof(DLI_AcbReqSubrateEvt) + 1, DLI_AcbSubrateCbk},
    // dli_hadm_event
    {sizeof(DLI_CsIqReportEvt), DLI_CsIqReportCbk},
    {sizeof(DLI_ReadRemoteCsCapsEvt), DLI_ReadRemoteCsCapsCbk},
    {sizeof(DLI_MeasureStateChangeEvt), DLI_MeasureStateChangeCbk},
    // dli_dev_discovery_event
    {sizeof(DLI_AdvertisingTerminatedEvt), DLI_AdvTerminatedCbk},
    // dli_secu_event
    {sizeof(DLI_EncryptParamReqEvt), DLI_EncryptParamReqCbk},
    {sizeof(DLI_ControllerDataEvt), DLI_ControllerDataCbk},
    // dli_nbc_event
    {sizeof(DLI_RssiEvt), DLI_ChanInfoCbk},
    {sizeof(DLI_ChipResetNotifyEvt), DLI_ChipResetNotifyCbk}
};

namespace OHOS {
SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

void FuzzRecvEventProc(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    DLI_LOGI("FuzzRecvEventProc enter, size:%zu", size);
    for (uint16_t idx = 0; idx < (sizeof(event) / sizeof(uint16_t)); idx++) {
        DLI_LOGI("FuzzRecvEventProc idx=%u, event=0x%02X", idx, event[idx]);
        if (event[idx] == DLI_CONTROLLER_DATA_EVT && size >= sizeof(DLI_ControllerDataEvt) && size <= UINT8_MAX) {
            DLI_ControllerDataEvt *evt = (DLI_ControllerDataEvt *)SDF_MemZalloc(size);
            if (evt == nullptr) {
                continue;
            }
            evt->connHandle = provider.ConsumeIntegral<uint16_t>();
            evt->ctrlDataIndex = provider.ConsumeIntegral<uint16_t>();
            evt->len = size - sizeof(DLI_ControllerDataEvt);
            memcpy_s(evt->data, evt->len, data + sizeof(DLI_ControllerDataEvt), evt->len);
            RecvEventHandler(event[idx], nullptr, (uint8_t *)evt, size);
            SDF_MemFree(evt);
            continue;
        }
        if ((event[idx] == DLI_ADVERTISING_REPORT_EVT || event[idx] == DLI_ADVERTISING_TERMINATED_EVT) &&
            size >= sizeof(DLI_AdvReportEvt) && size <= UINT8_MAX) {
            DLI_AdvReportEvt *evt = (DLI_AdvReportEvt *)SDF_MemZalloc(size);
            if (evt == nullptr) {
                continue;
            }
            memcpy_s(evt, sizeof(DLI_AdvReportEvt), data, sizeof(DLI_AdvReportEvt));
            evt->dataLength = size - sizeof(DLI_AdvReportEvt);
            memcpy_s(evt->data, evt->dataLength, data + sizeof(DLI_AdvReportEvt), evt->dataLength);
            RecvEventHandler(event[idx], nullptr, (uint8_t *)evt, size);
            SDF_MemFree(evt);
            continue;
        }
        if (size >= sizeof(DLI_ControllerData)) {
            RecvEventHandler(event[idx], nullptr, data, size);
        }
    }
    DLI_LOGI("FuzzRecvEventProc exit");
}

void FuzzReadRemoteMeasureCaps(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_ReadRemoteMeasureCapsParam)) {
        DLI_ReadRemoteMeasureCapsParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_ReadRemoteMeasureCapsParam), data, sizeof(DLI_ReadRemoteMeasureCapsParam));
        DLI_ReadRemoteMeasureCaps(&inputData);
    }
}

void FuzzEnableEncryption(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_EnableEncryptParam)) {
        DLI_EnableEncryptParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_EnableEncryptParam), data, sizeof(DLI_EnableEncryptParam));
        DLI_EnableEncryption(&inputData);
    }
}

void FuzzReadRemoteVersion(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_ConnHandleStru)) {
        DLI_ConnHandleStru inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_ConnHandleStru), data, sizeof(DLI_ConnHandleStru));
        DLI_ReadRemoteVersion(&inputData);
        DLI_ReadRemoteRssi(&inputData);
        DLI_ReadRemoteFeatures(&inputData);
        DLI_EncryptionParamReqNegativeReply(&inputData);
    }
}

void FuzzSetPhy(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_SetPhyParam)) {
        DLI_SetPhyParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_SetPhyParam), data, sizeof(DLI_SetPhyParam));
        DLI_SetPhy(&inputData);
    }
}

void FuzzSetDataLength(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_SetDataLenParam)) {
        DLI_SetDataLenParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_SetDataLenParam), data, sizeof(DLI_SetDataLenParam));
        DLI_SetDataLength(&inputData);
    }
}

void FuzzRemoteConnectionParamReqReply(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_RemConParamReqReplyParam)) {
        DLI_RemConParamReqReplyParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_RemConParamReqReplyParam), data, sizeof(DLI_RemConParamReqReplyParam));
        DLI_RemoteConnectionParamReqReply(&inputData);
    }
}

void FuzzSetMcs(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_SetMcsParam)) {
        DLI_SetMcsParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_SetMcsParam), data, sizeof(DLI_SetMcsParam));
        DLI_SetMcs(&inputData);
    }
}

void FuzzSetControllerData(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_ControllerData) && size <= UINT8_MAX) {
        uint8_t dataLength = size - sizeof(DLI_ControllerData);
        DLI_ControllerData *inputData = (DLI_ControllerData *)SDF_MemZalloc(size);
        if (inputData != nullptr) {
            (void)memcpy_s(inputData, size, data, size);
            inputData->dataLength = dataLength;
            DLI_SetControllerData(inputData);
            SDF_MemFree(inputData);
        }
    }
}

void FuzzEncrypt(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_EncryptParam)) {
        DLI_EncryptParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_EncryptParam), data, sizeof(DLI_EncryptParam));
        DLI_Encrypt(&inputData);
    }
}

void FuzzSetMeasureEnable(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_SetMeasureEnableParam)) {
        DLI_SetMeasureEnableParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_SetMeasureEnableParam), data, sizeof(DLI_SetMeasureEnableParam));
        DLI_SetMeasureEnable(&inputData);
    }
}

void FuzzSetMeasureParam(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_SetMeasureConfigParam)) {
        DLI_SetMeasureConfigParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_SetMeasureConfigParam), data, sizeof(DLI_SetMeasureConfigParam));
        DLI_SetMeasureParam(&inputData);
    }
}

void FuzzEncryptionParamReqReply(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_EncryptReqReplyParam)) {
        DLI_EncryptReqReplyParam inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_EncryptReqReplyParam), data, sizeof(DLI_EncryptReqReplyParam));
        DLI_EncryptionParamReqReply(&inputData);
    }
}

void FuzzRemoveDeviceFromAcceptFilterList(uint8_t *data, size_t size)
{
    if (size >= sizeof(SLE_Addr_S)) {
        SLE_Addr_S inputData = {};
        (void)memcpy_s(&inputData, sizeof(SLE_Addr_S), data, sizeof(SLE_Addr_S));
        DLI_RemoveDeviceFromAcceptFilterList(&inputData);
        DLI_AddDeviceToAcceptFilterList(&inputData);
    }
}

void FuzzEnableScan(uint8_t *data, size_t size)
{
    if (size >= sizeof(DLI_ScanEnable)) {
        DLI_ScanEnable inputData = {};
        (void)memcpy_s(&inputData, sizeof(DLI_ScanEnable), data, sizeof(DLI_ScanEnable));
        DLI_EnableScan(&inputData);
    }
}

void FuzzSetPublicAddress(uint8_t *data, size_t size)
{
    if (size >= SLE_ADDR_LEN) {
        DLI_SetPublicAddress(data);
    }
}

void FuzzCreateConnection(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    size_t paramSize = sizeof(DLI_ConnectionCreateParam);
    std::vector<uint8_t> paramData = provider.ConsumeBytes<uint8_t>(paramSize);
    if (paramData.size() < paramSize) {
        return;
    }
    DLI_ConnectionCreateParam param = {};
    (void)memcpy_s(&param, paramSize,
        paramData.data(), paramSize);

    DLI_CreateConnection(
        provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeIntegral<uint16_t>(),
        &param);
}

void FuzzSetHostChannelClassification(uint8_t *data, size_t size)
{
    DLI_SetHostChannelClassification(data, (uint32_t)size);
}

void FuzzRemoveAdvSet(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    DLI_RemoveAdvSet(provider.ConsumeIntegral<uint8_t>());
}

void FuzzEnableAdv(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::vector<uint8_t> advEnableData = provider.ConsumeBytes<uint8_t>(sizeof(DLI_AdvEnable));
    if (advEnableData.size() < sizeof(DLI_AdvEnable)) {
        return;
    }
    DLI_AdvEnable advEnable = {};
    (void)memcpy_s(&advEnable, sizeof(DLI_AdvEnable), advEnableData.data(), sizeof(DLI_AdvEnable));
    DLI_EnableAdv(provider.ConsumeIntegral<uint8_t>(), &advEnable);
}

void FuzzSetScanParam(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    uint8_t dataLength = sizeof(DLI_ScanParam) + sizeof(DLI_ScanParamCoreNoPhy);
    std::vector<uint8_t> paramData = provider.ConsumeBytes<uint8_t>(sizeof(DLI_ScanParamCoreNoPhy));
    if (paramData.size() < sizeof(DLI_ScanParamCoreNoPhy)) {
        return;
    }

    DLI_ScanParam *scanparam = (DLI_ScanParam *)SDF_MemZalloc(dataLength);
    if (scanparam != nullptr) {
        scanparam->ownAddrType = data[DATA_INDEX_0];
        scanparam->scanFilterPolicy = data[DATA_INDEX_1];
        (void)memcpy_s(
            scanparam->param, sizeof(DLI_ScanParamCoreNoPhy), paramData.data(), sizeof(DLI_ScanParamCoreNoPhy));
        DLI_SetScanParam(scanparam);
        SDF_MemFree(scanparam);
    }
}

void FuzzUpdateConnectionParam(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::vector<uint8_t> paramData = provider.ConsumeBytes<uint8_t>(sizeof(DLI_ConnectionUpdateParam));
    if (paramData.size() < sizeof(DLI_ConnectionUpdateParam)) {
        return;
    }
    DLI_ConnectionUpdateParam param = {};
    (void)memcpy_s(&param, sizeof(DLI_ConnectionUpdateParam), paramData.data(), sizeof(DLI_ConnectionUpdateParam));
    DLI_UpdateConnectionParam(
        provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeIntegral<uint16_t>(),
        &param);
}

void FuzzDisconnect(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::vector<uint8_t> paramData = provider.ConsumeBytes<uint8_t>(sizeof(DLI_DisconnectParam));
    if (paramData.size() < sizeof(DLI_DisconnectParam)) {
        return;
    }
    DLI_DisconnectParam param = {};
    (void)memcpy_s(&param, sizeof(DLI_DisconnectParam), paramData.data(), sizeof(DLI_DisconnectParam));
    DLI_Disconnect(
        provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeIntegral<uint16_t>(),
        &param);

}

void FuzzSetAdvParam(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    DLI_AdvParam param = {0};
    param.advHandle = provider.ConsumeIntegral<uint8_t>();
    param.advMode = provider.ConsumeIntegral<uint8_t>();
    param.advGtRole = provider.ConsumeIntegral<uint8_t>();
    uint32_t val = provider.ConsumeIntegral<uint32_t>();
    param.primAdvIntervalMin[DATA_INDEX_0] = (val >> DATA_VAL_16) & 0xFF;
    param.primAdvIntervalMin[DATA_INDEX_1] = (val >> DATA_VAL_8) & 0xFF;
    param.primAdvIntervalMin[DATA_INDEX_2] = val & 0xFF;
    param.primAdvIntervalMax[DATA_INDEX_0] = (val >> DATA_VAL_16) & 0xFF;
    param.primAdvIntervalMax[DATA_INDEX_1] = (val >> DATA_VAL_8) & 0xFF;
    param.primAdvIntervalMax[DATA_INDEX_2] = val & 0xFF;
    param.primAdvChannelMap = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> addrData = provider.ConsumeBytes<uint8_t>(SLE_ADDR_LEN);
    if (addrData.size() < SLE_ADDR_LEN) {
        return;
    }
    (void)memcpy_s(param.ownAddr, SLE_ADDR_LEN, addrData.data(), SLE_ADDR_LEN);
    (void)memcpy_s(param.peerAddr, SLE_ADDR_LEN, addrData.data(), SLE_ADDR_LEN);
    DLI_ConnParam connParam = {0};
    std::vector<uint8_t> connParamData = provider.ConsumeBytes<uint8_t>(sizeof(DLI_ConnParam));
    if (connParamData.size() < sizeof(DLI_ConnParam)) {
        return;
    }
    (void)memcpy_s(&param.connIntervalMin, sizeof(DLI_ConnParam), connParamData.data(), sizeof(DLI_ConnParam));
    DLI_SetAdvParam(&param);
}

void FuzzSetAdvData(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    uint16_t ops = provider.ConsumeBool() ? DLI_SET_ADVERTISING_DATA : DLI_SET_SCAN_RESPONSE_DATA;
    std::vector<uint8_t> remainData = provider.ConsumeRemainingBytes<uint8_t>();
    if (ops == DLI_SET_ADVERTISING_DATA) {
        uint32_t cmdSize = (uint32_t)sizeof(DLI_AdvData) + remainData.size();
        DLI_AdvData *advData = (DLI_AdvData *)SDF_MemZalloc(cmdSize);
        if (advData == nullptr) {
            return;
        }
        advData->advHandle = provider.ConsumeIntegral<uint8_t>();
        advData->operation = provider.ConsumeIntegral<uint8_t>();
        advData->selection = 0;
        (void)memcpy_s(advData->advData, remainData.size(), remainData.data(), remainData.size());
        DLI_SetAdvData(advData, 0);
        SDF_MemFree(advData);
    } else {
        uint32_t cmdSize = (uint32_t)sizeof(DLI_ScanRspData) + remainData.size();
        DLI_ScanRspData *scanRspData = (DLI_ScanRspData *)SDF_MemZalloc(cmdSize);
        if (scanRspData == nullptr) {
            return;
        }
        scanRspData->advHandle = provider.ConsumeIntegral<uint8_t>();
        scanRspData->operation = provider.ConsumeIntegral<uint8_t>();
        scanRspData->selection = 0;
        (void)memcpy_s(scanRspData->scanRspData, remainData.size(), remainData.data(), remainData.size());
        DLI_SetScanRspData(scanRspData, 0);
        SDF_MemFree(scanRspData);
    }
}

void SendNonParamMethod()
{
    DLI_ReadAcceptFilterListSize();
    DLI_ClearAcceptFilterList();
    DLI_ReadMaximumAdvDataLen();
    DLI_ReadAdvSetsNum();
    DLI_ReadLocalFeatures();
    DLI_ReadLocalVersion();
    DLI_ReadCommConfigValue();
    DLI_ReadLocalMeasureCaps();
    DLI_ReadSupportCryptoAlgo();
    DLI_CancelCreateConnection();
    DLI_ReadBufferSize();
    (void)DLI_GetAcbDataLen();
}

void FuzzEventApiTest(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    size_t numEventCbks = sizeof(EventCbks) / sizeof(EventCbks[0]);
    const DLI_EventCallbackInfo *cbkInfo;
    size_t argSize;
    void *arg = nullptr;
    uint16_t evtOpcode = provider.ConsumeIntegral<uint16_t>();
    std::vector<uint8_t> remainData = provider.ConsumeRemainingBytes<uint8_t>();

    for (size_t cbkIndex = 0; cbkIndex < numEventCbks; cbkIndex++) {
        cbkInfo = &EventCbks[cbkIndex];
        argSize = cbkInfo->size;
        if (remainData.size() < argSize) {
            continue;
        }
        arg = malloc(argSize);
        if (arg == nullptr) {
            continue;
        }
        memcpy_s(arg, argSize, remainData.data(), argSize);
        cbkInfo->cbk(NULL, arg, argSize, evtOpcode);
        free(arg);
        arg = nullptr;
    }
}

void FuzzDliLastApi()
{
#define SL EXT_START
    DLI_CmdCbkUnReg(DEVD, nullptr, 0, nullptr, 0);
    DLI_CmdCbkUnReg(DEVD, nullptr, 0, g_devdCbkTable, sizeof(g_devdCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkUnReg(CM, nullptr, 0, g_cmCbkTable, sizeof(g_cmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkUnReg(HADM, nullptr, 0, g_hadmCbkTable, sizeof(g_hadmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkUnReg(SM, nullptr, 0, g_smCbkTable, sizeof(g_smCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkUnReg((ModuleType)(NBC + SL), nullptr, 0, g_smCbkTable, 1);
    DLI_DeInit();

    DLI_CmdCbkReg(DEVD, nullptr, 0, nullptr, 0);
    DLI_CmdCbkReg(DEVD, nullptr, 0, g_devdCbkTable, sizeof(g_devdCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(CM, nullptr, 0, g_cmCbkTable, sizeof(g_cmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(HADM, nullptr, 0, g_hadmCbkTable, sizeof(g_hadmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(SM, nullptr, 0, g_smCbkTable, sizeof(g_smCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg((ModuleType)(NBC + SL), nullptr, 0, g_smCbkTable, 1);
    DLI_Init();
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    SDF_ThreadInit(TEST_DEFAULT_NUM);
    SDF_EvcInit();
    ScheduleEnable();
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = NULL;
    DLI_SetCallback(&dliCallback);
    DLI_Init();

    DLI_CmdCbkReg(DEVD, nullptr, 0, g_devdCbkTable, sizeof(g_devdCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(CM, nullptr, 0, g_cmCbkTable, sizeof(g_cmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(HADM, nullptr, 0, g_hadmCbkTable, sizeof(g_hadmCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_CmdCbkReg(SM, nullptr, 0, g_smCbkTable, sizeof(g_smCbkTable) / sizeof(DLI_CbkLineStru));
    DLI_LOGI("dli_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::g_dliThreadUtil.InitThread();
    uint16_t inputLen = static_cast<uint16_t>(size * LEN_EXTEND_MUL);
    uint8_t *fuzzData = static_cast<uint8_t *>(SDF_MemAlloc(inputLen));
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    for (uint16_t i = 0; i < LEN_EXTEND_MUL; i++) {
        (void)memcpy_s(fuzzData + size * i, size, data, size);
    }
    OHOS::FuzzReadRemoteMeasureCaps(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEnableEncryption(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzReadRemoteVersion(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetPhy(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzRemoveDeviceFromAcceptFilterList(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEncryptionParamReqReply(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetMeasureParam(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetMeasureEnable(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEncrypt(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetControllerData(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetMcs(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzRemoteConnectionParamReqReply(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetDataLength(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetScanParam(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEnableAdv(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzRemoveAdvSet(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetHostChannelClassification(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzCreateConnection(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetPublicAddress(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEnableScan(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetAdvData(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzSetAdvParam(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzDisconnect(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzUpdateConnectionParam(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::SendNonParamMethod();
    OHOS::FuzzRecvEventProc(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEventApiTest(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzDliLastApi();
    OHOS::g_dliThreadUtil.DestroyQueue();

    SDF_MemFree(fuzzData);
    return 0;
}