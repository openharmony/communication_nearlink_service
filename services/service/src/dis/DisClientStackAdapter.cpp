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
#include "DisClientStackAdapter.h"
#include "DisService.h"
#include "SleServiceFfrtLog.h"
#include "nearlink_dft_device_data.h"
#include "nearlink_dft_exception.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleUtils.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int DIS_MANUFACTURE_SIZE = 15;
constexpr int DIS_PRODUCT_ID_SIZE = 2;
constexpr int DIS_VENDOR_ID_SIZE = 2;
constexpr int DIS_VENSION_ID_SIZE = 2;
constexpr uint8_t DIS_SHIFT_OPRATURN_8 = 8;
constexpr uint8_t DIS_MAX_CONNECTION_NUM = 8;
constexpr uint16_t DIS_INVALID_MANUFACTURER_INFO = 0;
constexpr uint16_t DIS_INVALID_PRODUCT_ID = 0;
constexpr uint16_t DIS_INVALID_VENDOR_ID = 0;
constexpr uint16_t DIS_INVALID_VERSION_INFO = 0;
SLE_Addr_S ConvertToStackAddr(const RawAddress &addr)
{
    SLE_Addr_S sleAddr;
    (void)memset_s(&sleAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    addr.ConvertToUint8(sleAddr.addr);
    sleAddr.type = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(addr);
    return sleAddr;
}
}

DisClientStackAdapter::DisClientStackAdapter()
{}

DisClientStackAdapter::~DisClientStackAdapter() = default;

void DisClientStackAdapter::OnConnectionStateChanged(
    SLE_Addr_S *addr, NLSTK_DisConnectState_E curState, NLSTK_DisConnectState_E preState, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[DIS Adapter] addr(%{public}s), curState(%{public}d), preState(%{public}d), ret(%{public}d)",
        GET_ENCRYPT_ADDR(device), curState, preState, ret);
    if (preState == DIS_CONNECTING && curState == DIS_DISCONNECTED) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_DIS, ret);
    }
    DisService *disService = DisService::GetDisService();
    NL_CHECK_RETURN(disService, "disService is null.");
    disService->NotifyStateChanged(
        device, static_cast<SleConnectState>(curState), static_cast<SleConnectState>(preState));
}

int DisClientStackAdapter::RegisteCallBackToStack()
{
    NLSTK_DisClientCbk_S cb = {};
    cb.stateChangeCbk = &OnConnectionStateChanged;

    NLSTK_Errcode_E ret = NLSTK_DisRegisterCallbBack(&cb);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[DIS Adapter] register callback to stack failed, ret(%{public}d).", ret);
        return DIS_FAILURE;
    }
    return DIS_SUCCESS;
}

int DisClientStackAdapter::Connect(const RawAddress &addr)
{
    HILOGD("[DIS Adapter] Connect device addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint8_t num = 0;
    NLSTK_Errcode_E res = NLSTK_GetConnectedDeviceNum(&num);
    if (res != NLSTK_ERRCODE_SUCCESS || num >= DIS_MAX_CONNECTION_NUM) {
        DftReportPairInfo(addr.GetAddress(), PAIR_CONN_PATH_DIS, DIS_FAILURE, PAIR_DIS_REACH_MAX_NUM);
        HILOGE("[DIS Adapter] Get connected device num failed, res(%{public}d), num(%{public}d)", res, num);
        return DIS_FAILURE;
    }
    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[DIS Adapter] Connect failed, ret(%{public}d)", ret);
        return DIS_FAILURE;
    }
    return DIS_SUCCESS;
}

int DisClientStackAdapter::Disconnect(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = NLSTK_DisProfileDisconnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[DIS Adapter] Disconnect failed, ret(%{public}d)", ret);
        return DIS_FAILURE;
    }
    return DIS_SUCCESS;
}

uint16_t DisClientStackAdapter::ParseManufacturerInfo(uint8_t *inputData, uint16_t len, ManufacturerInfoType type)
{
    if (inputData == nullptr || len < DIS_MANUFACTURE_SIZE) {
        HILOGD("[DIS Adapter] ParseManufacturerInfo failed, len(%{public}d), type(%{public}d)", len, type);
        return DIS_INVALID_MANUFACTURER_INFO;
    }
    char *data = reinterpret_cast<char *>(inputData);
    uint16_t pos = 0;
    for (; pos < len; pos++) {
        if (data[pos] == '-' && (pos + DIS_MANUFACTURE_SIZE == len)) {
            break;
        }
    }
    if (pos == len) {
        HILOGE("[DIS Adapter] data is wrong, len(%{public}d)", len);
        return DIS_INVALID_MANUFACTURER_INFO;
    }

    uint8_t productId[DIS_PRODUCT_ID_SIZE] = {0};
    uint8_t vendorId[DIS_VENDOR_ID_SIZE] = {0};
    uint8_t vension[DIS_VENSION_ID_SIZE] = {0};
    int ret = sscanf_s(data + pos, "-%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx", &productId[1], &productId[0],
        &vendorId[1], &vendorId[0], &vension[1], &vension[0]);
    if (ret < 0) {
        HILOGE("[DIS Adapter]:sscanf_s failed");
        return DIS_INVALID_MANUFACTURER_INFO;
    }

    uint16_t result = DIS_INVALID_MANUFACTURER_INFO;
    switch (type) {
        case ManufacturerInfoType::MANUFACTURER_INFO_VENDOR_ID:
            result = static_cast<uint16_t>(vendorId[0]) +
                static_cast<uint16_t>(static_cast<uint16_t>(vendorId[1]) << DIS_SHIFT_OPRATURN_8);
            break;
        case ManufacturerInfoType::MANUFACTURER_INFO_PRODUCT_ID:
            result = static_cast<uint16_t>(productId[0]) +
                static_cast<uint16_t>(static_cast<uint16_t>(productId[1]) << DIS_SHIFT_OPRATURN_8);
            break;
        case ManufacturerInfoType::MANUFACTURER_INFO_VERSION:
            result = static_cast<uint16_t>(vension[0]) +
                static_cast<uint16_t>(static_cast<uint16_t>(vension[1]) << DIS_SHIFT_OPRATURN_8);
            break;
        default:
            HILOGE("[DIS Adapter]:type error, type(%{public}d)", type);
            break;
    }
    HILOGI("[DIS Adapter]:type(%{public}d), result(%{public}d)", type, result);
    return result;
}

uint16_t DisClientStackAdapter::GetDeviceVendorId(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    NLSTK_VariableData_S tempInfo = {};
    tempInfo.data = new (std::nothrow) uint8_t[DIS_MAX_VAR_LEN];
    NL_CHECK_RETURN_RET(tempInfo.data, DIS_INVALID_VENDOR_ID, "[DIS Adapter] tempInfo.data is null");
    tempInfo.len = DIS_MAX_VAR_LEN;
    NLSTK_Errcode_E res = NLSTK_DisReadInfo(&stackAddr, NLSTK_DisInfoType_E::DIS_MANUFACTURER_INFO, &tempInfo);
    if (res != NLSTK_ERRCODE_SUCCESS) {
        delete[] tempInfo.data;
        tempInfo.data = nullptr;
        return DIS_INVALID_VENDOR_ID;
    }
    // 计算vendor id
    uint16_t outRes = ParseManufacturerInfo(tempInfo.data, tempInfo.len,
        ManufacturerInfoType::MANUFACTURER_INFO_VENDOR_ID);
    std::string manufacturer(tempInfo.data, tempInfo.data + tempInfo.len);
    DftCachePeerManufacturer(device.GetAddress(), manufacturer);
    DftDeviceManager::GetInstance().UpdateManufacturer(device, manufacturer);
    delete[] tempInfo.data;
    tempInfo.data = nullptr;
    return outRes;
}

uint16_t DisClientStackAdapter::GetDeviceProductId(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    NLSTK_VariableData_S tempInfo = {};
    tempInfo.data = new (std::nothrow) uint8_t[DIS_MAX_VAR_LEN];
    NL_CHECK_RETURN_RET(tempInfo.data, DIS_INVALID_PRODUCT_ID, "[DIS Adapter] tempInfo.data is null");
    tempInfo.len = DIS_MAX_VAR_LEN;
    NLSTK_Errcode_E res = NLSTK_DisReadInfo(&stackAddr, NLSTK_DisInfoType_E::DIS_MANUFACTURER_INFO, &tempInfo);
    if (res != NLSTK_ERRCODE_SUCCESS) {
        delete[] tempInfo.data;
        tempInfo.data = nullptr;
        return DIS_INVALID_PRODUCT_ID;
    }
    // 计算product id
    uint16_t outRes = ParseManufacturerInfo(tempInfo.data, tempInfo.len,
        ManufacturerInfoType::MANUFACTURER_INFO_PRODUCT_ID);
    delete[] tempInfo.data;
    tempInfo.data = nullptr;
    return outRes;
}

uint16_t DisClientStackAdapter::GetDeviceVersion(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    NLSTK_VariableData_S tempInfo = {};
    tempInfo.data = new (std::nothrow) uint8_t[DIS_MAX_VAR_LEN];
    NL_CHECK_RETURN_RET(tempInfo.data, DIS_INVALID_VERSION_INFO, "[DIS Adapter] tempInfo.data is null");
    tempInfo.len = DIS_MAX_VAR_LEN;
    NLSTK_Errcode_E res = NLSTK_DisReadInfo(&stackAddr, NLSTK_DisInfoType_E::DIS_MANUFACTURER_INFO, &tempInfo);
    if (res != NLSTK_ERRCODE_SUCCESS) {
        delete[] tempInfo.data;
        tempInfo.data = nullptr;
        return DIS_INVALID_VERSION_INFO;
    }
    // 计算 version
    uint16_t outRes = ParseManufacturerInfo(tempInfo.data, tempInfo.len,
        ManufacturerInfoType::MANUFACTURER_INFO_VERSION);
    delete[] tempInfo.data;
    tempInfo.data = nullptr;
    return outRes;
}

uint32_t DisClientStackAdapter::GetAppearanceInfo(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    uint32_t appearance = DIS_INVALID_APPEARANCE_INFO;
    NLSTK_Errcode_E ret = NLSTK_DisReadAppearanceInfo(&stackAddr, &appearance);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[DIS Adapter] get appearance failed, ret(%{public}d)", ret);
        return DIS_INVALID_APPEARANCE_INFO;
    }
    HILOGD("[DIS Adapter] appearance(%{public}d)", appearance);
    return appearance;
}

std::string DisClientStackAdapter::GetNameInfo(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    NLSTK_VariableData_S tempInfo = {};
    tempInfo.data = new (std::nothrow) uint8_t[DIS_MAX_VAR_LEN]();
    NL_CHECK_RETURN_RET(tempInfo.data, "", "[DIS Adapter] tempInfo.data is null");
    tempInfo.len = DIS_MAX_VAR_LEN;
    NLSTK_Errcode_E res = NLSTK_DisReadInfo(&stackAddr, NLSTK_DisInfoType_E::DIS_LOCAL_ALIAS_INFO, &tempInfo);
    if (res != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[DIS Adapter] NLSTK_DisReadInfo get info failed, res(%{public}d)", res);
        delete[] tempInfo.data;
        tempInfo.data = nullptr;
        return "";
    }
    std::string name(tempInfo.data, tempInfo.data + tempInfo.len);
    HILOGD("[DIS Adapter] name size (%{public}zu)", name.size());
    delete[] tempInfo.data;
    tempInfo.data = nullptr;
    return name;
}

void DisClientStackAdapter::GetDeviceInformation(const RawAddress &device, DeviceInformation &information)
{
    HILOGI("enter");
    NLSTK_DisAllPropInfo_S *allProp = new (std::nothrow) NLSTK_DisAllPropInfo_S;
    NL_CHECK_RETURN(allProp, "[DIS Adapter] mem alloc failed");
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    allProp->addr = stackAddr;
    NLSTK_Errcode_E res = NLSTK_DisReadAllInfo(allProp);
    if (res != NLSTK_ERRCODE_SUCCESS) {
        delete allProp;
        allProp = nullptr;
        HILOGE("[DIS Adapter] NLSTK_DisReadAllInfo res(%{public}d)", res);
        return;
    }
    if (!allProp->find) {
        delete allProp;
        allProp = nullptr;
        HILOGE("[DIS Adapter] NLSTK_DisReadAllInfo not find device info");
        return;
    }
    std::string manufacturer(allProp->manufacturerInfo.var, allProp->manufacturerInfo.var + DIS_MAX_VAR_LEN);
    std::string deviceModel(allProp->deviceModel.var, allProp->deviceModel.var + DIS_MAX_VAR_LEN);
    information.SetManufacturerData(manufacturer);
    information.SetModelData(deviceModel);
    HILOGI("[DIS Adapter]device(%{public}s) get device info: (%{public}s)",
        GET_ENCRYPT_ADDR(device), DeviceInformationToString(information).c_str());
    delete allProp;
    allProp = nullptr;
}

std::string DisClientStackAdapter::DeviceInformationToString(const DeviceInformation &info)
{
    std::ostringstream oss;
    oss << "DeviceInformation=[manufactureData:" << SleUtils::StringDataToHexString(info.GetManufacturerData()) <<
                ", modelData:" << SleUtils::StringDataToHexString(info.GetModelData()) << "]";
    return oss.str();
}
} // namespace Nearlink
} // namespace OHOS