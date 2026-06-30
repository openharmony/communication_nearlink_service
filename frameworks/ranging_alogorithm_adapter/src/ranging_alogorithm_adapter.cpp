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

#include <dlfcn.h>
#include <sys/time.h>
#include <ctime>
#include <chrono>
#include "nearlink_errorcode.h"
#include "log.h"
#include "ranging_alogorithm_adapter.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {
namespace {
const char* const NEARLINK_HADM_NAME = "libnearlink_measure.z.so";
const char* const NEARLINK_HADM_RANGING_SYMBOL = "measure_alg_func";
const char* const NEARLINK_HADM_INIT_SYMBOL = "measure_init";
const uint8_t PARA_KEYS_INFO = 0;
const ParaPair PARA_LIMIT_VALUE = { -98, 2, 20.0f };
}


HadmRangingAdapter::HadmRangingAdapter()
{
    HILOGI("enter");
}

HadmRangingAdapter::~HadmRangingAdapter()
{
    HILOGI("enter");
    CleanUp();
}

HadmRangingAdapter &HadmRangingAdapter::GetInstance()
{
    HILOGI("enter");
    static HadmRangingAdapter instance;
    return instance;
}

bool HadmRangingAdapter::InitHadm()
{
    if (hadmHandle_ != nullptr) {
        HILOGI("%{public}s already opened", NEARLINK_HADM_NAME);
        return true;
    }
    hadmHandle_ = dlopen(NEARLINK_HADM_NAME, RTLD_NOW);
    NL_CHECK_RETURN_RET(hadmHandle_, false, "dlopen %{public}s failed, error code: %{public}s", 
        NEARLINK_HADM_NAME, dlerror());
    calcHadmDis_ = reinterpret_cast<CalcHadmDis>(dlsym(hadmHandle_, NEARLINK_HADM_RANGING_SYMBOL));
    NL_CHECK_RETURN_RET(calcHadmDis_, false, "HadmRangingAdapter dlsym %{public}s failed.",
        NEARLINK_HADM_RANGING_SYMBOL);
    initHadmAlg_ = reinterpret_cast<InitHadmAlg>(dlsym(hadmHandle_, NEARLINK_HADM_INIT_SYMBOL));
    NL_CHECK_RETURN_RET(initHadmAlg_, false, "HadmRangingAdapter dlsym %{public}s failed.",
        NEARLINK_HADM_INIT_SYMBOL);
    return true;
}

bool HadmRangingAdapter::InitHadmAlgo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    NL_CHECK_RETURN_RET(InitHadm(), false, "InitHadm failed.");
    NL_CHECK_RETURN_RET(initHadmAlg_, false, "initHadmAlg_ is nullptr.");
    initHadmAlg_();
    return true;
}

// transfer NearlinkHadmSoundingResult to algorithm parameters MeasureAlgPara
void HadmRangingAdapter::TransferSoundingToAlgPara(NearlinkHadmSoundingResult soundingResult,
    MeasureAlgPara &algPara)
{
    std::vector<uint16_t> dutIData = soundingResult.GetDutIData();
    std::vector<uint16_t> rtdIData = soundingResult.GetRtdIData();
    std::vector<uint16_t> dutQData = soundingResult.GetDutQData();
    std::vector<uint16_t> rtdQData = soundingResult.GetRtdQData();
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    algPara.iqChnlNum = static_cast<uint32_t>(dutIData.size());
    algPara.iqDut = new (std::nothrow) algIq[dutIData.size()];
    algPara.iqRtd = new (std::nothrow) algIq[dutIData.size()];
    NL_CHECK_RETURN(algPara.iqDut, "iqDut is nullptr");
    NL_CHECK_RETURN(algPara.iqRtd, "iqRtd is nullptr");
    for (size_t i = 0; i < dutIData.size(); i++) {
        algPara.iqDut[i].iData = dutIData[i];
        algPara.iqDut[i].qData = dutQData[i];
        algPara.iqRtd[i].iData = rtdIData[i];
        algPara.iqRtd[i].qData = rtdQData[i];
    }
    algPara.rssiDut = soundingResult.dutRssi_;
    algPara.rssiRtd = soundingResult.rtdRssi_;
    algPara.paraLimit = PARA_LIMIT_VALUE;
#ifdef WATCH_STANDARD
    algPara.flagInter = METHOD_1M;
#else
    algPara.flagInter = METHOD_ADJ_R_END;
#endif
    algPara.tofRtd = soundingResult.GetRtdTof();
    algPara.tofDut = soundingResult.GetDutTof();
    algPara.keyId = PARA_KEYS_INFO;
    algPara.totalCount = totalCount_;
    algPara.curTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    algPara.isMultiTone = soundingResult.isMultiTone_;
    algPara.dutIqBitLen = soundingResult.dutIqBitLen_;
    algPara.rtdIqBitLen = soundingResult.rtdIqBitLen_;
    (void)memcpy_s(algPara.dutSlemChmap, HADM_CHMAP_BYTE_LEN, soundingResult.dutSlemChmap_, HADM_CHMAP_BYTE_LEN);
    (void)memcpy_s(algPara.rtdSlemChmap, HADM_CHMAP_BYTE_LEN, soundingResult.rtdSlemChmap_, HADM_CHMAP_BYTE_LEN);
    algPara.localNvOffset = soundingResult.localNvOffset_;
    algPara.remoteNvOffset = soundingResult.remoteNvOffset_;
    algPara.localTofOffset = soundingResult.localTofOffset_;
    algPara.remoteTofOffset = soundingResult.remoteTofOffset_;
    HILOGI("Sounding to hadm offset localNv=%{public}u remoteNv=%{public}u localTof=%{public}u remoteTof=%{public}u",
        algPara.localNvOffset, algPara.remoteNvOffset, algPara.localTofOffset, algPara.remoteTofOffset);
    totalCount_++;
}

int HadmRangingAdapter::CalculateHadmDistance(NearlinkHadmSoundingResult soundingResult, DisResult &rangingResult)
{
    std::lock_guard<std::mutex> lock(mutex_);
    NL_CHECK_RETURN_RET(soundingResult.IsCompleteData(), NL_ERR_INVALID_PARAM,
        "soundingResult is not complete or not start");
    NL_CHECK_RETURN_RET(calcHadmDis_, NL_ERR_INTERNAL_ERROR, "calcHadmDis_ is nullptr.");

    MeasureAlgPara algPara;
    (void)memset_s(&algPara, sizeof(MeasureAlgPara), 0, sizeof(MeasureAlgPara));
    TransferSoundingToAlgPara(soundingResult, algPara);

    DisResult result;
    errcode_slem err = calcHadmDis_(&result, &algPara);
    delete[] algPara.iqDut;
    delete[] algPara.iqRtd;
    NL_CHECK_RETURN_RET(err == ERRCODE_SLEM_SUCCESS, NL_RANGING_RESULT_ERR, 
        "hadm ranging failed, error code: 0x%{public}x, distFirst result: %{public}f", err, result.disSmoothed);

    HILOGI("get hadm distFirst result: %{public}f, disOri: %{public}f", result.disSmoothed, result.disOri);
    rangingResult = result;
    return NL_NO_ERROR;
}

void HadmRangingAdapter::CleanUp()
{
    HILOGI("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (hadmHandle_ == nullptr) {
        HILOGE("CleanUp, hadmHandle_ is nullptr.");
        return;
    }
    calcHadmDis_ = nullptr;
    dlclose(hadmHandle_);
    hadmHandle_ = nullptr;
}

}  // namespace Nearlink
}  // namespace OHOS
