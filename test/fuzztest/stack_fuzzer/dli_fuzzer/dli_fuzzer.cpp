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
#include "dli_ext_func_wrapper.h"
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
/* FUZZ-009 维护：最近直调命令的 opcode，供 FUZZ-011 CMD_STATUS/ERROR 结构化回执联动。 */
static uint16_t g_fuzzLastCmdOpcode = 0;

/* DLI 投递回调（recvAcbHandler）：消费并释放 buff（真实场景由 DTAP_DataRecv 消费）。 */
void FuzzDliRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    SDF_BuffFree(buf);
}

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
        if ((event[idx] == DLI_CMD_STATUS_EVT || event[idx] == DLI_CMD_ERROR_EVT) &&
            size >= sizeof(DLI_CommandStatus)) {
            /* FUZZ-011: 结构化构造——opcode 取自刚发送命令（g_fuzzLastCmdOpcode，
             * FuzzDliSyncCmdApi 维护），status 成功/失败两档。 */
            DLI_CommandStatus evt = {0};
            evt.cmdOpcode = g_fuzzLastCmdOpcode;
            evt.numDliCommandPackets = provider.ConsumeIntegral<uint8_t>();
            evt.status = provider.ConsumeBool() ? 0 : 1;
            RecvEventHandler(event[idx], nullptr, (uint8_t *)&evt, sizeof(evt));
            continue;
        }
        if (event[idx] == DLI_NUMBER_OF_COMPLETED_PACKETS_EVT &&
            size >= sizeof(DLI_NumberOfCompletedPacketsEvt)) {
            /* FUZZ-011: NOCP 结构化——经 DLI_RunCmdCbk 分发注册回调（配额消费方
             * DTAP 未在本进程 Init，配额路径在 dtap_fuzzer 的 FuzzFireNocp 生效）。 */
            DLI_NumberOfCompletedPacketsEvt evt = {0};
            evt.connHandle = provider.ConsumeIntegral<uint16_t>();
            evt.numCompletedPackets = provider.ConsumeIntegral<uint8_t>();
            RecvEventHandler(event[idx], nullptr, (uint8_t *)&evt, sizeof(evt));
            continue;
        }
        if (size >= sizeof(DLI_ControllerData)) {
            RecvEventHandler(event[idx], nullptr, data, size);
        }
    }
    DLI_LOGI("FuzzRecvEventProc exit");
}

/* FUZZ-009: 同步链路命令族 11 API 直调——此前全仓零真实调用（仅 adv/sm 工程 stub）。
 * 计数字段 icbCnt/paramCnt/channelCnt 按 0/1/2/SLE_ICB_MAX_NUM 四档驱动双重循环；
 * 每 API 配 (NULL,cbk)/(param,NULL) 变体；opcode 记入 g_fuzzLastCmdOpcode 供
 * FUZZ-011 的 CMD_STATUS/ERROR 结构化回执联动。 */
void FuzzDliSyncCmdApi(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    static const uint8_t cntTable[] = {0, 1, 2, SLE_ICB_MAX_NUM};
    uint8_t cnt = cntTable[provider.ConsumeIntegral<uint8_t>() % sizeof(cntTable)];

    DLI_ICBParam icbParams[SLE_ICB_MAX_NUM] = {{0}};
    DLI_ICBParam::DLI_ICB icbInners[SLE_ICB_MAX_NUM] = {{0}}; /* DLI_ICBParam.param 指向的内层结构 */
    DLI_ICBChannel channels[SLE_ICB_MAX_NUM] = {{0}};
    for (uint8_t i = 0; i < SLE_ICB_MAX_NUM; i++) {
        icbParams[i].id = provider.ConsumeIntegral<uint8_t>();
        icbParams[i].param = &icbInners[i]; /* 产品侧 dli_cmd.c:1130 直接解引用，必须真实构造 */
        icbInners[i].maxSduG2T = provider.ConsumeIntegral<uint16_t>();
        icbInners[i].maxSduT2G = provider.ConsumeIntegral<uint16_t>();
        icbInners[i].rtnG2T = provider.ConsumeIntegral<uint8_t>();
        icbInners[i].rtnT2G = provider.ConsumeIntegral<uint8_t>();
        channels[i].connHandle = provider.ConsumeIntegral<uint16_t>();
        channels[i].lcid = provider.ConsumeIntegral<uint16_t>();
    }
    DLI_ICGCbkParam cbkParam = {0};
    cbkParam.type = provider.ConsumeIntegral<uint8_t>();
    cbkParam.id = provider.ConsumeIntegral<uint8_t>();
    cbkParam.direction = provider.ConsumeIntegral<uint8_t>();
    cbkParam.connHandleNum = cnt;
    for (uint8_t i = 0; i < cnt && i < SLE_ICB_MAX_NUM; i++) {
        cbkParam.connHandle[i] = channels[i].connHandle;
    }

    DLI_ICGParam icgParam = {0};
    icgParam.type = provider.ConsumeIntegral<uint8_t>();
    icgParam.opCode = static_cast<DLI_CmdOpcode>(provider.ConsumeIntegral<uint16_t>());
    icgParam.id = provider.ConsumeIntegral<uint8_t>();
    icgParam.sduIntervalG2T = provider.ConsumeIntegral<uint32_t>();
    icgParam.sduIntervalT2G = provider.ConsumeIntegral<uint32_t>();
    icgParam.maxLatencyG2T = provider.ConsumeIntegral<uint16_t>();
    icgParam.maxLatencyT2G = provider.ConsumeIntegral<uint16_t>();
    icgParam.icbCnt = cnt;
    icgParam.paramCnt = cnt;
    icgParam.icbParam = icbParams;
    g_fuzzLastCmdOpcode = icgParam.opCode;
    (void)DLI_SetICGParam(&icgParam, &cbkParam);
    (void)DLI_SetICGParam(nullptr, &cbkParam);
    (void)DLI_SetICGParam(&icgParam, nullptr);

    DLI_ICGAutorateParam autorate = {0};
    autorate.type = provider.ConsumeIntegral<uint8_t>();
    autorate.opCode = static_cast<DLI_CmdOpcode>(provider.ConsumeIntegral<uint16_t>());
    autorate.id = provider.ConsumeIntegral<uint8_t>();
    autorate.labelId = provider.ConsumeIntegral<uint8_t>();
    autorate.icbInterval = provider.ConsumeIntegral<uint16_t>();
    autorate.paramCnt = cnt;
    DLI_ICBAutorateParam autorateParams[SLE_ICB_MAX_NUM] = {{0}};
    for (uint8_t i = 0; i < SLE_ICB_MAX_NUM; i++) {
        autorateParams[i].maxSduG2T = provider.ConsumeIntegral<uint16_t>();
        autorateParams[i].frameG2T = provider.ConsumeIntegral<uint8_t>();
        autorateParams[i].phyG2T = provider.ConsumeIntegral<uint8_t>();
    }
    autorate.icbParam = autorateParams;
    g_fuzzLastCmdOpcode = autorate.opCode;
    (void)DLI_SetICGAutorateParam(&autorate, &cbkParam);
    (void)DLI_SetICGAutorateParam(nullptr, &cbkParam);

    DLI_ICBConnectionParam connParam = {0};
    connParam.type = provider.ConsumeIntegral<uint8_t>();
    connParam.opCode = static_cast<DLI_CmdOpcode>(provider.ConsumeIntegral<uint16_t>());
    connParam.id = provider.ConsumeIntegral<uint8_t>();
    connParam.labelId = provider.ConsumeIntegral<uint8_t>();
    connParam.channelCnt = cnt;
    connParam.channel = channels;
    g_fuzzLastCmdOpcode = connParam.opCode;
    (void)DLI_CreateICB(&connParam, &cbkParam);
    (void)DLI_CreateICB(nullptr, &cbkParam);

    DLI_DisconnectParam discParam = {0};
    discParam.connHandle = channels[0].connHandle;
    discParam.reason = provider.ConsumeIntegral<uint8_t>();
    g_fuzzLastCmdOpcode = DLI_DISCONNECT;
    (void)DLI_DisconnectICB(&discParam, &cbkParam);
    (void)DLI_DisconnectICB(nullptr, &cbkParam);

    DLI_AcceptICBReqParam acceptParam = {0};
    acceptParam.type = provider.ConsumeIntegral<uint8_t>();
    acceptParam.opCode = static_cast<DLI_CmdOpcode>(provider.ConsumeIntegral<uint16_t>());
    acceptParam.connHandle = channels[0].connHandle;
    g_fuzzLastCmdOpcode = acceptParam.opCode;
    (void)DLI_AcceptICBReq(&acceptParam);
    (void)DLI_AcceptICBReq(nullptr);

    DLI_RejectICBReqParam rejectParam = {};
    rejectParam.opCode = static_cast<DLI_CmdOpcode>(provider.ConsumeIntegral<uint16_t>());
    rejectParam.regOpCode = provider.ConsumeIntegral<uint8_t>();
    rejectParam.id = provider.ConsumeIntegral<uint8_t>();
    rejectParam.connHandle = channels[0].connHandle;
    rejectParam.reason = provider.ConsumeIntegral<uint8_t>();
    g_fuzzLastCmdOpcode = rejectParam.opCode;
    (void)DLI_RejectICBReq(&rejectParam);
    (void)DLI_RejectICBReq(nullptr);

    uint8_t codecConfig[8] = {0};
    for (uint8_t i = 0; i < sizeof(codecConfig); i++) {
        codecConfig[i] = provider.ConsumeIntegral<uint8_t>();
    }
    DLI_SetupICBDataPathParam pathParam = {0};
    pathParam.connHandle = channels[0].connHandle;
    pathParam.direction = provider.ConsumeIntegral<uint8_t>();
    pathParam.pathId = provider.ConsumeIntegral<uint8_t>();
    pathParam.codec.codecId = provider.ConsumeIntegral<uint8_t>();
    pathParam.codec.vendorId = provider.ConsumeIntegral<uint16_t>();
    pathParam.controllerDelay = provider.ConsumeIntegral<uint32_t>();
    pathParam.codecConfigLen = sizeof(codecConfig);
    pathParam.codecConfigData = codecConfig;
    g_fuzzLastCmdOpcode = DLI_SETUP_ICB_DATA_PATH;
    (void)DLI_SetupICBDataPath(&pathParam, &cbkParam);
    (void)DLI_SetupICBDataPath(nullptr, &cbkParam);

    DLI_RemoveICBDataPathParam delPathParam = {0};
    delPathParam.connHandle = channels[0].connHandle;
    delPathParam.direction = provider.ConsumeIntegral<uint8_t>();
    g_fuzzLastCmdOpcode = DLI_REMOVE_ICB_DATA_PATH;
    (void)DLI_RemoveICBDataPath(&delPathParam, &cbkParam);
    (void)DLI_RemoveICBDataPath(&delPathParam, nullptr);

    uint16_t removeOp = provider.ConsumeIntegral<uint16_t>();
    g_fuzzLastCmdOpcode = removeOp;
    (void)DLI_RemoveICGParam(removeOp, &cbkParam);
    (void)DLI_RemoveICGParam(removeOp, nullptr);

    DLI_CmdParams cmdParams = {0};
    cmdParams.cmd = provider.ConsumeIntegral<uint16_t>();
    cmdParams.event = provider.ConsumeIntegral<uint16_t>();
    cmdParams.inParam = &icgParam;
    cmdParams.paramLen = (uint16_t)sizeof(icgParam);
    cmdParams.cbk = MockDliCbk;
    cmdParams.cbkContext = nullptr;
    cmdParams.cbkContextLen = 0;
    g_fuzzLastCmdOpcode = cmdParams.cmd;
    (void)DLI_SetCmd(&cmdParams);
    (void)DLI_SetCmd(nullptr);

    DLI_IMGEncryptParam imgParam = {0};
    imgParam.handler = provider.ConsumeIntegral<uint16_t>();
    imgParam.algo = provider.ConsumeIntegral<uint8_t>();
    for (uint8_t i = 0; i < SLE_IMG_ENCRYPTION_IV_LEN; i++) {
        imgParam.iv[i] = provider.ConsumeIntegral<uint8_t>();
    }
    for (uint8_t i = 0; i < SLE_IMG_ENCRYPTION_GROUP_KEY_LEN; i++) {
        imgParam.key[i] = provider.ConsumeIntegral<uint8_t>();
    }
    g_fuzzLastCmdOpcode = DLI_ENABLE_IMG_ENCRYPTION;
    (void)DLI_EnableIMGEncryption(&imgParam);
    (void)DLI_EnableIMGEncryption(nullptr);
}

/* FUZZ-010: adapter 层 11 个包装函数直调（计划口径 8 已按代码修正为 11）。
 * 判空行（funcList 未注册时 WARN+return 0）无条件命中，覆盖即得。 */
void FuzzDliExtWrapperApi(uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    DLI_AddrExtStru addrExt = {0};
    for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
        addrExt.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    addrExt.ext = provider.ConsumeIntegral<uint8_t>();
    (void)DLI_AddDeviceToAcceptFilterListExt(&addrExt);
    (void)DLI_AddDeviceToAcceptFilterListExt(nullptr);

    DLI_FreqBandExtParam freqBand = {0};
    freqBand.trigType = provider.ConsumeIntegral<uint8_t>();
    (void)DLI_EnableFreqBandExt(&freqBand);
    (void)DLI_EnableFreqBandExt(nullptr);

    DLI_ACBEnableSubrateParam acbEnable = {0};
    acbEnable.subrateMin = provider.ConsumeIntegral<uint16_t>();
    acbEnable.subrateMax = provider.ConsumeIntegral<uint16_t>();
    acbEnable.maxLatency = provider.ConsumeIntegral<uint16_t>();
    acbEnable.continuationNum = provider.ConsumeIntegral<uint16_t>();
    acbEnable.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
    (void)DLI_ACBEnableSubrate(&acbEnable);
    (void)DLI_ACBEnableSubrate(nullptr);

    DLI_LocalPrivateFeatures features = {0};
    features.bitValue = provider.ConsumeIntegral<uint8_t>();
    (void)DLI_SetLocalPrivateFeatures(&features);
    (void)DLI_SetLocalPrivateFeatures(nullptr);

    DLI_ACBSubrateParam acbSubrate = {0};
    acbSubrate.lcid = provider.ConsumeIntegral<uint16_t>();
    acbSubrate.subrateMin = provider.ConsumeIntegral<uint16_t>();
    acbSubrate.subrateMax = provider.ConsumeIntegral<uint16_t>();
    acbSubrate.maxLatency = provider.ConsumeIntegral<uint16_t>();
    acbSubrate.continuationNum = provider.ConsumeIntegral<uint16_t>();
    acbSubrate.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
    (void)DLI_ACBSetSubrate(&acbSubrate);
    (void)DLI_ACBSetSubrate(nullptr);

    DLI_SetConnFramePowerLevelParam powerLevel = {0};
    powerLevel.frameType = provider.ConsumeIntegral<uint8_t>();
    powerLevel.txPower = provider.ConsumeIntegral<int8_t>();
    (void)DLI_SetConnFramePowerLevel(&powerLevel);
    (void)DLI_SetConnFramePowerLevel(nullptr);

    (void)DLI_ReadLocalPrivateFeatures();

    DLI_ICGAutorateParam autorateExt = {0};
    autorateExt.type = provider.ConsumeIntegral<uint8_t>();
    autorateExt.paramCnt = 0;
    bool mcast = provider.ConsumeBool();
    DLI_ICGCbkParam cbkExt = {0};
    (void)DLI_SetICGAutorateParamExt(&autorateExt, mcast, &cbkExt);
    (void)DLI_SetICGAutorateParamExt(nullptr, mcast, nullptr);

    DLI_ICGLabelParam labelExt = {0};
    labelExt.type = provider.ConsumeIntegral<uint8_t>();
    labelExt.icbCnt = 0;
    (void)DLI_SetICGLabelExt(&labelExt, mcast, &cbkExt);
    (void)DLI_SetICGLabelExt(nullptr, mcast, nullptr);

    DLI_ICBConnectionParam connExt = {0};
    connExt.type = provider.ConsumeIntegral<uint8_t>();
    connExt.channelCnt = 0;
    (void)DLI_CreateICBExt(&connExt, mcast, &cbkExt);
    (void)DLI_CreateICBExt(nullptr, mcast, nullptr);

    DLI_ICGUpdatedParam updatedExt = {0};
    updatedExt.type = provider.ConsumeIntegral<uint8_t>();
    updatedExt.icbCnt = 0;
    (void)DLI_UpdateICGParamExt(&updatedExt, mcast, &cbkExt);
    (void)DLI_UpdateICGParamExt(nullptr, mcast, nullptr);
}

/* FUZZ-011: 等时/加密/频段事件族——头部结构体对齐的半结构化构造（照 AdvReport
 * 模板：头部 fuzz 字节拷入 + 尾部载荷），事件码追加喂入 RecvEventHandler。 */
typedef struct {
    uint16_t evt;
    size_t hdrSize;
} DLI_StructEvtStru;

static const DLI_StructEvtStru g_structEvtTable[] = {
    {DLI_IOB_CONNECT_REQ_EVT, sizeof(DLI_ICBConnectReqEvt)},
    {DLI_IOB_ESTABLISHED_EVT, sizeof(DLI_ICBEstablishedEvt)},
    {DLI_IOB_PARAM_REPORT_EVT, sizeof(DLI_IOBQualityReportEvt)},
    {DLI_IOG_LABEL_REPORT_EVT, sizeof(DLI_ICGLabelReportEvt)},
    {DLI_IOG_UPDATE_PARAM_EVT, sizeof(DLI_ICBParamUpdateEvt)},
    {DLI_IMB_CONNECT_REQ_EVT, sizeof(DLI_ICBConnectReqEvt)},
    {DLI_IMB_ESTABLISHED_EVT, sizeof(DLI_ICBEstablishedEvt)},
    {DLI_IMB_PARAM_REPORT_EVT, sizeof(DLI_IOBQualityReportEvt)},
    {DLI_IMG_LABEL_REPORT_EVT, sizeof(DLI_ICGLabelReportEvt)},
    {DLI_IMG_UPDATE_PARAM_EVT, sizeof(DLI_ICBParamUpdateEvt)},
    {DLI_ENCRYPTION_CHANGE_EVT, sizeof(DLI_EncryptChangeEvt)},
    {DLI_FREQ_BAND_SWITCH_VENDOR_EVENT, sizeof(DLI_FreqBandSwitchEvt)},
};

void FuzzRecvStructEvtProc(uint8_t *data, size_t size)
{
    for (size_t idx = 0; idx < sizeof(g_structEvtTable) / sizeof(g_structEvtTable[0]); idx++) {
        if (size < g_structEvtTable[idx].hdrSize) {
            continue;
        }
        DLI_LOGI("FuzzRecvStructEvtProc evt=0x%02X", g_structEvtTable[idx].evt);
        RecvEventHandler(g_structEvtTable[idx].evt, nullptr, data, size);
    }
}

/* DEVD 报告解析确定性序列：同地址 ADV_IND + SCAN_RSP 两帧驱动缓存合并；
 * 帧载荷携带铅笔广播签名（g_pencilAdvertstingData1/2），触发旧解析识别分支；
 * 末尾补一帧 dataLength=0 的长度边界帧。地址取自 fuzz 输入保持可变异。 */
void FuzzAdvReportSameAddrFrames(uint8_t *data, size_t size)
{
    static const uint8_t pencilSig1[] = {0x03, 0x19, 0xC7, 0x03, 0x0F, 0x16, 0xEE, 0xFD};
    static const uint8_t pencilSig2[] = {0x12, 0x09, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49,
        0x20, 0x4D, 0x2D, 0x50, 0x65, 0x6E, 0x63, 0x69, 0x6C, 0x20, 0x33};
    if (size < SLE_ADDR_LEN) {
        return;
    }
    const struct {
        uint8_t eventType;
        const uint8_t *payload;
        uint16_t payloadLen;
    } frames[] = {
        {0x03, pencilSig1, sizeof(pencilSig1)},   /* connectable+scannable，完整广播帧 */
        {0x08, pencilSig2, sizeof(pencilSig2)},   /* scan response，完整响应帧 */
        {0x03, nullptr, 0},                        /* 长度边界：空载荷帧 */
    };
    for (size_t f = 0; f < sizeof(frames) / sizeof(frames[0]); f++) {
        size_t evtSize = sizeof(DLI_AdvReportEvt) + frames[f].payloadLen;
        DLI_AdvReportEvt *evt = (DLI_AdvReportEvt *)SDF_MemZalloc(evtSize);
        if (evt == nullptr) {
            continue;
        }
        evt->eventType = frames[f].eventType;
        evt->addrType = data[0] % 2;
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            evt->addr[i] = data[i]; /* 同地址：三帧一致，驱动缓存合并 */
        }
        evt->primFrameType = 0;
        evt->secondPhy = 0;
        evt->rssi = static_cast<uint8_t>(0xC0); /* -64dBm 合法值 */
        evt->dataLength = static_cast<uint8_t>(frames[f].payloadLen);
        if (frames[f].payloadLen != 0) {
            (void)memcpy_s(evt->data, frames[f].payloadLen, frames[f].payload, frames[f].payloadLen);
        }
        RecvEventHandler(DLI_ADVERTISING_REPORT_EVT, nullptr, (uint8_t *)evt, evtSize);
        SDF_MemFree(evt);
    }
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
    if (size >= sizeof(DLI_AddrStru)) {
        DLI_SetPublicAddress((DLI_AddrStru *)data);
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
        advData->advDataLen = (uint8_t)remainData.size();
        (void)memcpy_s(advData->advData, remainData.size(), remainData.data(), remainData.size());
        DLI_SetAdvData(advData);
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
        scanRspData->scanRspDataLen = (uint8_t)remainData.size();
        (void)memcpy_s(scanRspData->scanRspData, remainData.size(), remainData.data(), remainData.size());
        DLI_SetScanRspData(scanRspData);
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
    /* recvAcbHandler 必须非空：DLI_SetCallback 三必选字段任一 NULL 即拒绝（dli_layer_
     * callback.c:58-63），DLI_Init/事件注册表（g_commInnerCbkList/g_cmInnerCbkList）
     * 随之失效，RecvEventHandler 分发空转。no-op 消费并释放投递 buff。 */
    dliCallback.recvAcbHandler = OHOS::FuzzDliRecvAcbHandler;
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
    OHOS::FuzzAdvReportSameAddrFrames(static_cast<uint8_t *>(fuzzData), inputLen);
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
    OHOS::FuzzDliSyncCmdApi(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzDliExtWrapperApi(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzRecvEventProc(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzRecvStructEvtProc(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzEventApiTest(static_cast<uint8_t *>(fuzzData), inputLen);
    OHOS::FuzzDliLastApi();
    OHOS::g_dliThreadUtil.DestroyQueue();

    SDF_MemFree(fuzzData);
    return 0;
}