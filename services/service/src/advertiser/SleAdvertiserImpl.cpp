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
#include <memory>

#include "SleServiceManager.h"
#include "interface_advertiser_service.h"
#include "SleFeature.h"
#include "SleProperties.h"
#include "SleUtils.h"
#include "ThreadUtil.h"
#include "nlstk_devd_api.h"
#include "nlstk_api_type_ext.h"

#include "SleAdvertiserImpl.h"
namespace OHOS {
namespace Nearlink {
static SleAdvertiserImpl *g_advertiserImpl = nullptr;
struct SleAdvertiserImpl::impl {
    std::map<uint8_t, SleAdvertiserImplData> advHandleSettingDatas_ {};
    uint8_t connectableHandle_;
};

SleAdvertiserImpl::SleAdvertiserImpl() : pimpl(std::make_unique<SleAdvertiserImpl::impl>())
{
    LOG_INFO("enter");
    g_advertiserImpl = this;
    pimpl->connectableHandle_ = static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
}

SleAdvertiserImpl::~SleAdvertiserImpl()
{
    LOG_INFO("enter");
    pimpl->advHandleSettingDatas_.clear();
    g_advertiserImpl = nullptr;
}

void SleAdvertiserImpl::RegisterSleAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback)
{
    LOG_INFO("enter");
    callback_ = callback;
}

void SleAdvertiserImpl::RegisterSleConnectableAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback)
{
    LOG_INFO("enter");
    connectableCallback_ = callback;
}
//LCOV_EXCL_START
void SleAdvertiserImpl::AdvEventResult(NLSTK_DevdAdvCbkParam_S *param)
{
    if (param == nullptr) {
        LOG_ERROR("param is nullptr");
        return;
    }
    LOG_INFO("event=%{public}hhu, adv_handle=%{public}hhu, result=%{public}hhu", param->event, param->advHandle,
        param->result);
    DoInAdvThread([advImp = g_advertiserImpl, event = param->event, handle = param->advHandle, 
        result = param->result]() -> void {
        advImp->HandleDdEvent(event, handle, result);
    });
}

void SleAdvertiserImpl::ChangeAdvertisingStatus(int newAdvStatus, int &advStatus, uint8_t advHandle) const
{
    const std::map<int, std::string> advStatusString = {
        { static_cast<int>(ADVERTISE_STATUS::NOT_STARTED), "NOT_STARTED" },
        { static_cast<int>(ADVERTISE_STATUS::STARTTING), "STARTTING" },
        { static_cast<int>(ADVERTISE_STATUS::STARTED), "STARTED" },
        { static_cast<int>(ADVERTISE_STATUS::DISABLING), "DISABLING" },
        { static_cast<int>(ADVERTISE_STATUS::DISABLED), "DISABLED" },
        { static_cast<int>(ADVERTISE_STATUS::ENABLING), "ENABLING" },
        { static_cast<int>(ADVERTISE_STATUS::ENABLED), "ENABLED" },
        { static_cast<int>(ADVERTISE_STATUS::REMOVING), "REMOVING" },
        { static_cast<int>(ADVERTISE_STATUS::UPDATING), "UPDATING" },
    };
    auto iter = advStatusString.find(newAdvStatus);
    if (iter != advStatusString.end()) {
        LOG_INFO("newAdvStatus=%{public}s, advHandle=%{public}d", iter->second.c_str(), advHandle);
        advStatus = newAdvStatus;
    } else {
        LOG_INFO("error status! newAdvStatus=%{public}d", newAdvStatus);
    }
}

int SleAdvertiserImpl::GetAdvertisingStatus() const
{
    for (auto iter = pimpl->advHandleSettingDatas_.begin(); iter != pimpl->advHandleSettingDatas_.end(); iter++) {
        if (iter->second.advStatus != static_cast<int>(ADVERTISE_STATUS::NOT_STARTED)) {
            return static_cast<int>(SleAdvState::SLE_ADV_STATE_ADVERTISING);
        }
    }
    return static_cast<int>(SleAdvState::SLE_ADV_STATE_IDLE);
}

int SleAdvertiserImpl::CheckAdvertiserPara(const SleAdvertiserSettingsImpl &settings,
    const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse) const
{
    bool isLegacyMode = settings.IsLegacyMode();
    if (isLegacyMode) {
        if (advData.GetPayload().length() > SLE_LEGACY_ADV_DATA_LEN_MAX) {
            LOG_ERROR("Legacy advertising data too big.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
        if (scanResponse.GetPayload().length() > SLE_LEGACY_ADV_DATA_LEN_MAX) {
            LOG_ERROR("Legacy scan response data too big.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
    } else {
        bool isCodedPhySupported = SleFeature::GetInstance().IsSleCodedPhySupported();
        bool is2MPhySupported = SleFeature::GetInstance().IsSle2MPhySupported();
        int primaryPhy = settings.GetPrimaryPhy();
        int secondaryPhy = settings.GetSecondaryPhy();

        if ((!isCodedPhySupported) && (primaryPhy == static_cast<uint8_t>(SlePhyType::PHY_LE_CODED))) {
            LOG_ERROR("Unsupported primary coded PHY selected.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
        if ((!is2MPhySupported) && (secondaryPhy == static_cast<uint8_t>(SlePhyType::PHY_LE_CODED))) {
            LOG_ERROR("Unsupported primary 2M PHY selected.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
        size_t maxData = GetMaxAdvertisingDataLength(settings);
        if (advData.GetPayload().length() > maxData) {
            LOG_ERROR("Advertising data too big.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
        if (scanResponse.GetPayload().length() > maxData) {
            LOG_ERROR("Scan response data too big.");
            return NLSTK_ERRCODE_PARAM_ERR;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void SleAdvertiserImpl::StartAdvertising(const SleAdvertiserSettingsImpl &settings,
    const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle)
{
    LOG_INFO("advHandle=%{public}d", advHandle);
    int advStatus = static_cast<int>(ADVERTISE_STATUS::NOT_STARTED);
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter != pimpl->advHandleSettingDatas_.end()) {
        advStatus = iter->second.advStatus;
        if (advStatus != static_cast<int>(ADVERTISE_STATUS::NOT_STARTED)) {
            LOG_INFO("Advertising has started already");
            OnStartResultEvent(static_cast<int>(ADV_RESULT_FAILED_ALREADY_STARTED), advHandle);
            return;
        }
        pimpl->advHandleSettingDatas_.erase(iter);
    }
    if (CheckAdvertiserPara(settings, advData, scanResponse) != NLSTK_ERRCODE_SUCCESS) {
        OnStartResultEvent(static_cast<int>(ADV_RESULT_FAILED_CHECK_PARA_FAIL), advHandle);
        return;
    }

    auto insertRet = pimpl->advHandleSettingDatas_.insert(
        std::make_pair(advHandle, SleAdvertiserImplData(settings, advData, scanResponse, advStatus)));
    if (insertRet.second == false) {
        LOG_ERROR("advHandleSettingDatas_ insert failed");
        OnStartResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
    iter = insertRet.first;
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTTING), iter->second.advStatus, advHandle);

    int currentTargetStatus = iter->second.advTargetStatus;
    iter->second.advTargetStatus = static_cast<int>(ADVERTISE_STATUS::STARTED);
    int ret = SetAdvToDd(settings, advData, scanResponse, advHandle);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        RemoveAdvHandle(advHandle, iter->second.advStatus);
        iter->second.advTargetStatus = currentTargetStatus;
        OnStartResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
}

void SleAdvertiserImpl::SetAdvertisingData(const SleAdvertiserDataImpl &advData,
    const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle)
{
    LOG_DEBUG("advHandle=%{public}d", advHandle);
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invlalid handle! advHandle=%{public}u.", advHandle);
        OnSetAdvDataEvent(static_cast<int>(ADV_RESULT_FAILED_INVALID_HANDLE), advHandle);
        return;
    }
    if (iter->second.advStatus != static_cast<int>(ADVERTISE_STATUS::STARTED)) {
        LOG_ERROR("advertiser not started");
        OnSetAdvDataEvent(static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED), advHandle);
        return;
    }

    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::UPDATING), iter->second.advStatus, advHandle);
    int ret = UpdateAdvDataToDd(advData, scanResponse, advHandle);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("Update Adv Data to dd failed!");
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), iter->second.advStatus, advHandle);
        OnSetAdvDataEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
}

void SleAdvertiserImpl::SendStopAdvStatus(int resultCode, uint8_t advHandle)
{
    LOG_INFO("resultCode=%{public}d advHandle=%{public}u.", resultCode, advHandle);
    OnStopResultEvent(resultCode, advHandle);
    if (pimpl->advHandleSettingDatas_.empty() && stopAllAdvPromise_) {
        stopAllAdvPromise_->set_value();
        stopAllAdvPromise_.reset();
    }
}

void SleAdvertiserImpl::StopAdvertising(uint8_t advHandle)
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_INFO("invlalid handle! advHandle=%{public}u.", advHandle);
        SendStopAdvStatus(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }

    int currentAdvStatus = iter->second.advStatus;

    if (currentAdvStatus == static_cast<int>(ADVERTISE_STATUS::NOT_STARTED)) {
        LOG_INFO("advStatus is NOT_STARTED! advHandle=%{public}u.", advHandle);
        SendStopAdvStatus(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }

    if (currentAdvStatus != static_cast<int>(ADVERTISE_STATUS::STARTED) &&
        currentAdvStatus != static_cast<int>(ADVERTISE_STATUS::DISABLED)) {
        LOG_INFO("advStatus is not STARTED or DISABLED! advHandle=%{public}u.", advHandle);
        SendStopAdvStatus(static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED), advHandle);
        return;
    }

    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::DISABLING), iter->second.advStatus, advHandle);

    int currentTargetStatus = iter->second.advTargetStatus;
    iter->second.advTargetStatus = static_cast<int>(ADVERTISE_STATUS::NOT_STARTED);
    int ret = SetAdvEnableToDd(advHandle, false);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("Stop advertising failed!");
        ChangeAdvertisingStatus(currentAdvStatus, iter->second.advStatus, advHandle);
        iter->second.advTargetStatus = currentTargetStatus;
        SendStopAdvStatus(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
}

void SleAdvertiserImpl::StopAdvertisingAll(std::shared_ptr<std::promise<void>> &promise)
{
    HILOGI("enter");
    stopAllAdvPromise_ = promise;
    bool isNeedStop = false;
    for (auto iter = pimpl->advHandleSettingDatas_.begin(); iter != pimpl->advHandleSettingDatas_.end(); iter++) {
        isNeedStop = true;
        StopAdvertising(iter->first);
    }
    if (!isNeedStop) {
        HILOGI("already stop all adv");
        stopAllAdvPromise_->set_value();
        stopAllAdvPromise_.reset();
    }
}

void SleAdvertiserImpl::EnableAdvertising(int32_t advHandle)
{
    HILOGI("enter, advHandle=%{public}d", advHandle);

    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        HILOGE("invlalid handle! advHandle=%{public}d.", advHandle);
        OnEnableResultEvent(static_cast<int>(ADV_RESULT_FAILED_INVALID_HANDLE), advHandle);
        return;
    }
    if (iter->second.advStatus != static_cast<int>(ADVERTISE_STATUS::DISABLED)) {
        HILOGE("advertiser not disable");
        OnEnableResultEvent(static_cast<int>(ADV_RESULT_FAILED_ALREADY_STARTED), advHandle);
        return;
    }
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::ENABLING), iter->second.advStatus, advHandle);

    int currentTargetStatus = iter->second.advTargetStatus;
    iter->second.advTargetStatus = static_cast<int>(ADVERTISE_STATUS::ENABLED);
    int ret = SetAdvEnableToDd(advHandle, true);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("enable advertising failed!");
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::DISABLED), iter->second.advStatus, advHandle);
        iter->second.advTargetStatus = currentTargetStatus;
        OnEnableResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
}


void SleAdvertiserImpl::DisableAdvertising(int32_t advHandle)
{
    HILOGI("enter, advHandle=%{public}d", advHandle);
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_INFO("invlalid handle! advHandle=%{public}u.", advHandle);
        OnDisableResultEvent(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }
    if (iter->second.advStatus == static_cast<int>(ADVERTISE_STATUS::NOT_STARTED)) {
        LOG_INFO("advStatus is NOT_STARTED! advHandle=%{public}u.", advHandle);
        OnDisableResultEvent(static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED), advHandle);
        return;
    }
    if (iter->second.advStatus == static_cast<int>(ADVERTISE_STATUS::DISABLED)) {
        LOG_INFO("advStatus is DISABLED! advHandle=%{public}u.", advHandle);
        OnDisableResultEvent(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::DISABLING), iter->second.advStatus, advHandle);
    int currentTargetStatus = iter->second.advTargetStatus;
    iter->second.advTargetStatus = static_cast<int>(ADVERTISE_STATUS::DISABLED);
    int ret = SetAdvEnableToDd(advHandle, false);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("disable advertising failed!");
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), iter->second.advStatus, advHandle);
        iter->second.advTargetStatus = currentTargetStatus;
        OnDisableResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
}

int SleAdvertiserImpl::DeregisterCallbackToDd()
{
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::SetAdvParam(
    uint8_t advHandle, const SleAdvertiserSettingsImpl &settings, bool hasScanRsp) const
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        HILOGE("advHandle is invalid, advHandle=%{public}d", advHandle);
        return NLSTK_ERRCODE_PARAM_ERR;
    }

    HILOGI("isConnectable=%{public}d, hasScanRsp=%{public}d", settings.IsConnectable(), hasScanRsp);
    if (settings.IsConnectable()) {
        SetAdvMode(iter->second, hasScanRsp ? ADV_TYPE_CONNECTABLE_SCANABLE : ADV_TYPE_CONNECTABLE_NONSCAN);
    } else {
        SetAdvMode(iter->second, hasScanRsp ? ADV_TYPE_NONCONN_SCANABLE : ADV_TYPE_NONCONN_NONSCAN);
    }

    SetMaxInterval(iter->second, settings.GetInterval());
    SetMinInterval(iter->second, settings.GetInterval());
    SetFilter(iter->second, false, false);
    SetChannelMap(iter->second, ADV_CHNL_ALL);
    SetLinkRole(iter->second, settings.GetLinkRole());
    SetPrimaryFrameType(iter->second, settings.GetPrimaryFrameType());
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::SetAdvExtParam(NLSTK_DevdSetAdvParams_S &advParams,
    const SleAdvertiserSettingsImpl &settings) const
{
    // 失败时返回，其他资源设置失败后随着释放，成功后在SetAdvToDd()最后释放
    advParams.param.extParam = new (std::nothrow) NLSTK_DevdAdvExtParam_S();
    NL_CHECK_RETURN_RET(advParams.param.extParam, NLSTK_ERRCODE_MALLOC_FAIL, "advParams.param.extParam is nullptr");
    (void)memset_s(advParams.param.extParam, sizeof(NLSTK_DevdAdvExtParam_S), 0x00, sizeof(NLSTK_DevdAdvExtParam_S));

    advParams.param.extParam->advFilterPolicy = ADV_FLT_ANY_SCAN_ANY_CONNECT;
    advParams.param.extParam->advSid = 0;
    advParams.param.extParam->advTxPower = settings.GetTxPower();

    advParams.param.extParam->phyParam.primAdvPhy = ADV_PHY_1M;
    advParams.param.extParam->phyParam.secondAdvFrame = ADV_FRAME_TYPE_GFSK;
    advParams.param.extParam->phyParam.secondAdvPhy = ADV_PHY_1M;

    advParams.param.extParam->phyParam.secondAdvMcs = ADV_MCS_06;
    advParams.param.extParam->phyParam.secondAdvMaxSkip = 0;

    advParams.param.extParam->scanParam.scanReqNotifEnable = 0;
    advParams.param.extParam->scanParam.scanReqRecvNumberMax = 1;
    advParams.param.extParam->scanParam.scanReqRxDurMax = 0;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::SetAdvConnParam(NLSTK_DevdSetAdvParams_S &advParams) const
{
    // 失败时返回，其他资源设置失败后随着释放，成功后在SetAdvToDd()最后释放
    advParams.param.connParam = new (std::nothrow) NLSTK_DevdConnParam_S();
    NL_CHECK_RETURN_RET(advParams.param.connParam, NLSTK_ERRCODE_MALLOC_FAIL, "connParam is nullptr");
    advParams.param.connParam->intervalMin = CM_CONN_PRIVATE_MIN_INTERVAL; // Customization
    advParams.param.connParam->intervalMax = CM_CONN_PRIVATE_MAX_INTERVAL; // Customization
    advParams.param.connParam->maxLatency = CM_CONN_LATENCY; // Customization
    advParams.param.connParam->supervisionTimeout = CM_CONN_PRIVATE_TIMEOUT;  // Customization
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::FillAdvParam(uint8_t advHandle, NLSTK_DevdSetAdvParams_S &advParams,
    const SleAdvertiserSettingsImpl &settings) const
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        HILOGE("advHandle is invalid, advHandle=%{public}d", advHandle);
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    advParams.param.advHandle = advHandle;
    advParams.param.advMode = iter->second.advParams.advMode; // Customization
    advParams.param.advGtRole = iter->second.advParams.linkRole; // Customization
    advParams.param.advIntervalMin = iter->second.advParams.advMinInterval;
    advParams.param.advIntervalMax = iter->second.advParams.advMaxInterval;
    advParams.param.advChannelMap = iter->second.advParams.channelMap;
    advParams.param.primaryFrameType = iter->second.advParams.primaryFrameType;

    HILOGI("start advertising role = %{public}u, advMode = %{public}u, advParams.param.primaryFrameType "
           "%{public}hhu",
        iter->second.advParams.linkRole,
        iter->second.advParams.advMode,
        iter->second.advParams.primaryFrameType);

    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> addr = settings.GetOwnAddr();
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> invalidAddr = {0};
    if (std::equal(addr.begin(), addr.end(), invalidAddr.begin())) {
        advParams.param.ownAddr = SleProperties::GetInstance().GetLocalSleAddress();
        advParams.param.ownAddr.type = SLE_PUBLIC_ADDRESS_TYPE; // Customization
    } else {
        std::copy(addr.begin(), addr.end(), advParams.param.ownAddr.addr);
        advParams.param.ownAddr.type = settings.GetOwnAddrType(); // Customization
    }

    // 设置广播拓展参数
    NL_CHECK_RETURN_RET(
        SetAdvExtParam(advParams, settings) == NLSTK_ERRCODE_SUCCESS, NLSTK_ERRCODE_FAIL, "set adv ext params failed.");

    // 设置广播连接参数
    NL_CHECK_RETURN_RET(
        SetAdvConnParam(advParams) == NLSTK_ERRCODE_SUCCESS, NLSTK_ERRCODE_FAIL, "set adv conn params failed.");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::FillAdvData(NLSTK_DevdAdvData_S &data, const SleAdvertiserDataImpl &sleAdvData) const
{
    data.advDataLen = sleAdvData.GetPayload().size();
    data.advData = new (std::nothrow) uint8_t[data.advDataLen];
    NL_CHECK_RETURN_RET(data.advData, NLSTK_ERRCODE_POINTER_NULL, "advData is nullptr");
    (void)memset_s(data.advData, data.advDataLen, 0x00, data.advDataLen);
    for (int i = 0; i < data.advDataLen; i++) {
        data.advData[i] = sleAdvData.GetPayload()[i];
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_ERRCODE SleAdvertiserImpl::FillScanRspData(NLSTK_DevdAdvData_S &data,
    const SleAdvertiserDataImpl &scanRspData) const
{
    data.scanRspDataLen = scanRspData.GetPayload().size();
    if (data.scanRspDataLen == 0) {
        data.scanRspData = NULL;
        HILOGI("scan rsp data len is 0 set scanRspData to NULL, no need scan rsp");
        return NLSTK_ERRCODE_SUCCESS;
    }
    data.scanRspData = new (std::nothrow) uint8_t[data.scanRspDataLen];
    NL_CHECK_RETURN_RET(data.scanRspData, NLSTK_ERRCODE_POINTER_NULL, "scanRspData is nullptr");
    (void)memset_s(data.scanRspData, data.scanRspDataLen, 0x00, data.scanRspDataLen);
    for (int i = 0; i < data.scanRspDataLen; i++) {
        data.scanRspData[i] = scanRspData.GetPayload()[i];
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void SleAdvertiserImpl::PrintSetAdvLog(NLSTK_DevdSetAdvParams_S &advParams) const
{
    std::vector<uint8_t> adv(advParams.data.advData, advParams.data.advData + advParams.data.advDataLen);
    std::vector<uint8_t> rsp(
        advParams.data.scanRspData, advParams.data.scanRspData + advParams.data.scanRspDataLen);
    HILOGI("SetAdvToDd advMode:%{public}d, own_addr=%{public}02x:%{public}02x:**:**:**:%{public}02x, "
           "own_addr_type=%{public}d, adv_interval=%{public}d advDataLen=%{public}d advData=%{public}s "
           "scanRspDataLen=%{public}d, scanRspData=%{public}s, primaryFrameType: %{public}hhu, ",
        advParams.param.advMode,
        advParams.param.ownAddr.addr[0],
        advParams.param.ownAddr.addr[1],
        advParams.param.ownAddr.addr[5], // addr 5th byte
        advParams.param.ownAddr.type,
        advParams.param.advIntervalMin,
        advParams.data.advDataLen,
        SleUtils::ConvertIntToHexString(adv).c_str(),
        advParams.data.scanRspDataLen,
        SleUtils::ConvertIntToHexString(rsp).c_str(),
        advParams.param.primaryFrameType);
}

int SleAdvertiserImpl::SetAdvToDd(const SleAdvertiserSettingsImpl &settings,
    const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const
{
    SleAdvertiserDataImpl sleAdvData = advData;
    SleAdvertiserDataImpl scanRspData = scanResponse;

    NLSTK_DevdSetAdvParams_S advParams;
    (void)memset_s(&advParams, sizeof(advParams), 0x00, sizeof(advParams));
    advParams.accessMode = ADV_ACCESS_MODE_SLE;
    advParams.discoveryLevel = advData.GetFlags();

    bool isScanRspExist = (scanResponse.GetPayload().size() != 0);
    NL_CHECK_RETURN_RET(SetAdvParam(advHandle, settings, isScanRspExist) == NLSTK_ERRCODE_SUCCESS,
        NLSTK_ERRCODE_PARAM_ERR, "set adv params failed.");

    if (FillAdvParam(advHandle, advParams, settings) != NLSTK_ERRCODE_SUCCESS) {
        delete advParams.param.extParam;
        delete advParams.param.connParam;
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (FillAdvData(advParams.data, sleAdvData) != NLSTK_ERRCODE_SUCCESS) {
        delete advParams.param.extParam;
        delete advParams.param.connParam;
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (FillScanRspData(advParams.data, scanRspData) != NLSTK_ERRCODE_SUCCESS) {
        delete[] advParams.data.advData;
        delete advParams.param.extParam;
        delete advParams.param.connParam;
        return NLSTK_ERRCODE_PARAM_ERR;
    }

    PrintSetAdvLog(advParams);

    // 这里为开启广播入口
    NLSTK_ERRCODE ret = NLSTK_DevdStartAdv(&advParams);
    delete[] advParams.data.advData;
    if (advParams.data.scanRspData != NULL) {
        delete[] advParams.data.scanRspData;
    }
    if (advParams.param.extParam != NULL) {
        delete advParams.param.extParam;
    }
    delete advParams.param.connParam;
    return ret;
}

int SleAdvertiserImpl::CheckScanRspDataWhenUpdateAdvData(const SleAdvertiserDataImpl &scanResponse,
    uint8_t advHandle) const
{
    if (IsScanableAdv(advHandle) && (scanResponse.GetPayload().size() == 0)) {
        HILOGI("advMode is SCANABLE, but no scanResponseData!");
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (!IsScanableAdv(advHandle) && (scanResponse.GetPayload().size() != 0)) {
        HILOGI("advMode is NONSCAN, but has scanResponseData!");
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

int SleAdvertiserImpl::UpdateAdvDataToDd(const SleAdvertiserDataImpl &advData,
    const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const
{
    SleAdvertiserDataImpl sleAdvData = advData;
    SleAdvertiserDataImpl scanRspData = scanResponse;

    NLSTK_DevdSetAdvData_S updateAdvData;
    (void)memset_s(&updateAdvData, sizeof(updateAdvData), 0x00, sizeof(updateAdvData));
    updateAdvData.advHandle = advHandle;
    if (CheckScanRspDataWhenUpdateAdvData(scanResponse, advHandle) != NLSTK_ERRCODE_SUCCESS) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (FillAdvData(updateAdvData.data, sleAdvData) != NLSTK_ERRCODE_SUCCESS) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    if (FillScanRspData(updateAdvData.data, scanRspData) != NLSTK_ERRCODE_SUCCESS) {
        delete[] updateAdvData.data.advData;
        return NLSTK_ERRCODE_PARAM_ERR;
    }

    std::vector<uint8_t> adv(updateAdvData.data.advData, updateAdvData.data.advData +
        updateAdvData.data.advDataLen);
    std::vector<uint8_t> rsp(updateAdvData.data.scanRspData, updateAdvData.data.scanRspData +
        updateAdvData.data.scanRspDataLen);
    HILOGI("UpdateAdvDataToDd advDataLen=%{public}d advData=%{public}s scanRspDataLen=%{public}d"
        " scanRspData=%{public}s",
        updateAdvData.data.advDataLen, SleUtils::ConvertIntToHexString(adv).c_str(),
        updateAdvData.data.scanRspDataLen, SleUtils::ConvertIntToHexString(rsp).c_str());

    NLSTK_ERRCODE ret = NLSTK_DevdSetAdvData(&updateAdvData);
    delete[] updateAdvData.data.advData;
    if (updateAdvData.data.scanRspData != NULL) {
        delete[] updateAdvData.data.scanRspData;
    }
    return ret;
}

int SleAdvertiserImpl::SetAdvEnableToDd(uint8_t advHandle, bool isEnable) const
{
    LOG_INFO("isEnable=%{public}d, adv_handle=%{public}d", isEnable, advHandle);
    NLSTK_DevdSetAdvEnable_S advEnable = {};
    memset_s(&advEnable, sizeof(advEnable), 0, sizeof(advEnable));
    advEnable.enable = isEnable;
    advEnable.advHandle = advHandle;
    return NLSTK_DevdEnableAdv(&advEnable);
}

void SleAdvertiserImpl::SetAdvMode(SleAdvertiserImplData &advImplData, uint8_t advMode) const
{
    advImplData.advParams.advMode = advMode;
}

void SleAdvertiserImpl::SetMinInterval(SleAdvertiserImplData &advImplData, uint32_t mininterval) const
{
    advImplData.advParams.advMinInterval = mininterval;
}

void SleAdvertiserImpl::SetMaxInterval(SleAdvertiserImplData &advImplData, uint32_t maxinterval) const
{
    advImplData.advParams.advMaxInterval = maxinterval;
}

void SleAdvertiserImpl::SetFilter(SleAdvertiserImplData &advImplData, bool scanReqAllowlistOnly,
    bool connectAllowlistOnly) const
{
    if ((!scanReqAllowlistOnly) && (!connectAllowlistOnly)) {
        advImplData.advParams.advFilterPolicy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
        return;
    }
    if ((scanReqAllowlistOnly) && (!connectAllowlistOnly)) {
        advImplData.advParams.advFilterPolicy = ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY;
        return;
    }
    if ((!scanReqAllowlistOnly) && (connectAllowlistOnly)) {
        advImplData.advParams.advFilterPolicy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
        return;
    }
    if ((scanReqAllowlistOnly) && (connectAllowlistOnly)) {
        advImplData.advParams.advFilterPolicy = ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST;
        return;
    }
}

void SleAdvertiserImpl::SetChannelMap(SleAdvertiserImplData &advImplData, const SLE_ADV_CHANNEL &channelMap) const
{
    advImplData.advParams.channelMap = channelMap;
}

void SleAdvertiserImpl::SetLinkRole(SleAdvertiserImplData &advImplData, uint8_t role) const
{
    if (role > static_cast<uint8_t>(SleLinkRole::G_NO_NEGO)) {
        HILOGE("unknown role %{public}u", role);
        advImplData.advParams.linkRole = static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO);
        return;
    }
    advImplData.advParams.linkRole = role;
}

void SleAdvertiserImpl::SetPrimaryFrameType(SleAdvertiserImplData &advImplData, uint8_t primaryFrameType) const
{
    if (primaryFrameType > static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4)) {
        HILOGE("unknown primaryFrameType %{public}hhu", primaryFrameType);
        advImplData.advParams.primaryFrameType =
            static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
        return;
    }
    advImplData.advParams.primaryFrameType = primaryFrameType;
}

uint8_t SleAdvertiserImpl::CreateAdvertiserSetHandle() const
{
    uint8_t newHandle = NLSTK_DevdCreateAdvHandle(&SleAdvertiserImpl::AdvEventResult);
    if (newHandle != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE)) {
        LOG_INFO("newHandle=%{public}d", newHandle);
        return newHandle;
    }
    LOG_INFO("no valid handle");
    return static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
}

uint8_t SleAdvertiserImpl::CreateConnectableAdvertiserSetHandle() const
{
    NL_CHECK_RETURN_RET(
        pimpl->connectableHandle_ == static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
        "[SleAdapter] already has sleConnectableHandle");
    uint8_t connectableHanlde = CreateAdvertiserSetHandle();
    pimpl->connectableHandle_ = connectableHanlde;
    LOG_INFO("create connectable handle : %{public}u", pimpl->connectableHandle_);
    return connectableHanlde;
}

void SleAdvertiserImpl::RemoveAdvHandle(uint8_t handle, int &advStatus) const
{
    LOG_INFO("handle=%{public}d", handle);
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::REMOVING), advStatus, handle);
    NLSTK_DevdRemoveAdv(&handle);
}

bool SleAdvertiserImpl::IsScanableAdv(uint8_t advHandle) const
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return false;
    }
    uint8_t advMode = iter->second.advParams.advMode;
    if (advMode == ADV_MODE_NONCONN_SCANABLE || advMode == ADV_MODE_CONNECTABLE_SCANABLE) {
        return true;
    }
    return false;
}

void SleAdvertiserImpl::DdAdvDataUpdateComplete(uint8_t advHandle, uint8_t result, int &advStatus)
{
    if (result != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("Set adv data failed! result=%{public}d.", result);
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), advStatus, advHandle);
        OnSetAdvDataEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
    if (IsScanableAdv(advHandle)) {
        return;
    }
    LOG_INFO("Set scan response data success!");
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), advStatus, advHandle);
    OnSetAdvDataEvent(static_cast<int>(NLSTK_ERRCODE_SUCCESS), advHandle);
}

void SleAdvertiserImpl::DdAdvDataSetCompleteEvt(uint8_t advHandle, uint8_t result)
{
    LOG_DEBUG("[SleAdvertiserImpl]");
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return;
    }

    if (iter->second.advStatus == static_cast<int>(ADVERTISE_STATUS::UPDATING)) {
        DdAdvDataUpdateComplete(advHandle, result, iter->second.advStatus);
        return;
    }
}

void SleAdvertiserImpl::DdAdvScanRspDataUpdateComplete(uint8_t advHandle, uint8_t result, int &advStatus)
{
    if (result != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("Set scan response data failed! result=%{public}d.", result);
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), advStatus, advHandle);
        OnSetAdvDataEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
    LOG_INFO("Set scan response data success!");
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), advStatus, advHandle);
    OnSetAdvDataEvent(result, advHandle);
}

void SleAdvertiserImpl::DdAdvScanRspDataSetCompleteEvt(uint8_t advHandle, uint8_t result)
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u", advHandle);
        return;
    }

    if (iter->second.advStatus == static_cast<int>(ADVERTISE_STATUS::UPDATING)) {
        DdAdvScanRspDataUpdateComplete(advHandle, result, iter->second.advStatus);
        return;
    }
}

void SleAdvertiserImpl::DdAdvEnableCompleteEvt(uint8_t advHandle, uint8_t result)
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return;
    }

    if (result != NLSTK_ERRCODE_SUCCESS) {
        if (iter->second.advTargetStatus == static_cast<int>(ADVERTISE_STATUS::ENABLED)) {
            LOG_ERROR("Enable advertising failed! result=%{public}d.", result);
            OnEnableResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
            return;
        }
        LOG_ERROR("Start advertising failed! result=%{public}d.", result);
        RemoveAdvHandle(advHandle, iter->second.advStatus);
        OnStartResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), iter->second.advStatus, advHandle);
    if (iter->second.advTargetStatus == static_cast<int>(ADVERTISE_STATUS::ENABLED)) {
        LOG_INFO("Enable advertising success!");
        OnEnableResultEvent(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }
    LOG_INFO("Start advertising success!");
    OnStartResultEvent(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
}

void SleAdvertiserImpl::DdAdvDisableCompleteEvt(uint8_t advHandle, uint8_t result)
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return;
    }

    if (result != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("Stop advertising failed! result=%{public}d, advHandle=%{public}d, SleServiceManager.state=%{public}d",
            result, advHandle, SleServiceManager::GetInstance()->GetState(ADAPTER_SLE));
        if (SleServiceManager::GetInstance()->IsDisabling()) {
            // 关星闪触发的停广播失败，当成停扫描成功处理。
            HILOGE("In disabling, stop adv fail, processed as stop ok");
            RemoveAdvHandle(advHandle, iter->second.advStatus);
            return;
        }

        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::STARTED), iter->second.advStatus, advHandle);
        if (iter->second.advTargetStatus == static_cast<int>(ADVERTISE_STATUS::DISABLED)) {
            OnDisableResultEvent(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
            return;
        }
        SendStopAdvStatus(static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR), advHandle);
        return;
    }
    if (iter->second.advTargetStatus == static_cast<int>(ADVERTISE_STATUS::DISABLED)) {
        LOG_INFO("DisableAdv advHandle=%{public}u.", advHandle);
        ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::DISABLED), iter->second.advStatus, advHandle);
        OnDisableResultEvent(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
        return;
    }
    LOG_INFO("RemoveAdvHandle advHandle=%{public}u.", advHandle);
    RemoveAdvHandle(advHandle, iter->second.advStatus);
}

void SleAdvertiserImpl::DdAdvRemoveCompleteEvt(uint8_t advHandle, uint8_t result)
{
    LOG_INFO("handle=%{public}d", advHandle);
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return;
    }
    ChangeAdvertisingStatus(static_cast<int>(ADVERTISE_STATUS::NOT_STARTED), iter->second.advStatus, advHandle);
    pimpl->advHandleSettingDatas_.erase(advHandle);
    SendStopAdvStatus(static_cast<int>(ADV_RESULT_SUCCESS), advHandle);
}

void SleAdvertiserImpl::DdAdvTerminatedCompleteEvt(uint8_t advHandle, uint8_t result)
{
    auto iter = pimpl->advHandleSettingDatas_.find(advHandle);
    if (iter == pimpl->advHandleSettingDatas_.end()) {
        LOG_ERROR("invalid handle! advHandle=%{public}u.", advHandle);
        return;
    }
    if (result != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("advertising terminated evt failed and ignore");
        return;
    }
    RemoveAdvHandle(advHandle, iter->second.advStatus);
}

void SleAdvertiserImpl::HandleDdEvent(const int event, uint8_t advHandle, uint8_t result)
{
    switch (event) {
        case SLE_DD_ADV_DATA_SET_COMPLETE_EVT:      // 保留更新广播数据完成的通知接口，目前暂未使用
            DdAdvDataSetCompleteEvt(advHandle, result);
            break;
        case SLE_DD_ADV_SCAN_RSP_DATA_SET_COMPLETE_EVT:     // 保留更新广播响应数据完成的通知接口，目前暂未使用
            DdAdvScanRspDataSetCompleteEvt(advHandle, result);
            break;
        case SLE_DD_ADV_ENABLE_COMPLETE_EVT:
            DdAdvEnableCompleteEvt(advHandle, result);
            break;
        case SLE_DD_ADV_DISABLE_COMPLETE_EVT:
            DdAdvDisableCompleteEvt(advHandle, result);
            break;
        case SLE_DD_ADV_REMOVE_COMPLETE_EVT:
            DdAdvRemoveCompleteEvt(advHandle, result);
            break;
        case SLE_DD_ADV_CLEAR_COMPLETE_EVT:
            LOG_ERROR("SLE_DD_ADV_CLEAR_COMPLETE_EVT");
            break;
        case SLE_DD_ADV_TERMINATED_COMPLETE_EVT:
            DdAdvTerminatedCompleteEvt(advHandle, result);
            break;
        default: {
            LOG_ERROR("Invalid event!");
            break;
        }
    }
}

uint32_t SleAdvertiserImpl::GetMaxAdvertisingDataLength(const SleAdvertiserSettingsImpl &settings)
{
    if (settings.IsLegacyMode()) {
        return SLE_LEGACY_ADV_DATA_LEN_MAX;
    } else {
        return SleFeature::GetInstance().GetBleMaximumAdvertisingDataLength();
    }
}

void SleAdvertiserImpl::OnStartResultEvent(int result, uint8_t advHandle) const
{
    if (pimpl->connectableHandle_ != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE) &&
        advHandle == pimpl->connectableHandle_) {
        auto connectableSpt = connectableCallback_.lock();
        if (connectableSpt) {
            connectableSpt->OnStartResultEvent(result, advHandle);
        }
    } else {
        auto spt = callback_.lock();
        if (spt) {
            spt->OnStartResultEvent(result, advHandle);
        }
    }
}

void SleAdvertiserImpl::OnStopResultEvent(int result, uint8_t advHandle) const
{
    if (pimpl->connectableHandle_ != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE) &&
        advHandle == pimpl->connectableHandle_) {
        if (result == static_cast<int>(ADV_RESULT_SUCCESS)) {
            HILOGD("clear connectableHandle_");
            pimpl->connectableHandle_ = static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
        }
        auto connectableSpt = connectableCallback_.lock();
        if (connectableSpt) {
            connectableSpt->OnStopResultEvent(result, advHandle);
        }
    } else {
        auto spt = callback_.lock();
        if (spt) {
            spt->OnStopResultEvent(result, advHandle);
        }
    }
}

void SleAdvertiserImpl::OnEnableResultEvent(int result, uint8_t advHandle) const
{
    if (pimpl->connectableHandle_ != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE) &&
        advHandle == pimpl->connectableHandle_) {
        auto connectableSpt = connectableCallback_.lock();
        if (connectableSpt) {
            connectableSpt->OnEnableResultEvent(result, advHandle);
        }
    } else {
        auto spt = callback_.lock();
        if (spt) {
            spt->OnEnableResultEvent(result, advHandle);
        }
    }
}

void SleAdvertiserImpl::OnDisableResultEvent(int result, uint8_t advHandle) const
{
    if (pimpl->connectableHandle_ != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE) &&
        advHandle == pimpl->connectableHandle_) {
        auto connectableSpt = connectableCallback_.lock();
        if (connectableSpt) {
            connectableSpt->OnDisableResultEvent(result, advHandle);
        }
    } else {
        auto spt = callback_.lock();
        if (spt) {
            spt->OnDisableResultEvent(result, advHandle);
        }
    }
}

void SleAdvertiserImpl::OnSetAdvDataEvent(int result, uint8_t advHandle) const
{
    if (pimpl->connectableHandle_ != static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE) &&
        advHandle == pimpl->connectableHandle_) {
        auto connectableSpt = connectableCallback_.lock();
        if (connectableSpt) {
            connectableSpt->OnSetAdvDataEvent(result, advHandle);
        }
    } else {
        auto spt = callback_.lock();
        if (spt) {
            spt->OnSetAdvDataEvent(result, advHandle);
        }
    }
}
//LCOV_EXCL_STOP
}  // namespace Nearlink
}  // namespace OHOS
