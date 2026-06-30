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

#ifndef NEARLINK_DFT_EXCEP_H
#define NEARLINK_DFT_EXCEP_H

#include "raw_address.h"
#include "nearlink_dft_database.h"

namespace OHOS {
namespace Nearlink {

class NearlinkDftExcep {
public:
    static NearlinkDftExcep& GetInstance();
    // used for events similar to g_templateOneExcepParam.
    void WriteCommonExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode);
    // used for events similar to g_templateTwoExcepParam.
    void WriteCommonExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode, std::string callingName);
    // used for events similar to g_templateThreeExcepParam.
    void WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int accessType,
        const int errorCode, const int subErrorCode);
    // used for events similar to g_templateFourExcepParam.
    void WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode, const int subErrorCode);
    // used for events similar to g_templateFiveExcepParam.
    void WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode, const int subErrorCode,
        std::string callingName);
    // used for events of DftIcceProfileExcepParamEnum.
    void WriteIcceProfileExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode,
        const int subErrorCode);
private:
    NearlinkDftExcep();
};

}   // namespace Nearlink
}   // namespace OHOS

#endif // NEARLINK_DFT_EXCEP_H