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

#include "nearlink_datashare_helper.h"

#include <charconv>
#include "log.h"
#include "datashare_predicates.h"
#include "datashare_errno.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "os_account_manager.h"
#include "parameters.h"
#include "SleHiviewUe.h"
#include "SleNameChangeManager.h"
#include "SleCollaborationManager.h"
#if (defined(DEVICE_MANAGER))
#include "device_manager.h"
#endif
namespace OHOS {
namespace Nearlink {

namespace {
const std::string SETTINGSDATA_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
const std::string SETTINGSDATA_EXTENSION_URI = "datashare:///com.ohos.settingsdata.DataAbility";
const std::string USER_SETTINGSDATA_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/USER_SETTINGSDATA_SECURE_";
const std::string DATA_COLUMN_KEYWORD = "KEYWORD";
const std::string DATA_COLUMN_VALUE = "VALUE";
const std::string NEARLINK_SWITCH_KEYWORD = "nearlink_switch_enable";
const std::string DEVICE_NAME_KEYWORD = "settings.general.display_device_name";
const std::string AIRPLANE_MODE_KEYWORD = "settings.telephony.airplanemode";
const std::string COLLABORATION_SWITCH_KEYWORD =
    "settings.collaboration.multi_device_collaboration_service_switch";
const std::string HIVIEW_UE_SWITCH_KEYWORD = "hiview_ue_switch_enable";
const int32_t SWITCH_STATE = 0;
const bool AIRPLANE_MODE_DEFAULT = false;
const bool COLLABORATION_SWITCH_DEFAULT = true;
const bool HIVIEW_UE_SWITCH_DEFAULT = true;
const std::string NEARLINK_SERVICE_NAME = "nearlink_service";
constexpr int32_t MAX_NAME_LENGTH = 30;
constexpr int32_t DM_OK = 0;
constexpr int32_t UTF8_INVALID_BYTE_LENGTH = 0;
constexpr int32_t UTF8_SINGLE_BYTE_LENGTH = 1;
constexpr int32_t UTF8_DOUBLE_BYTE_LENGTH = 2;
constexpr int32_t UTF8_TRIPLE_BYTE_LENGTH = 3;
constexpr int32_t UTF8_QUADRUPLE_BYTE_LENGTH = 4;
} // namespace
std::shared_ptr<DataShare::DataShareHelper> NearlinkDataShareHelper::CreateDataShareHelper()
{
    HILOGD("enter");
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(saManager != nullptr, nullptr, "GetSystemAbilityManager failed.");

    sptr<IRemoteObject> remoteObj = saManager->GetSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);
    NL_CHECK_RETURN_RET(remoteObj != nullptr, nullptr, "GetSystemAbility failed.");

    return DataShare::DataShareHelper::Creator(remoteObj, SETTINGSDATA_URI);
}

std::shared_ptr<DataShare::DataShareHelper> NearlinkDataShareHelper::CreateDataShareHelperInstance() const
{
    HILOGI("enter");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(samgr != nullptr, nullptr, "Get samgr failed.");

    sptr<IRemoteObject> remoteObject = samgr->GetSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);
    NL_CHECK_RETURN_RET(remoteObject != nullptr, nullptr, "nearlink SA is not ready.");

    sptr<IRemoteObject> dataSharedServer = samgr->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    NL_CHECK_RETURN_RET(dataSharedServer != nullptr, nullptr, "DataShare server is not ready!");

    std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> res = DataShare::DataShareHelper::Create(remoteObject,
        SETTINGSDATA_URI, SETTINGSDATA_EXTENSION_URI);
    if (res.first == DataShare::E_DATA_SHARE_NOT_READY) {
        HILOGE("DataShareHelper::Create failed: E_DATA_SHARE_NOT_READY");
        return nullptr;
    }
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = res.second;
    NL_CHECK_RETURN_RET(res.first == DataShare::E_OK && dataShareHelper != nullptr, nullptr, "fail:%{public}d",
        res.first);
    return dataShareHelper;
}

bool NearlinkDataShareHelper::GetSettingDeviceName(std::string &localName)
{
    int32_t osAccountId = GetActiveOsAccountIds();
    if (osAccountId == -1) {
        return false;
    }
    std::string osaccountIdStr = std::to_string(osAccountId);
    Uri uri(USER_SETTINGSDATA_URI + osaccountIdStr + "?Proxy=true");
    bool ret = GetValue(uri, DEVICE_NAME_KEYWORD, localName);
    if (!ret) {
        std::string deviceName = system::GetParameter("const.product.name", "");
        if (!deviceName.empty()) {
            localName = deviceName;
            HILOGI("Get deviceName success");
            return true;
        }
        HILOGE("GetLocalName failed, return default value");
        return false;
    }
    return true;
}

int NearlinkDataShareHelper::GetUTF8StringLength(const char firstByte)
{
    // 根据首字节获取UTF8字符占用的字节数量
    int length = UTF8_SINGLE_BYTE_LENGTH;
    if ((firstByte & 0x80) == 0) {
        length = UTF8_SINGLE_BYTE_LENGTH;
    } else if ((firstByte & 0xE0) == 0xC0) {
        length = UTF8_DOUBLE_BYTE_LENGTH;
    } else if ((firstByte & 0xF0) == 0xE0) {
        length = UTF8_TRIPLE_BYTE_LENGTH;
    } else if ((firstByte & 0xF8) == 0xF0) {
        length = UTF8_QUADRUPLE_BYTE_LENGTH;
    }
    return length;
}

int NearlinkDataShareHelper::GetValidUTF8StringLength(const std::string &localName)
{
    int byteCount = 0;
    size_t stringSize = localName.size();
    for (size_t i = 0; i < stringSize;) {
        int utf8Length = GetUTF8StringLength(localName[i]);
        if (byteCount + utf8Length > MAX_NAME_LENGTH) {
            break;
        }
        byteCount += utf8Length;
        if (byteCount == MAX_NAME_LENGTH) {
            return byteCount;
        }
        i += static_cast<size_t>(utf8Length);
    }
    return byteCount;
}

void NearlinkDataShareHelper::GetTruncationName(std::string &localName)
{
    unsigned int length = localName.length();
    if (length <= MAX_NAME_LENGTH) {
        return;
    }
    // 字符串长度超过30字节后，截取完整的且小于30个字节的UTF-8的字符串,截断后不满30个字节时用'.'补齐30字节
    std::string ellipsis = ".";
    int limitLength = GetValidUTF8StringLength(localName);
    HILOGI("limitLength = %{public}d", limitLength);
    std::string newDeviceName = localName.substr(0, limitLength);
    for (int i = (MAX_NAME_LENGTH - limitLength); i > 0; i--) {
        newDeviceName += ellipsis;
    }
    localName = newDeviceName;
}

std::string NearlinkDataShareHelper::GetLocalDeviceName()
{
    std::string localName = "";
#if (defined(DEVICE_MANAGER))
    int32_t ret = DistributedHardware::DeviceManager::GetInstance().GetLocalDisplayDeviceName(
        NEARLINK_SERVICE_NAME, MAX_NAME_LENGTH, localName);
    NL_CHECK_RETURN_RET(ret == DM_OK, "", "GetLocalDisplayDeviceName failed");
#else
    NearlinkDataShareHelper::GetInstance().GetSettingDeviceName(localName);
    NearlinkDataShareHelper::GetInstance().GetTruncationName(localName);
#endif
    return localName;
}

bool NearlinkDataShareHelper::RegisterNameChangeObserver() const
{
    HILOGI("enter");
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = CreateDataShareHelperInstance();
    NL_CHECK_RETURN_RET(dataShareHelper != nullptr, false, "dataShareHelper is NULL");
    int32_t osAccountId = GetActiveOsAccountIds();
    std::string osaccountIdStr = "100";  // 默认
    if (osAccountId == -1) {
        HILOGE("Invalid osAccountId");
    } else {
        osaccountIdStr = std::to_string(osAccountId);
    }
    auto uri = std::make_shared<Uri>(USER_SETTINGSDATA_URI + osaccountIdStr +
        "?Proxy=true" + "&key=" + DEVICE_NAME_KEYWORD);
    sptr<AAFwk::DataAbilityObserverStub> settingDataObserver = sptr<SleNameChangeObserver>::MakeSptr();
    dataShareHelper->RegisterObserver(*uri, settingDataObserver);
    dataShareHelper->Release();
    return true;
}

bool NearlinkDataShareHelper::RegisterCollaborationChangeObserver() const
{
    HILOGI("enter");
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = CreateDataShareHelperInstance();
    NL_CHECK_RETURN_RET(dataShareHelper != nullptr, false, "dataShareHelper is NULL");
    auto uri = std::make_shared<Uri>(SETTINGSDATA_URI + "&key=" + COLLABORATION_SWITCH_KEYWORD);
    sptr<AAFwk::DataAbilityObserverStub> settingDataObserver = sptr<SleCollaborationChangeObserver>::MakeSptr();
    dataShareHelper->RegisterObserver(*uri, settingDataObserver);
    dataShareHelper->Release();
    return true;
}

bool NearlinkDataShareHelper::RegisterHiviewUeChangeObserver() const
{
    HILOGI("enter");
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = CreateDataShareHelperInstance();
    NL_CHECK_RETURN_RET(dataShareHelper != nullptr, false, "dataShareHelper is NULL");
    auto uri = std::make_shared<Uri>(SETTINGSDATA_URI + "&key=" + HIVIEW_UE_SWITCH_KEYWORD);
    sptr<AAFwk::DataAbilityObserverStub> settingDataObserver = sptr<SleHiviewUeChangeObserver>::MakeSptr();
    dataShareHelper->RegisterObserver(*uri, settingDataObserver);
    dataShareHelper->Release();
    return true;
}

int32_t NearlinkDataShareHelper::GetActiveOsAccountIds() const
{
    std::vector<int32_t> accountId;
    AccountSA::OsAccountManager::QueryActiveOsAccountIds(accountId);
    if (accountId.empty()) {
        HILOGE("Get accountId failed");
        return -1;
    }
    return accountId[0];
}

int32_t NearlinkDataShareHelper::GetSwitchState()
{
    Uri uri(SETTINGSDATA_URI);
    std::string value;
    bool ret = GetValue(uri, NEARLINK_SWITCH_KEYWORD, value);
    NL_CHECK_RETURN_RET(ret, SWITCH_STATE, "GetSwitchState failed, return default value");
    int32_t result = SWITCH_STATE;
    auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (!(ec == std::errc{} && ptr == value.data() + value.size())) {
        HILOGE("GetSwitchState invalid value %{public}s", value.c_str());
        return SWITCH_STATE;
    }
    return result;
}

bool NearlinkDataShareHelper::GetAirplaneModeState()
{
    Uri uri(SETTINGSDATA_URI + "&key=airplane_mode");
    std::string value;
    bool ret = GetValue(uri, AIRPLANE_MODE_KEYWORD, value);
    NL_CHECK_RETURN_RET(ret, AIRPLANE_MODE_DEFAULT, "GetAirplaneModeState failed, return default value");
    HILOGI("Airplane mode value is: %{public}s", value.c_str());
    return value == "1"; // "1" means airplane mode is on
}

bool NearlinkDataShareHelper::GetCollaborationState()
{
    Uri uri(SETTINGSDATA_URI);
    std::string value;
    bool ret = GetValue(uri, COLLABORATION_SWITCH_KEYWORD, value);
    NL_CHECK_RETURN_RET(ret, COLLABORATION_SWITCH_DEFAULT, "GetCollaborationState failed, return default value");
    HILOGI("Collaboration state value is: %{public}s", value.c_str());
    return value == "1"; // "1" means collaboration switch is on
}

bool NearlinkDataShareHelper::GetHiviewUeState()
{
    Uri uri(SETTINGSDATA_URI + "&key=" + HIVIEW_UE_SWITCH_KEYWORD);
    std::string value;
    bool ret = GetValue(uri, HIVIEW_UE_SWITCH_KEYWORD, value);
    NL_CHECK_RETURN_RET(ret, HIVIEW_UE_SWITCH_DEFAULT, "GetHiviewUeState failed, return default value");
    HILOGI("HiviewUe state value is: %{public}s", value.c_str());
    return value == "1"; // "1" means hiview_ue switch is on
}

bool NearlinkDataShareHelper::GetValue(Uri &uri, const std::string &key, std::string &value)
{
    HILOGD("enter");
    auto dataShareHelper = CreateDataShareHelper();
    NL_CHECK_RETURN_RET(dataShareHelper != nullptr, false, "CreateDataShareHelper failed");

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(DATA_COLUMN_KEYWORD, key);
    std::vector<std::string> columns;
    auto rows = dataShareHelper->Query(uri, predicates, columns);
    if (rows == nullptr) {
        HILOGE("Query failed");
        dataShareHelper->Release();
        return false;
    }

    rows->GoToFirstRow();
    int32_t columnIndex;
    rows->GetColumnIndex(DATA_COLUMN_VALUE, columnIndex);
    int32_t ret = rows->GetString(columnIndex, value);
    if (ret != DataShare::E_OK) {
        HILOGE("GetInt failed with ret=%{public}d", ret);
        rows->Close();
        dataShareHelper->Release();
        return false;
    }

    rows->Close();
    dataShareHelper->Release();
    return true;
}

#ifdef WATCH_STANDARD
void NearlinkDataShareHelper::SaveNearlinkSwitchStatus(const std::string &sleState)
{
    HILOGD("enter");
    std::string strValue = "";
    Uri strUri(USER_SETTINGSDATA_URI + "100?Proxy=true&key=");
    const std::string watchSystemStrType = "watch_system_str_type";
    if (GetValue(strUri, watchSystemStrType, strValue)) {
        if (strValue == "1") {
            HILOGI("strMode not write settingdb");
            return;
        }
    }
    auto dataShareHelper = CreateDataShareHelper();
    NL_CHECK_RETURN(dataShareHelper != nullptr, "CreateDataShareHelper failed");
    const std::string nearlinkSwitchStatus = "nearlink_switch_status";
    DataShare::DataShareValueObject key(nearlinkSwitchStatus);
    DataShare::DataShareValueObject value(sleState);
    DataShare::DataShareValuesBucket bucket;
    bucket.Put(DATA_COLUMN_KEYWORD, key);
    bucket.Put(DATA_COLUMN_VALUE, value);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(DATA_COLUMN_KEYWORD, nearlinkSwitchStatus);
    Uri uri(SETTINGSDATA_URI + "&key=" + nearlinkSwitchStatus);
    int32_t ret = dataShareHelper->Update(uri, predicates, bucket);
    if (ret <= 0) { // ret > 0 operation succ
        HILOGI("switch status data not exist:%{public}d", ret);
        ret = dataShareHelper->Insert(uri, bucket);
        if (ret <= 0) {
            HILOGE("switch status Insert fail:%{public}d", ret);
            dataShareHelper->Release();
            return;
        }
    }
    dataShareHelper->NotifyChange(uri);
    dataShareHelper->Release();
    return;
}

void NearlinkDataShareHelper::SaveNearlinkCarkeyName(const std::string &sleCarkeyName)
{
    HILOGI("sle carkey name is: %{public}s", sleCarkeyName.c_str());
    auto dataShareHelper = CreateDataShareHelper();
    NL_CHECK_RETURN(dataShareHelper != nullptr, "CreateDataShareHelper failed");
    const std::string sleCarkeyNameTag = "nearlink_carkey_name";
    DataShare::DataShareValueObject key(sleCarkeyNameTag);
    DataShare::DataShareValueObject value(sleCarkeyName);
    DataShare::DataShareValuesBucket bucket;
    bucket.Put(DATA_COLUMN_KEYWORD, key);
    bucket.Put(DATA_COLUMN_VALUE, value);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(DATA_COLUMN_KEYWORD, sleCarkeyNameTag);
    Uri uri(SETTINGSDATA_URI + "&key=" + sleCarkeyNameTag);
    int32_t ret = dataShareHelper->Update(uri, predicates, bucket);
    if (ret <= 0) { // ret > 0 operation succ
        HILOGI("sle carkey name data not exist:%{public}d", ret);
        ret = dataShareHelper->Insert(uri, bucket);
        if (ret <= 0) {
            HILOGE("sle carkey name data Insert fail:%{public}d", ret);
            dataShareHelper->Release();
            return;
        }
    }
    dataShareHelper->NotifyChange(uri);
    dataShareHelper->Release();
    HILOGI("sle carkey name data notify end");
    return;
}
#endif
} // namespace Nearlink
} // namespace OHOS