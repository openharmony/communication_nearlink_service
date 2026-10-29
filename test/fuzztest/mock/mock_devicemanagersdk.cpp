#include "device_manager.h"

namespace OHOS { namespace DistributedHardware {

// fuzz 环境注入:DM 服务在裸进程不可用,全部接口空实现返回失败(调用方按错误分支跳过)
class DmFuzzStub : public DeviceManager {
public:
    int32_t InitDeviceManager(const std::string &pkgName, std::shared_ptr<DmInitCallback> dmInitCallback) override { return -1; }
    int32_t UnInitDeviceManager(const std::string &pkgName) override { return -1; }
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra, std::vector<DmDeviceInfo> &deviceList) override { return -1; }
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra, bool isRefresh, std::vector<DmDeviceInfo> &deviceList) override { return -1; }
    int32_t GetAvailableDeviceList(const std::string &pkgName, std::vector<DmDeviceBasicInfo> &deviceList) override { return -1; }
    int32_t GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo) override { return -1; }
    int32_t GetDeviceInfo(const std::string &pkgName, const std::string networkId, DmDeviceInfo &deviceInfo) override { return -1; }
    int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra, std::shared_ptr<DeviceStateCallback> callback) override { return -1; }
    int32_t RegisterDevStatusCallback(const std::string &pkgName, const std::string &extra, std::shared_ptr<DeviceStatusCallback> callback) override { return -1; }
    int32_t UnRegisterDevStateCallback(const std::string &pkgName) override { return -1; }
    int32_t UnRegisterDevStatusCallback(const std::string &pkgName) override { return -1; }
    int32_t StartDeviceDiscovery(const std::string &pkgName, const DmSubscribeInfo &subscribeInfo, const std::string &extra, std::shared_ptr<DiscoveryCallback> callback) override { return -1; }
    int32_t StartDeviceDiscovery(const std::string &pkgName, uint64_t tokenId, const std::string &filterOptions, std::shared_ptr<DiscoveryCallback> callback) override { return -1; }
    int32_t StopDeviceDiscovery(const std::string &pkgName, uint16_t subscribeId) override { return -1; }
    int32_t StopDeviceDiscovery(uint64_t tokenId, const std::string &pkgName) override { return -1; }
    int32_t PublishDeviceDiscovery(const std::string &pkgName, const DmPublishInfo &publishInfo, std::shared_ptr<PublishCallback> callback) override { return -1; }
    int32_t UnPublishDeviceDiscovery(const std::string &pkgName, int32_t publishId) override { return -1; }
    int32_t AuthenticateDevice(const std::string &pkgName, int32_t authType, const DmDeviceInfo &deviceInfo, const std::string &extra, std::shared_ptr<AuthenticateCallback> callback) override { return -1; }
    int32_t UnAuthenticateDevice(const std::string &pkgName, const DmDeviceInfo &deviceInfo) override { return -1; }
    int32_t VerifyAuthentication(const std::string &pkgName, const std::string &authPara, std::shared_ptr<VerifyAuthCallback> callback) override { return -1; }
    int32_t RegisterDeviceManagerFaCallback(const std::string &pkgName, std::shared_ptr<DeviceManagerUiCallback> callback) override { return -1; }
    int32_t UnRegisterDeviceManagerFaCallback(const std::string &pkgName) override { return -1; }
    int32_t GetFaParam(const std::string &pkgName, DmAuthParam &faParam) override { return -1; }
    int32_t SetUserOperation(const std::string &pkgName, int32_t action, const std::string &params) override { return -1; }
    int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &netWorkId, std::string &udid) override { return -1; }
    int32_t GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId, std::string &uuid) override { return -1; }
    int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra) override { return -1; }
    int32_t UnRegisterDevStateCallback(const std::string &pkgName, const std::string &extra) override { return -1; }
    int32_t RequestCredential(const std::string &pkgName, const std::string &reqJsonStr, std::string &returnJsonStr) override { return -1; }
    int32_t ImportCredential(const std::string &pkgName, const std::string &credentialInfo) override { return -1; }
    int32_t DeleteCredential(const std::string &pkgName, const std::string &deleteInfo) override { return -1; }
    int32_t RegisterCredentialCallback(const std::string &pkgName, std::shared_ptr<CredentialCallback> callback) override { return -1; }
    int32_t UnRegisterCredentialCallback(const std::string &pkgName) override { return -1; }
    int32_t NotifyEvent(const std::string &pkgName, const int32_t eventId, const std::string &event) override { return -1; }
    int32_t GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &uuid) override { return -1; }
    int32_t GenerateEncryptedUuid(const std::string &pkgName, const std::string &uuid, const std::string &appId, std::string &encryptedUuid) override { return -1; }
    int32_t CheckAPIAccessPermission() override { return -1; }
    int32_t GetLocalDeviceNetWorkId(const std::string &pkgName, std::string &networkId) override { return -1; }
    int32_t GetLocalDeviceId(const std::string &pkgName, std::string &networkId) override { return -1; }
    int32_t GetLocalDeviceName(const std::string &pkgName, std::string &deviceName) override { return -1; }
    int32_t GetLocalDeviceName(std::string &deviceName) override { return -1; }
    int32_t GetLocalDeviceType(const std::string &pkgName, int32_t &deviceType) override { return -1; }
    int32_t GetDeviceName(const std::string &pkgName, const std::string &networkId, std::string &deviceName) override { return -1; }
    int32_t GetDeviceType(const std::string &pkgName, const std::string &networkId, int32_t &deviceType) override { return -1; }
    int32_t BindDevice(const std::string &pkgName, int32_t bindType, const std::string &deviceId, const std::string &extra, std::shared_ptr<AuthenticateCallback> callback) override { return -1; }
    int32_t UnBindDevice(const std::string &pkgName, const std::string &deviceId) override { return -1; }
    int32_t UnBindDevice(const std::string &pkgName, const std::string &deviceId, const std::string &extra) override { return -1; }
    int32_t CheckNewAPIAccessPermission() override { return -1; }
    int32_t GetNetworkTypeByNetworkId(const std::string &pkgName, const std::string &netWorkId, int32_t &netWorkType) override { return -1; }
    int32_t GetOsTypeByNetworkId(const std::string &pkgName, const std::string &networkId, int32_t &osType) override { return -1; }
    int32_t ImportAuthCode(const std::string &pkgName, const std::string &authCode) override { return -1; }
    int32_t ExportAuthCode(std::string &authCode) override { return -1; }
    int32_t StartDiscovering(const std::string &pkgName, std::map<std::string, std::string> &discoverParam, const std::map<std::string, std::string> &filterOptions, std::shared_ptr<DiscoveryCallback> callback) override { return -1; }
    int32_t StopDiscovering(const std::string &pkgName, std::map<std::string, std::string> &discoverParam) override { return -1; }
    int32_t RegisterDiscoveryCallback(const std::string &pkgName, std::map<std::string, std::string> &discoverParam, const std::map<std::string, std::string> &filterOptions, std::shared_ptr<DiscoveryCallback> callback) override { return -1; }
    int32_t UnRegisterDiscoveryCallback(const std::string &pkgName) override { return -1; }
    int32_t StartAdvertising(const std::string &pkgName, std::map<std::string, std::string> &advertiseParam, std::shared_ptr<PublishCallback> callback) override { return -1; }
    int32_t StopAdvertising(const std::string &pkgName, std::map<std::string, std::string> &advertiseParam) override { return -1; }
    int32_t BindTarget(const std::string &pkgName, const PeerTargetId &targetId, std::map<std::string, std::string> &bindParam, std::shared_ptr<BindTargetCallback> callback) override { return -1; }
    int32_t UnbindTarget(const std::string &pkgName, const PeerTargetId &targetId, std::map<std::string, std::string> &unbindParam, std::shared_ptr<UnbindTargetCallback> callback) override { return -1; }
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::map<std::string, std::string> &filterOptions, bool isRefresh, std::vector<DmDeviceInfo> &deviceList) override { return -1; }
    int32_t RegisterDevStateCallback(const std::string &pkgName, const std::map<std::string, std::string> &extraParam, std::shared_ptr<DeviceStateCallback> callback) override { return -1; }
    int32_t CheckAccessToTarget(uint64_t tokenId, const std::string &targetId) override { return -1; }
    int32_t RegisterPinHolderCallback(const std::string &pkgName, std::shared_ptr<PinHolderCallback> callback) override { return -1; }
    int32_t CreatePinHolder(const std::string &pkgName, const PeerTargetId &targetId, DmPinType pinType, const std::string &payload) override { return -1; }
    int32_t DestroyPinHolder(const std::string &pkgName, const PeerTargetId &targetId, DmPinType pinType, const std::string &payload) override { return -1; }
    int32_t RequestCredential(const std::string &pkgName, std::string &returnJsonStr) override { return -1; }
    int32_t CheckCredential(const std::string &pkgName, const std::string &reqJsonStr, std::string &returnJsonStr) override { return -1; }
    int32_t ImportCredential(const std::string &pkgName, const std::string &reqJsonStr, std::string &returnJsonStr) override { return -1; }
    int32_t DeleteCredential(const std::string &pkgName, const std::string &reqJsonStr, std::string &returnJsonStr) override { return -1; }
    int32_t DpAclAdd(const int64_t accessControlId, const std::string &udid, const int32_t bindType) override { return -1; }
    int32_t GetDeviceSecurityLevel(const std::string &pkgName, const std::string &networkId, int32_t &securityLevel) override { return -1; }
    bool IsSameAccount(const std::string &netWorkId) override { return -1; }
    bool CheckAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
    bool CheckIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
    int32_t GetErrCode(int32_t errCode) override { return -1; }
    int32_t ShiftLNNGear(const std::string &pkgName) override { return -1; }
    int32_t RegDevTrustChangeCallback(const std::string &pkgName, std::shared_ptr<DevTrustChangeCallback> callback) override { return -1; }
    int32_t RegisterDeviceScreenStatusCallback(const std::string &pkgName, std::shared_ptr<DeviceScreenStatusCallback> callback) override { return -1; }
    int32_t UnRegisterDeviceScreenStatusCallback(const std::string &pkgName) override { return -1; }
    int32_t GetDeviceScreenStatus(const std::string &pkgName, const std::string &networkId, int32_t &screenStatus) override { return -1; }
    int32_t SetDnPolicy(const std::string &pkgName, std::map<std::string, std::string> &policy) override { return -1; }
    int32_t StopAuthenticateDevice(const std::string &pkgName) override { return -1; }
    int32_t GetNetworkIdByUdid(const std::string &pkgName, const std::string &udid, std::string &networkId) override { return -1; }
    int32_t RegisterCredentialAuthStatusCallback(const std::string &pkgName, std::shared_ptr<CredentialAuthStatusCallback> callback) override { return -1; }
    int32_t UnRegisterCredentialAuthStatusCallback(const std::string &pkgName) override { return -1; }
    int32_t RegisterSinkBindCallback(const std::string &pkgName, std::shared_ptr<BindTargetCallback> callback) override { return -1; }
    int32_t UnRegisterSinkBindCallback(const std::string &pkgName) override { return -1; }
    int32_t GetAllTrustedDeviceList(const std::string &pkgName, const std::string &extra, std::vector<DmDeviceInfo> &deviceList) override { return -1; }
    int32_t RegisterAuthenticationType(const std::string &pkgName, const std::map<std::string, std::string> &authParam) override { return -1; }
    int32_t GetDeviceProfileInfoList(const std::string &pkgName, const DmDeviceProfileInfoFilterOptions &filterOptions, std::shared_ptr<GetDeviceProfileInfoListCallback> callback) override { return -1; }
    int32_t GetDeviceIconInfo(const std::string &pkgName, const DmDeviceIconInfoFilterOptions &filterOptions, std::shared_ptr<GetDeviceIconInfoCallback> callback) override { return -1; }
    int32_t PutDeviceProfileInfoList(const std::string &pkgName, const std::vector<OHOS::DistributedHardware::DmDeviceProfileInfo> &deviceProfileInfoList) override { return -1; }
    int32_t GetLocalDisplayDeviceName(const std::string &pkgName, int32_t maxNameLength, std::string &displayName) override { return -1; }
    int32_t GetDeviceNetworkIdList(const std::string &bundleName, const NetworkIdQueryFilter &queryFilter, std::vector<std::string> &networkIds) override { return -1; }
    int32_t SetLocalDeviceName(const std::string &pkgName, const std::string &deviceName, std::shared_ptr<SetLocalDeviceNameCallback> callback) override { return -1; }
    int32_t SetRemoteDeviceName(const std::string &pkgName, const std::string &deviceId, const std::string &deviceName, std::shared_ptr<SetRemoteDeviceNameCallback> callback) override { return -1; }
    int32_t RestoreLocalDeviceName(const std::string &pkgName) override { return -1; }
    int32_t RegisterLocalServiceInfo(const DMLocalServiceInfo &info) override { return -1; }
    int32_t UnRegisterLocalServiceInfo(const std::string &bundleName, int32_t pinExchangeType) override { return -1; }
    int32_t UpdateLocalServiceInfo(const DMLocalServiceInfo &info) override { return -1; }
    int32_t GetLocalServiceInfoByBundleNameAndPinExchangeType(const std::string &bundleName, int32_t pinExchangeType, DMLocalServiceInfo &info) override { return -1; }
    int32_t UnRegisterPinHolderCallback(const std::string &pkgName) override { return -1; }
    bool CheckSrcAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
    bool CheckSinkAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
    bool CheckSrcIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
    bool CheckSinkIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee) override { return -1; }
};

DeviceManager::~DeviceManager() {}

DeviceManager& DeviceManager::GetInstance()
{
    static DmFuzzStub instance;
    return instance;
}

} } // namespaces
