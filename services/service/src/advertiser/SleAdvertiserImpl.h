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
#ifndef SLE_ADVERTISER_IMPL_H
#define SLE_ADVERTISER_IMPL_H

#include <atomic>
#include <future>
#include <map>
#include <mutex>
#include <vector>

#include "nearlink_def.h"
#include "SleDefs.h"
#include "interface_advertiser_service.h"
#include "nearlink_timer.h"
#include "nlstk_devd_api.h"
#include "nlstk_public_define_ext.h"

/*
 * @brief The Nearlink subsystem.
 */
namespace OHOS {
namespace Nearlink {
enum class SleAdvState : int {
    SLE_ADV_STATE_IDLE = 0,
    SLE_ADV_STATE_ADVERTISING = 1
};
/**
 * @brief sle advertiser wrap data.
 */
struct SleAdvertiserImplData {
    SleAdvertiserImplData(const SleAdvertiserSettingsImpl &settings, const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &rspData, int curAdvStatus)
        : settings_(settings), advData_(advData), rspData_(rspData)
    {
        advStatus = curAdvStatus;
        advTargetStatus = static_cast<int>(ADVERTISE_STATUS::NOT_STARTED);
        advParams.advMinInterval = static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MIN);
        advParams.advMaxInterval = static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MAX);
        advParams.advMode = ADV_TYPE_NONCONN_SCANABLE;
        advParams.channelMap = ADV_CHNL_ALL;
        advParams.advFilterPolicy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
        advParams.peerAddrType = SLE_ADDR_TYPE::SLE_PUBLIC_ADDRESS_TYPE;
        advParams.linkRole = static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO);
        advParams.primaryFrameType = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
    }

    SleAdvertiserSettingsImpl settings_ {};
    SleAdvertiserDataImpl advData_ {};
    SleAdvertiserDataImpl rspData_ {};
    SleAdvParams advParams {};
    int advStatus {}; // 当前广播状态
    int advTargetStatus {}; // 目标广播状态
};

/**
 * @brief SLE advertiser.
 */
class SleAdvertiserImpl {
public:
    explicit SleAdvertiserImpl();

    ~SleAdvertiserImpl();

    void RegisterSleAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback);
    void RegisterSleConnectableAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback);

    /**
     * @brief Get advertising status
     *
     * @return @c advertiser status.
     */
    int GetAdvertisingStatus() const;

    /**
     * @brief Start Nearlink SLE Advertising.
     *
     * @param [in] Advertising parameters.
     * @param [in] Advertising data.
     * @param [in] Scan response data.
     * @param [in] Advertising handle.
     */
    void StartAdvertising(const SleAdvertiserSettingsImpl &settings, const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle);

    void SetAdvertisingData(const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse,
        uint8_t advHandle);

    /**
     * @brief Stop Nearlink SLE advertising.
     *
     * @param [in] Advertise handle.
     * @param [in] True:RPA timer auto stop.
     */
    void StopAdvertising(uint8_t advHandle);
    /**
     * @brief deregister scan callback to dd
     *
     * @return @c status.
     */
    int DeregisterCallbackToDd();

    /**
     * @brief Create advertising set handle.
     *
     * @return @c  Advertising handle.
     */
    uint8_t CreateAdvertiserSetHandle() const;
    uint8_t CreateConnectableAdvertiserSetHandle() const;

    void StopAdvertisingAll(std::shared_ptr<std::promise<void>> &promise);

    void EnableAdvertising(int32_t advHandle);

    void DisableAdvertising(int32_t advHandle);

private:
    void ChangeAdvertisingStatus(int newAdvStatus, int &advStatus, uint8_t advHandle) const;

    void HandleDdEvent(const int event, uint8_t advHandle, uint8_t result);
    void DdAdvDataSetCompleteEvt(uint8_t advHandle, uint8_t result);
    void DdAdvDataUpdateComplete(uint8_t advHandle, uint8_t result, int &advStatus);
    void DdAdvScanRspDataUpdateComplete(uint8_t advHandle, uint8_t result, int &advStatus);
    void DdAdvScanRspDataSetCompleteEvt(uint8_t advHandle, uint8_t result);
    void DdAdvEnableCompleteEvt(uint8_t advHandle, uint8_t result);
    void DdAdvDisableCompleteEvt(uint8_t advHandle, uint8_t result);
    void DdAdvRemoveCompleteEvt(uint8_t advHandle, uint8_t result);
    void DdAdvTerminatedCompleteEvt(uint8_t advHandle, uint8_t result);

    static void AdvEventResult(NLSTK_DevdAdvCbkParam_S *param);

    int SetAdvToDd(const SleAdvertiserSettingsImpl &settings,
        const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const;
    int UpdateAdvDataToDd(const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const;
    int SetAdvEnableToDd(uint8_t advHandle, bool isEnable) const;
    int SetExAdvEnableToDd(uint8_t advHandle, bool isEnable) const;
    int SetExAdvBatchEnableToDd(bool isEnable) const;
    int CheckAdvertiserPara(const SleAdvertiserSettingsImpl &settings, const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &scanResponse) const;
    void SetAdvMode(SleAdvertiserImplData &advImplData, uint8_t advMode) const;
    void SetMaxInterval(SleAdvertiserImplData &advImplData, uint32_t maxinterval) const;
    void SetMinInterval(SleAdvertiserImplData &advImplData, uint32_t mininterval) const;
    void SetFilter(SleAdvertiserImplData &advImplData, bool scanReqAllowlistOnly, bool connectAllowlistOnly) const;
    void SetChannelMap(SleAdvertiserImplData &advImplData, const SLE_ADV_CHANNEL &channelMap) const;
    void SetLinkRole(SleAdvertiserImplData &advImplData, uint8_t role) const;
    void SetPrimaryFrameType(SleAdvertiserImplData &advImplData, uint8_t primaryFrameType) const;
    void LegacyAdvertising(const SleAdvertiserSettingsImpl &settings,
        const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle);
    void StartLegacyAdvOrExAdv(const SleAdvertiserSettingsImpl &settings,
        const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle);
    NLSTK_ERRCODE SetAdvParam(uint8_t advHandle, const SleAdvertiserSettingsImpl &settings, bool hasScanRsp) const;
    bool IsScanableAdv(uint8_t advHandle) const;
    int CheckScanRspDataWhenUpdateAdvData(const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const;
    NLSTK_ERRCODE SetAdvExtParam(NLSTK_DevdSetAdvParams_S &advParams, const SleAdvertiserSettingsImpl &settings) const;
    NLSTK_ERRCODE SetAdvConnParam(NLSTK_DevdSetAdvParams_S &advParams) const;
    NLSTK_ERRCODE FillAdvParam(uint8_t advHandle, NLSTK_DevdSetAdvParams_S &advParams,
        const SleAdvertiserSettingsImpl &settings) const;
    NLSTK_ERRCODE FillAdvData(NLSTK_DevdAdvData_S &data, const SleAdvertiserDataImpl &sleAdvData) const;
    NLSTK_ERRCODE FillScanRspData(NLSTK_DevdAdvData_S &data, const SleAdvertiserDataImpl &scanRspData) const;
    void PrintSetAdvLog( NLSTK_DevdSetAdvParams_S &advParams) const;
    static uint32_t GetMaxAdvertisingDataLength(const SleAdvertiserSettingsImpl &settings);
    static void TimerCallback(SleAdvertiserImpl *context, uint8_t advHandle);
    void RemoveAdvHandle(uint8_t handle, int &advStatus) const;
    void SendStopAdvStatus(int resultCode, uint8_t advHandle);

    void OnStartResultEvent(int result, uint8_t advHandle) const;
    void OnStopResultEvent(int result, uint8_t advHandle) const;
    void OnEnableResultEvent(int result, uint8_t advHandle) const;
    void OnDisableResultEvent(int result, uint8_t advHandle) const;
    void OnSetAdvDataEvent(int result, uint8_t advHandle) const;

    std::weak_ptr<ISleAdvertiserCallback> callback_;
    std::weak_ptr<ISleAdvertiserCallback> connectableCallback_;
    std::shared_ptr<std::promise<void>> stopAllAdvPromise_ {nullptr};
    SLE_DISALLOW_COPY_AND_ASSIGN(SleAdvertiserImpl);
    DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_ADVERTISER_IMPL_H
