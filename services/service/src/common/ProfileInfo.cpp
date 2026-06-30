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
#include "ProfileInfo.h"

#include <algorithm>

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {

const std::string SLE_UUID_SSAP = "37BEA880-FC70-11EA-B720-000000000000";
const std::string SLE_UUID_HID_HOST = "37BEA880-FC70-11EA-B720-00000000060B";
const std::string SLE_UUID_ASC = "37BEA880-FC70-11EA-B720-00000000060C";

enum class ProfileId : uint32_t {
    PROFILE_ID_SSAP_CLIENT = 0x00000001,
    PROFILE_ID_SSAP_SERVER = 0x00000002,
    PROFILE_ID_HID_HOST = 0x00004000,
    PROFILE_ID_DIS = 0x00004001,
    PROFILE_ID_LIS = 0x00004003,
    PROFILE_ID_ICCE = 0x00004004,
    PROFILE_ID_PORT = 0x00004005,
    PROFILE_ID_CDSM = 0x00004006,
    PROFILE_ID_MCP = 0x00004007,
    PROFILE_ID_ASC = 0x00004008,
    PROFILE_ID_TWS = 0x00004009,
    PROFILE_ID_VCP = 0x00004010,
    PROFILE_ID_CCP = 0x00004011,
    PROFILE_ID_VAS = 0x00004012,
    PROFILE_ID_BAS = 0x00004013,
    PROFILE_ID_MIC = 0x00004014,
};

const std::vector<ProfileInfo> SupportProfilesInfo::SUPPORT_FILES = {
    ProfileInfo(PROFILE_NAME_SSAP_CLIENT, static_cast<uint32_t>(ProfileId::PROFILE_ID_SSAP_CLIENT), SLE_UUID_SSAP),
    ProfileInfo(PROFILE_NAME_SSAP_SERVER, static_cast<uint32_t>(ProfileId::PROFILE_ID_SSAP_SERVER), SLE_UUID_SSAP),
    ProfileInfo(PROFILE_NAME_HID_HOST, static_cast<uint32_t>(ProfileId::PROFILE_ID_HID_HOST), SLE_UUID_HID_HOST),
    ProfileInfo(PROFILE_NAME_DIS, static_cast<uint32_t>(ProfileId::PROFILE_ID_DIS), SLE_UUID_DIS),
    ProfileInfo(PROFILE_NAME_LIS, static_cast<uint32_t>(ProfileId::PROFILE_ID_LIS), SLE_UUID_LIS),
    ProfileInfo(PROFILE_NAME_ICCE, static_cast<uint32_t>(ProfileId::PROFILE_ID_ICCE), SLE_UUID_ICCE),
    ProfileInfo(PROFILE_NAME_PORT, static_cast<uint32_t>(ProfileId::PROFILE_ID_PORT), SLE_UUID_PORT_PROFILE),
    ProfileInfo(PROFILE_NAME_CDSM, static_cast<uint32_t>(ProfileId::PROFILE_ID_CDSM), SLE_UUID_CDSM_PROFILE),
    ProfileInfo(PROFILE_NAME_MCP_SERVER, static_cast<uint32_t>(ProfileId::PROFILE_ID_MCP), SLE_UUID_MCP_PROFILE),
    ProfileInfo(PROFILE_NAME_ASC, static_cast<uint32_t>(ProfileId::PROFILE_ID_ASC), SLE_UUID_ASC),
    ProfileInfo(PROFILE_NAME_TWS, static_cast<uint32_t>(ProfileId::PROFILE_ID_TWS), SLE_UUID_TWS_PROFILE),
    ProfileInfo(PROFILE_NAME_VCP, static_cast<uint32_t>(ProfileId::PROFILE_ID_VCP), SLE_UUID_VCP_PROFILE),
    ProfileInfo(PROFILE_NAME_CCP, static_cast<uint32_t>(ProfileId::PROFILE_ID_CCP), SLE_UUID_CCP_PROFILE),
    ProfileInfo(PROFILE_NAME_VAS, static_cast<uint32_t>(ProfileId::PROFILE_ID_VAS), SLE_UUID_VAS_PROFILE),
    ProfileInfo(PROFILE_NAME_BAS, static_cast<uint32_t>(ProfileId::PROFILE_ID_BAS), SLE_UUID_BAS_PROFILE),
    ProfileInfo(PROFILE_NAME_MIC, static_cast<uint32_t>(ProfileId::PROFILE_ID_MIC), SLE_UUID_MIC_PROFILE),
};

const std::vector<ProfileInfo> &SupportProfilesInfo::GetSupportProfiles()
{
    return SUPPORT_FILES;
}

std::string SupportProfilesInfo::IdToName(uint32_t id)
{
    auto it = std::find_if(
        SUPPORT_FILES.begin(), SUPPORT_FILES.end(), [id](ProfileInfo pInfo) -> bool { return id == pInfo.mId; });
    if (it != SUPPORT_FILES.end()) {
        return it->mName;
    } else {
        return "";
    }
}

const std::vector<ProfileInfo> SupportProfilesInfo::GetConfigSupportProfiles(SleTransport transport)
{
    return SUPPORT_FILES;
}
}  // namespace sle
}