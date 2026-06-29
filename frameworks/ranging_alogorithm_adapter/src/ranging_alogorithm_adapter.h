/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef HADM_RANGING_ADAPTER_H
#define HADM_RANGING_ADAPTER_H

#include "log.h"
#include "ranging_alogorithm_adapter_def.h"

namespace OHOS {
namespace Nearlink {
typedef errcode_slem (*CalcHadmDis)(DisResult *rangingResult, const MeasureAlgPara *algPara);

typedef void (*InitHadmAlg)(void);

class HadmRangingAdapter {
public:
    static HadmRangingAdapter &GetInstance();

    /**
     * @brief Calculate hadm distance based on hadm algorithm.
     *
     * @param algPara input sounding result.
     * @param rangingResult output ranging result.
     * @return errcode of hadm ranging algorithm
     * @since 6
     */
    int CalculateHadmDistance(NearlinkHadmSoundingResult soundingResult, DisResult &rangingResult);

    /**
     * @brief Init hadm algorithm.
     *
     * @since 6
     */
    bool InitHadmAlgo();

    void CleanUp();

private:
    void TransferSoundingToAlgPara(NearlinkHadmSoundingResult soundingResult, MeasureAlgPara &algPara);
    HadmRangingAdapter();
    ~HadmRangingAdapter();
    bool InitHadm();

private:
    uint32_t totalCount_ = 0;
    void* hadmHandle_ = nullptr;
    CalcHadmDis calcHadmDis_ = nullptr;
    InitHadmAlg initHadmAlg_ = nullptr;
    std::mutex mutex_ {};
};

} // namespace Nearlink
} // namespace OHOS
 
#endif
