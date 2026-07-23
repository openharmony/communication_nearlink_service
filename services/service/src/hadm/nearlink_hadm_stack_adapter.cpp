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
#include "ssap_utils.h"
#include "hadm_defines.h"
#include "nearlink_dft_exception.h"
#include "nearlink_permission_manager.h"
#include "nearlink_hadm_stack_adapter.h"

namespace OHOS {
namespace Nearlink {
static NearlinkHadmStackAdapter *g_hadmClientStackAdapter = nullptr;

NearlinkHadmStackAdapter::NearlinkHadmStackAdapter(HadmStackAdapterCallback &callback)
    : callback_(callback)
{
    HILOGI("HadmClientService construct");
    g_hadmClientStackAdapter = this;
    HadmSoundingCbk_S cbs = {};
    cbs.reportIqDataCbk = &onReportSoundingIQResult;
    cbs.controlResultCbk = &onSetSoundingEnableDisable;
    cbs.soundingStateReportCbk = &onSoundingMeasureStateChange;
    /* 协议栈只保存一份cbk */
    NLSTK_Errcode_E ret = HadmRegCbk(&cbs);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("fail ! ret=%{public}d", ret);
    }
}

NearlinkHadmStackAdapter::~NearlinkHadmStackAdapter() = default;

void NearlinkHadmStackAdapter::onSetSoundingEnableDisable(SLE_Addr_S *addr, HadmUserOperate_E ctrlType,
    NLSTK_Errcode_E errorCode)
{
    HILOGI("Receive sounding enable or disable event. status=0x%{public}d", static_cast<int>(ctrlType));
    NL_CHECK_RETURN(addr != nullptr, "addr is nullptr");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    g_hadmClientStackAdapter->callback_.OnSoundingStateChange(device, static_cast<int>(ctrlType),
        static_cast<int>(errorCode));
}

void NearlinkHadmStackAdapter::onSoundingMeasureStateChange(HadmSoundingStateInfo_S *state)
{
    NL_CHECK_RETURN(state, "hadm service measure state is null.");
    g_hadmClientStackAdapter->callback_.onSoundingMeasureStateChange(state->status,
        state->posMeasureSigConfigIdx, state->measureState);
}

void NearlinkHadmStackAdapter::onReportSoundingIQResult(SLE_Addr_S *addr, HadmSoundingIqData_S *args)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(addr, "addr is nullptr.");
    NL_CHECK_RETURN(args, "data is nullptr.");

    NearlinkHadmSoundingResult result;
    std::vector<uint16_t> dutIData(args->iqChnlNum);
    std::vector<uint16_t> dutQData(args->iqChnlNum);
    std::vector<uint16_t> rtdIData(args->iqChnlNum);
    std::vector<uint16_t> rtdQData(args->iqChnlNum);

    result.dutRssi_ = args->dutRssi;
    result.rtdRssi_ = args->rtdRssi;
    result.dutTof_ = args->dutTof;
    result.rtdTof_ = args->rtdTof;
    result.timeStampSn_ = args->timeStampSn;
    result.isMultiTone_ = args->isMultiTone;
    result.dutIqBitLen_ = args->dutIqBitLen;
    result.rtdIqBitLen_ = args->rtdIqBitLen;
    (void)memcpy_s(result.dutSlemChmap_, HADM_CHMAP_BYTE_LEN, args->dutSlemChmap, HADM_CHMAP_BYTE_LEN);
    (void)memcpy_s(result.rtdSlemChmap_, HADM_CHMAP_BYTE_LEN, args->rtdSlemChmap, HADM_CHMAP_BYTE_LEN);
    result.localNvOffset_ = args->localNvOffset;
    result.remoteNvOffset_ = args->remoteNvOffset;
    result.localTofOffset_ = args->localTofOffset;
    result.remoteTofOffset_ = args->remoteTofOffset;
    result.addr_ = RawAddress::ConvertToString(addr->addr);
    for (uint32_t i = 0; i < args->iqChnlNum; i++) {
        dutIData[i] = (args->iqData + i)->dutIData;
        dutQData[i] = (args->iqData + i)->dutQData;
        rtdIData[i] = (args->iqData + i)->rtdIData;
        rtdQData[i] = (args->iqData + i)->rtdQData;
    }
    result.AppendDutIData(dutIData);
    result.AppendDutQData(dutQData);
    result.AppendRtdIData(rtdIData);
    result.AppendRtdQData(rtdQData);
    g_hadmClientStackAdapter->callback_.OnSoundingResult(result.addr_, result);
}

void NearlinkHadmStackAdapter::SetHadmConnectionParam(HadmConnectionParam_S *connectionParamIn) const
{
    connectionParamIn->version = 0;
    connectionParamIn->localIndex = 0;
    connectionParamIn->intervalMin = HADM_CONN_COEXIST_INTERVAL;
    connectionParamIn->intervalMax = HADM_CONN_COEXIST_INTERVAL;
    connectionParamIn->eventIfs = HADM_CONN_EVENT_IFS;
    connectionParamIn->eventEfs = HADM_CONN_EVENT_IFS;
    connectionParamIn->maxLatency = 0;
    connectionParamIn->supervisionTimeout = HADM_CONN_SUPERVISION_TIMEOUT;
    connectionParamIn->systemTimeUint = HADM_CONN_TIME_UNIT;
    connectionParamIn->txRxFlag = 0;
}

void NearlinkHadmStackAdapter::SetSoundingParam(HadmSoundingParam_S *paramIn) const
{
    uint8_t pm2400mBand[HADM_MEASURE_PM_24G_BAND_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F};
    paramIn->configId = 0x00;
    paramIn->schedulingTimeslot = 0x00;
    paramIn->rttPhy = 0x00;
    paramIn->freqHoppingMode = 0x00;
    paramIn->fmFreq = HADM_CONFIG_FREQ;
    paramIn->fmInteractionType = HADM_CONFIG_INTERACTION_TYPE;
    paramIn->occurrenceGroupPeriod = 0x0000;
    paramIn->fmOccurrenceGroupInterval = HADM_CONFIG_GROUP_INTERVAL;
    paramIn->pmMeasureType1Interval = HADM_CONFIG_MEASURE_INTERVAL_TYPE1;
    paramIn->pmMeasureType2Interval = HADM_CONFIG_MEASURE_INTERVAL_TYPE2;
    paramIn->fmTIp1Time = HADM_CONFIG_INTRA_EVENT_INTERVAL;
    paramIn->pmTIp2Time = HADM_CONFIG_INTRA_EVENT_INTERVAL;
    paramIn->fmTGuard = HADM_CONFIG_T_GUARD;
    paramIn->fmSignal2Length = HADM_CONFIG_SIGNAL2_LEN;
    paramIn->pmInitAntCount = 0x00;
    paramIn->pmInitSignal2Tone = 0x00;
    paramIn->pmReflAntCount = 0x00;
    paramIn->pmReflSignal2Tone = 0x00;
    paramIn->pmFreqHoppingBand = HADM_CONFIG_FREQ_HOP_BAND;
    (void)memcpy_s(paramIn->pm2400mBand, HADM_MEASURE_PM_24G_BAND_LEN, pm2400mBand, HADM_MEASURE_PM_24G_BAND_LEN);
    paramIn->glpMode = 0x00;
    paramIn->sleHadmMode = 0x00;
    paramIn->isCsParamChg = 0x00;
    paramIn->freqSpace = 0x00;
    paramIn->conAnchorNum = HADM_CONFIG_ANCHOR_NUM;
    paramIn->refreshRate = 0x00;
    paramIn->acbInterval = 0x00;
    paramIn->csInterval = 0x00;
}

int NearlinkHadmStackAdapter::StartSounding(const RawAddress &addr, std::string callingName) const
{
    HILOGI("Address:%{public}s", GetEncryptAddr(addr.GetAddress()).c_str());

    HadmConnectionParam_S updateParam = {};
    SetHadmConnectionParam(&updateParam);

    HadmSoundingParam_S paramIn = {};
    SetSoundingParam(&paramIn);

    SLE_Addr_S stackAddr = ConvertToSleAddr(addr);
    NLSTK_Errcode_E ret = HadmStartSounding(&stackAddr, &updateParam, &paramIn);
    if (ret == NLSTK_ERRCODE_SUCCESS) {
        DftCacheAccurateSearchMeasureInfo(callingName, addr.GetAddress(), DFT_MEASURE,
            DFT_MEASURE_SUCCESS);
        return HADM_SUCCESS;
    }
    return HADM_FAILURE;
}

int NearlinkHadmStackAdapter::StopSounding(const RawAddress &addr, std::string callingName) const
{
    HILOGI("Address:%{public}s", GetEncryptAddr(addr.GetAddress()).c_str());

    SLE_Addr_S stackAddr = ConvertToSleAddr(addr);
    NLSTK_Errcode_E ret = HadmStopSounding(&stackAddr);
    if (ret == NLSTK_ERRCODE_SUCCESS) {
        DftDealAccurateSearchMeasureInfo(callingName, addr.GetAddress());
        return HADM_SUCCESS;
    }
    return HADM_FAILURE;
}

uint8_t NearlinkHadmStackAdapter::GetSoundingState(const RawAddress &addr) const
{
    // 默认返回测距停止状态
    uint8_t soundingState = static_cast<uint8_t>(HADM_SOUNDING_STOP);
    SLE_Addr_S stackAddr = ConvertToSleAddr(addr);
    NLSTK_Errcode_E ret = HadmGetSoundingState(&stackAddr, &soundingState);
    HILOGI("Get sound state %{public}d, Address:%{public}s", soundingState, GetEncryptAddr(addr.GetAddress()).c_str());
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, HADM_SOUNDING_STOP, "fail ! ret=%{public}d", ret);
    return soundingState;
}

uint32_t NearlinkHadmStackAdapter::GetSoundingAddrInfo(RawAddress &addr)
{
    SLE_Addr_S stackAddr[MAX_RANGING_DEVICE] = {};
    uint32_t ret = HadmGetSoundingAddrInfo(&stackAddr[0], MAX_RANGING_DEVICE);
    addr = RawAddress::ConvertToString(stackAddr[0].addr);
    return ret;
}

}  // namespace Nearlink
}  // namespace OHOS