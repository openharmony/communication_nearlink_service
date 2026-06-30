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
#ifndef NEARLINK_HADM_STACK_ADAPTER_H
#define NEARLINK_HADM_STACK_ADAPTER_H

#include "nearlink_types.h"
#include "hadm_api.h"
#include "nearlink_hadm_sounding_result.h"

namespace OHOS {
namespace Nearlink {

class HadmStackAdapterCallback {
public:
    virtual ~HadmStackAdapterCallback() = default;

    virtual void OnSoundingResult(const RawAddress &addr, NearlinkHadmSoundingResult &result) {}
    virtual void OnSoundingStateChange(const RawAddress &addr, int ctrlType, int errorCode) {}
    virtual void onSoundingMeasureStateChange(uint8_t status, uint8_t posMeasureSigConfigIdx,
        uint8_t measureState) {}
};


/**
 *  @brief hadm stack adpter implementation class
 */
class NearlinkHadmStackAdapter {
public:
    explicit NearlinkHadmStackAdapter(HadmStackAdapterCallback &callback);
    ~NearlinkHadmStackAdapter();

    int StartSounding(const RawAddress &addr, std::string callingName) const;
    int StopSounding(const RawAddress &addr, std::string callingName) const;
    uint8_t GetSoundingState(const RawAddress &addr) const;
    uint32_t GetSoundingAddrInfo(RawAddress &addr);
private:
    static void onSetSoundingEnableDisable(SLE_Addr_S *addr, HadmUserOperate_E ctrlType,
    NLSTK_Errcode_E errorcode);
    static void onSoundingMeasureStateChange(HadmSoundingStateInfo_S *state);
    static void onReportSoundingIQResult(SLE_Addr_S *addr, HadmSoundingIqData_S *args);
    void SetHadmConnectionParam(HadmConnectionParam_S *connectionParamIn)  const;
    void SetSoundingParam(HadmSoundingParam_S *paramIn)  const;

    HadmStackAdapterCallback &callback_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkHadmStackAdapter);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_HADM_ADAPTER_H