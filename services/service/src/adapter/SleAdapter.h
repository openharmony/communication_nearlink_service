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
#ifndef SLE_ADAPTER_H
#define SLE_ADAPTER_H

#include <map>
#include <memory>

#include "BaseObserverList.h"
#include "log_util.h"
#include "SleCoexist.h"
#include "SleHuksTool.h"
#include "SleInterfaceAdapterSub.h"
#include "SleProperties.h"
#include "SleSecurity.h"
#include "context.h"
#include "nearlink_safe_map.h"
#include "nearlink_safe_set.h"
#include "sle_uuid.h"
#include "nearlink_timer.h"
#include "nearlink_errorcode.h"
#include "ThreadUtil.h"
#include "SleServiceFfrtLog.h"
#include "nlstk_cfgdb_api.h"
#include "cm.h"
#include "cm_api.h"
#include "cm_icb_def.h"
#include "CdsmService.h"
#include "SleRemoteDeviceAdapter.h"

/*
 * @brief The Sle subsystem.
 */
namespace OHOS {
namespace Nearlink {

constexpr const int32_t CHANNEL_INDEX_NUM = 14;

// Adv status
enum class SleAdvState : int {
    SLE_ADV_STATE_IDLE = 0,
    SLE_ADV_STATE_ADVERTISING = 1
};

typedef struct {
    void *data;
    uint32_t dataLen;
} nbc_callback_param_t;

struct __attribute__((packed)) DisconChipInfo {
    uint16_t connHandle;
    int8_t signalStrength;
    uint32_t actualRssiTs;
    uint8_t rssiIdx[CHANNEL_INDEX_NUM];
    int8_t actualRssiValue[CHANNEL_INDEX_NUM];
};

struct __attribute__((packed)) PowerLevelInfo {
    uint16_t connHandle;
    uint8_t powerLevel;
};

/**
 *  @brief SLE Adpter implementation class
 */
class SleAdapter : public SleInterfaceAdapterSub, public utility::Context {
public:
    /**
     * @brief Constructor.
     */
    SleAdapter();

    /**
     * @brief Destructor.
     */
    ~SleAdapter();

    utility::Context *GetContext() override;

    /**
     *  @brief Turn on the SLE Nearlink adapter
     *
     *  @return @c true Turn on SLE Nearlink successfully
     *          @c false Failed to turn on SLE Nearlink
     */
    void Enable() override;

    /**
     *  @brief Turn off the SLE Nearlink adapter
     *
     *  @return @c true Turn off SLE Nearlink successfully
     *          @c false Failed to turn off SLE Nearlink
     */
    void Disable() override;

    /**
     *  @brief Processing after Nearlink startup
     *
     *  @return @c true success
     *          @c false failure
     */
    void PostEnable() override;

    /**
     *  @brief Get local host nearlink address
     *
     *  @return @c Local host nearlink address
     */
    std::string GetLocalAddress() const override;

    /**
     *  @brief Get local host nearlink name
     *
     *  @return @c Local host nearlink name
     */
    std::string GetLocalName() const override;

    /**
     *  @brief Set local host nearlink name
     *
     *  @param [in] name Device name.
     *  @return @c true success
     *          @c false failure
     */
    bool SetLocalName(const std::string &name) const override;

    // add adapter manager common api
    uint16_t GetDeviceVendorId(const RawAddress &device) const override;
    uint16_t GetDeviceProductId(const RawAddress &device) const override;
    uint16_t GetDeviceVersion(const RawAddress &device) const override;
    DeviceModel GetDeviceModel(const RawAddress &device) const override;
    std::string GetDeviceName(const RawAddress &device) const override;
    std::string GetAliasName(const RawAddress &device) const override;
    int GetDeviceAppearance(const RawAddress &device) const override;
    bool SetAliasName(const RawAddress &device, const std::string &name) const override;
    std::vector<Uuid> GetDeviceUuids(const RawAddress &device) const override;
    std::vector<RawAddress> GetPairedDevices() const override;
    std::vector<RawAddress> GetConnectedDevices() const override;
    void SetSleConnectionMode(int32_t connectionMode, int32_t duration) override;
    void SleConnectableParamSetComplete(uint8_t advHandle, uint8_t result);
    void SleConnectableDataSetComplete(uint8_t advHandle, uint8_t result);
    void SleConnectableEnableComplete(uint8_t advHandle, uint8_t result);
    void SleConnectableDisableComplete(uint8_t advHandle, uint8_t result);
    void SleConnectableRemoveAdvComplete(uint8_t advHandle, uint8_t result);
    bool StartPair(const RawAddress &device) override;
    bool StartCrediblePair(const RawAddress &device) override;
    bool CancelPairing(const RawAddress &device) override;
    bool RemovePair(const RawAddress &device) override;
    bool RemoveAllPairs() override;
    bool SetPairingPassCode(const RawAddress &device, const std::string &passCode) override;
    bool SetPairingConfirmation(const RawAddress &device) const override;
    bool ConnectAllProfile(const RawAddress &device) override;
    bool DisconnectAllProfile(const RawAddress &device) override;
    void DisconnectAllProfileForSilentPort(const RawAddress &device) override;
    bool IsBondedFromLocal(const RawAddress &device) const override;
    bool PairRequestReply(const RawAddress &device, bool accept) const override;
    bool IsAcbConnected(const RawAddress &device) const override;
    bool IsAcbEncrypted(const RawAddress &device) const override;
    uint8_t GetLinkRole(const RawAddress &device) const override;
    int GetPairState(const RawAddress &device) const override;
    int GetBondableMode() const override;
    bool SetBondableMode(int mode) const override;
    uint32_t GetSleMaxAdvertisingDataLength() const override;
    int GetIoCapability() const override;
    bool SetIoCapability(int ioCapability) const override;
    bool IsSleEnabled() const;

    int GetAcbState(const RawAddress &device) const override;
    uint32_t GetAcbCount() const override;
    SLE_Addr_S GetLocalSleAddress() const;
    bool IsLlPrivacySupported() const override;

    void RegisterSlePeripheralCallback(ISlePeripheralCallback &callback) const override;
    void DeregisterSlePeripheralCallback(ISlePeripheralCallback &callback) const override;
    void RegisterSleConnectionCallback(ISleConnectionCallback &callback) const override;
    void DeregisterSleConnectionCallback(ISleConnectionCallback &callback) const override;
    void RegisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const override;
    void DeregisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const override;
    void RegisterBleSecurityCallback();
    void DeregisterBleSecurityCallback() const;

    bool RegisterSleAdapterObserver(IAdapterSleObserver &observer) const override;
    bool DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const override;
    bool IsFeatureSupported(int32_t feature) const override;

    // pair
    bool StartPairTask(const RawAddress &device);
    bool CancelPairingTask(const RawAddress &device) const;
    void PairStartChanged(const RawAddress &device) const;
    void PairComplete(const RawAddress &device, const int status) const;
    void PairCmpSuccess(const RawAddress &device, int pairState, int connDirect, int pairDirect) const;
    void PairCmpFail(const RawAddress &device, int pairState) const;
    void PairingStatus(const RawAddress &device) const;
    void PairingRequest(const RawAddress &device, std::string &passKey, const int type) const;
    void EncryptionComplete(const RawAddress &device, const int status) const;
    void EncryptionKeyMissingComplete(const RawAddress &device) const;
    void UpdateKeyMissingCdsmGroup(const RawAddress &device) const;
    void CancelPairComplete(const RawAddress &device, const int status, const int unpairedReason) const;
    void CancelPairCompleteInner(const RawAddress &device) const;
    void CdsmCancelPairingProcess(const RawAddress &device) const;
    void RemoveNotPairedCloudDevice(const RawAddress &device) const override;
    // auto connect
    /**
     * @brief Read Remote Rssi Value.
     *
     * @return @c true sucessfull otherwise false.
     */
    int ReadRemoteRssiValue(const RawAddress &device) override;

    bool FactoryReset() const override;
    void CancelAllConnection() override;
    void CancelAllConnectionTask() const;
    int GetProfileConnState(const RawAddress &device) override;
    bool SleFreqHopping(const std::vector<uint8_t> &freq) override;
    bool WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) override;
    bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) override;

    bool GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) const override;
    bool GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) const override;
    bool SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const override;
    bool IsScanConnTypeAndFrameType4(const RawAddress &device, uint8_t connCompleteType) const override;
    void UpdateDeviceModelInfo(const std::string &address, const DeviceModel &deviceModel,
        const std::string &newModelId) override;
    void NotifyPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) const override;
    void NotifyConnectionStateChanged(
        const RawAddress &device, const SleConnectionChangedParam &connChangedParam) const override;
    void ConnectAcb(const RawAddress &device) override;
    bool DisconnectAction(const RawAddress &device, uint8_t discReason) const override;
    bool DisconnectAcb(const RawAddress &device, uint8_t discReason) const override;
    void ClearBgConnDevice() const override;
    bool UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) const override;
    void DelConnFrameType(const std::string &addr) override;
    void SetConnFrameType(const std::string &addr, uint8_t frameType) override;
    bool GetConnFrameType(const std::string &addr, uint8_t &frameType) const override;
    bool HasConnectedDevice() override;
    bool GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency) const override;
    bool GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency,
        uint16_t &interval) const override;
    void SetPhy(const RawAddress &device, uint8_t frameType, uint8_t phyType) override;
private:
    int RegisterCallbackToCm();
    int DeregisterCallbackToBtm() const;
    int RegisterCallbackToNbc();
    int DeregisterAllCallback() const;
    bool EnableTask();
    bool DisableTask();
    bool PostEnableTask() const;
    void LoadConfig() const;
    void ConnectionStateTask(const CM_LogicLinkState_S &connResult);
    void CreateNewPeripheralDevice(const SLE_Addr_S &addr, uint16_t lcid, uint8_t role,
        uint8_t connCompleteType) const;
    void ConnectionCompleteTask(const SLE_Addr_S &addr, uint16_t lcid, uint8_t role, uint8_t connCompleteType) const;
    void DisconnectionCompleteTask(uint16_t lcid,  const RawAddress &peerAddr, int reason) const;
    bool NeedBgConn(int acbConnState, int pairState, const RawAddress& peerAddr, int reason) const;
    void OnReadRemoteRssiEventTask(uint8_t status, const SLE_Addr_S &addr, int8_t rssi) const;
    void ConnectionUpdateTask(const CM_ConnectUpdateParamRsp_S &param) const;
    bool IsProfileStateReport(const RawAddress &device, RawAddress &reportAddr,
        CdsmService *cdsmService, int newConnState) const;
    void SetSleConnectionModeTask(int32_t connectionMode, int32_t duration);
    void SetSleConnectable();
    void SetSleUnconnectable();
    void UpdateSleConnectableTimer();
    void ClearSleConnectableTimer();
    bool SetAliasNameTask(const RawAddress &device, const std::string &name) const;
    // pair
    bool ConnectionCompleteTaskInner(int pairState, uint16_t lcid,
                                       const RawAddress &peerAddr, const SLE_Addr_S &addr, uint8_t role) const;
    void ReadPeerDeviceInfoFromConf(const std::vector<std::string> &pairedAddrList) const;
    void ClearPeerDeviceInfo() const;
    int SetRpaAddrAndTypeToBtm();
    int SetLocalIrkAndIdentityAddrToBtm() const;
    int InitSlemAndCm();
    void InitSleProfileConnectManager();
    int SetBleRoles() const;
    std::string ReadRemoteDeviceNameByGatt(const RawAddress &addr, int appID) const;
    bool IsPairStateReport(const RawAddress &device, RawAddress &reportAddr,
        CdsmService *cdsmService, int newConnState) const;
    bool RemoveAllPairsTask();
    bool PairRequestReplyTask(const RawAddress &device, bool accept) const;
    bool IsDisconnectedByUser(int acbConnState, int pairState, int reason) const;
    int HandleDisconnAndUnpairedReason(int reason) const;
    bool HandleCdsmServiceConnectionState(
        const RawAddress &device, RawAddress &reportAddr, const SleConnectionChangedParam &connChangedParam) const;
    int HandleConnectionStateReason(const SleConnectionChangedParam &connChangedParam) const;

    // CM ACB Change status callback.
    static void ConnectionUpdateCallback(CM_ConnectUpdateParamRsp_S *param);
    static void ConnectionUpdateRequestCallback(CM_ConnectRemoteUpdateParamReq_S *param);
    static void ConnectionCancelCallback(uint8_t *param);
    static void ReadRemoteRssiCallback(CM_ReadRemoteRssiRsp_S *param);
    static void ReadFeatureVersionCallback(CM_ReadRemoteFeatureVersionRsp_S *param);
    static void SetPhyCallback(CM_SetPhyRsp_S *param);
    void PhyChangedTask(const CM_SetPhyRsp_S &param);
    static void AcbConnectionStateCallback(CM_LogicLinkState_S *param);
    static void FreqBandChanged(CM_FreqBandSwitchParam *param);
    void FreqBandChangedTask(const CM_FreqBandSwitchParam &param);
    static void AcbSubrateChanged(CM_AcbSubrateCbParam_S *param);
    void AcbSubrateChangedTask(const CM_AcbSubrateCbParam_S &param);
    static void AcbSubrateChangeReq(CM_AcbSubrateCbParam_S *param);
    void AcbSubrateChangeReqTask(const CM_AcbSubrateCbParam_S &param);
    static void ReadAcceptFilterListSizeCallback(CM_ReadAcceptFilterListSize_S *param);
    void ReadAcceptFilterListSizeCallbackTask(const CM_ReadAcceptFilterListSize_S &param);
    static void HidCoexModeCallback(CM_HidCoexModeParam_S *param);
    void PowerLevelChangedTask(const PowerLevelInfo &info);
    void RssiChangedCallbackTask(const DisconChipInfo &info);
    // ncb callback
    static void RssiChangedCallback(void *param);
    static void PowerLevelChangedCallback(void *param);
    static std::string GetAddressByConnHandle(uint16_t connHandle);
    static void ReadLocalFeatureCallback(void *param);
    static void ChipResetNotify(void *param);
    // connect acb
    void ProfileConnTimeoutTask(const RawAddress &device);
    void OnAcbStateChanged(const RawAddress &device, const int connectState, int discReason) const;
    int GetAcbStateTask(const RawAddress &device) const;
    bool SetAcbDisConnReasonTask(const std::string &address, int reason) const;
    int GetAcbDisConnReasonTask(const std::string &address) const;
    int GetProfileConnStateTask(const RawAddress &device);
    bool IsAcbConnectedTask(const RawAddress &device) const;
    bool IsAcbEncryptedTask(const RawAddress &device) const;
    uint8_t GetLinkRoleTask(const RawAddress &device) const;

    bool ProcCdsmDeviceConnect(const RawAddress &device);
    void ConnectAllProfileInner(const RawAddress &device);
    void ConnectAllProfileTask(const RawAddress &device);
    bool IsServiceSupportedConn(const RawAddress &device) const;
    void DisconnectAllProfileInner(const RawAddress &device, bool &result, uint8_t discReason);
    bool ProcCdsmDisconnectAllProfile(const RawAddress &device, bool &result, uint8_t discReason);
    void OnAllProfileDisconnected(const RawAddress &device);
    bool Disconnect(const RawAddress &addr);
    void DeleteDeviceInfoFiles() const;
    int GetUnDisconnectedCnt(const RawAddress &device) const;
    int GetConnectedCnt(const RawAddress &device) const;
    std::string GetDeviceNameTask(const RawAddress &device) const;
    std::vector<Uuid> GetDeviceUuidsTask(const RawAddress &device) const;
    bool SetBtAddrBySleAddrTask(const std::string &sleAddr, const std::string &btAddr) const;
    void ClearAllCdsmData(const RawAddress &device) const;
    void SendBgConnList() const;
    void RemoveBgConnDevice(const std::string &delAddr) const override;
    void SendDirectConnList() const;
    void SetSlePeripheralDeviceBasicInfo(const std::string &addr, std::shared_ptr<SlePeripheralDevice> &value) const;
    void SetSlePeripheralDeviceCdsmInfo(const std::string &addr, std::shared_ptr<SlePeripheralDevice> &value) const;
    void SetSlePeripheralDeviceModelInfo(const std::string &addr, std::shared_ptr<SlePeripheralDevice> &value) const;
    void ProcCreateCdsmGroup(const RawAddress &reportAddr, const RawAddress &realAddr, bool eraseDeviceIfNeed) const;
    bool ProcClearOldCdsmGroup(const RawAddress &reportAddr, const RawAddress &collabAddr,
        bool eraseDeviceIfNeed) const;
    bool ProcClearCommonEarphoneOldCdsmGroup(const RawAddress &newReportAddr, const RawAddress &oldReportAddr,
        bool eraseDeviceIfNeed) const;
    void ProcCreateCdsmGroupAndEraseDevice(const RawAddress &reportAddr, const RawAddress &otherAddr) const;
    static void FormatDeviceModelInfo(DeviceModel &model, std::string &newModelId);
    void SendImgSecuConfig(const RawAddress &device);
    void ClearDeviceManufacturerAbility(const RawAddress &device) const;

// 配对请求 start
    class ServiceSsapConnectInst {
    public:
        class ServiceSsapCallback : public InterfaceSsapClientCallback {
        public:
            explicit ServiceSsapCallback(const RawAddress &addr) : device_(addr)
            {}
            ~ServiceSsapCallback()
            {}
 
            void OnConnectionStateChanged(uint8_t state, int ret) override;
            void OnReadPropertiesByUuid(std::list<Property> &list, int ret) override;
        private:
            RawAddress device_;
        };
 
        explicit ServiceSsapConnectInst(const RawAddress &addr)
            : address_(addr), serviceSsapCallback_(std::make_shared<ServiceSsapCallback>(addr))
        {}
 
        ~ServiceSsapConnectInst()
        {}
 
        RawAddress GetAddr()
        {
            return address_;
        }
        void SetAppId(int appId)
        {
            appId_ = appId;
        }
        int GetAppId()
        {
            return appId_;
        }
        std::shared_ptr<ServiceSsapCallback> GetSsapClientCallback()
        {
            return serviceSsapCallback_;
        }
 
    private:
        RawAddress address_;
        int appId_ = -1;
        std::shared_ptr<ServiceSsapCallback> serviceSsapCallback_ = nullptr;
    };
#ifdef NO_PAIRING_DIALOG
    void ProcessPairingRequest(const RawAddress &device, std::string &passKey, const int type) const;
#endif
    void PullUpDialog(const RawAddress &device, std::string &passKey, const int type) const;
    void PassivePairingGetRemoteName(const RawAddress &device, const std::string &passKey, const int type) const;
    void ReadDisDeviceNameByUuid(const RawAddress &device) const;
    void DeregisterServiceSsapApplication() const;
    void OnSsapConnectionStateChangedTask(const RawAddress &device, uint8_t newState);
    void OnSsapReadPropertiesByUuidTask(const RawAddress &device, const std::list<Property> &list, int ret);
    void PassivePairingDialog() const;
// 配对请求 end

    SLE_DISALLOW_COPY_AND_ASSIGN(SleAdapter);
    struct impl;
    std::shared_ptr<impl> pimpl = nullptr;
    SleRemoteDeviceAdapter *adapterProperties_;
};
}  // namespace sle
}  // namespace OHOS

#endif  // SLE_ADAPTER_H
