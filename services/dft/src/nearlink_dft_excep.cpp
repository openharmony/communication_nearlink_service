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

#include "nearlink_dft_excep.h"
#include <string>
#include "log.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_dft_manager.h"
#include "nearlink_dft_utils.h"
#include "nearlink_utils.h"
#include "nearlink_dft_device_data.h"

namespace OHOS {
namespace Nearlink {

NearlinkDftExcep::NearlinkDftExcep()
{}

NearlinkDftExcep& NearlinkDftExcep::GetInstance()
{
    static NearlinkDftExcep instance;
    return instance;
}

void NearlinkDftExcep::WriteIcceProfileExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode,
    const int subErrorCode)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    params.emplace_back(CreateStrParamC(ICCE_PROFILE_EXCEP_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(ICCE_PROFILE_EXCEP_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(ICCE_PROFILE_EXCEP_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(ICCE_PROFILE_EXCEP_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(ICCE_PROFILE_EXCEP_SUB_ERROR_CODE, subErrorCode));
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftExcep::WriteCommonExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode)
{
    std::vector<DftParamC> params;
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_ONE_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_ONE_SUB_ERROR_CODE, subErrorCode));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_ONE_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftExcep::WriteCommonExcep(DftEventEnum eventId, const int errorCode, const int subErrorCode,
    std::string callingName)
{
    std::vector<DftParamC> params;
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_TWO_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_TWO_SUB_ERROR_CODE, subErrorCode));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_TWO_CALLING_NAME, callingName));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_TWO_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftExcep::WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int accessType,
    const int errorCode, const int subErrorCode)
{
    std::vector<DftParamC> refKeys;
    refKeys.emplace_back(CreateStrParamC(PEER_INFO_ADDR, device.GetAddress()));
    refKeys.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, accessType));
    DftSubEventRefC peerInfoRef = {DFT_PEER_INFO, refKeys.data(), refKeys.size()};
    std::vector<DftParamC> params;
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_THREE_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_THREE_SUB_ERROR_CODE, subErrorCode));
    params.emplace_back(CreateRefParamC(EXCEP_TEMPLATE_THREE_PEER_INFO, peerInfoRef));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_THREE_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftExcep::WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode,
    const int subErrorCode)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FOUR_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FOUR_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FOUR_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FOUR_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FOUR_SUB_ERROR_CODE, subErrorCode));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FOUR_TIME, time));
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftExcep::WriteCommonExcep(DftEventEnum eventId, const RawAddress &device, const int errorCode,
    const int subErrorCode, std::string callingName)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FIVE_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FIVE_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FIVE_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FIVE_ERROR_CODE, errorCode));
    params.emplace_back(CreateI32ParamC(EXCEP_TEMPLATE_FIVE_SUB_ERROR_CODE, subErrorCode));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FIVE_CALLING_NAME, callingName));
    params.emplace_back(CreateStrParamC(EXCEP_TEMPLATE_FIVE_TIME, time));
    DftManagerReport(eventId, params.data(), params.size());
}

}  // namespace Nearlink
}  // namespace OHOS