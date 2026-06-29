/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "HidHostService.h"
#include "ClassCreator.h"
#include "log_util.h"
#include "SleUtils.h"
#include "nearlink_dft_exception.h"
#include "nearlink_common_event_helper.h"
#include "ThreadUtil.h"
#include "HidHostUhid.h"
#include "HidHostStackAdapter.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceManager.h"

namespace OHOS {
namespace Nearlink {
namespace {
enum {
    HID_DEVICE_VENDOR_ID = 0,
    HID_DEVICE_PROUCT_ID = 1,
    HID_DEVICE_VERSION = 2,
};
constexpr uint8_t HID_MAX_CONNECTION_NUM = 6;
}

struct HidHostService::impl {
    impl() = default;
    ~impl() = default;
    // service status
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
    // service status
    std::atomic_bool isShuttingDown_ = ATOMIC_FLAG_INIT;
    HidHostStackAdapter stackAdapter_;
    BaseObserverList<HidHostObserver> hidHostObservers_ {};
    NearlinkSafeMap<RawAddress, std::shared_ptr<HidHostUhid>> uhids_ {};
};

HidHostService::HidHostService() : utility::Context(PROFILE_NAME_HID_HOST, "1.0.0"),
    pimpl(std::make_unique<HidHostService::impl>())
{
    LOG_DEBUG("[HIDH Service]%{public}s:%{public}s Create", PROFILE_NAME_HID_HOST.c_str(), Name().c_str());
    // 客户端向协议栈注册回调
    if (pimpl->stackAdapter_.RegisterCallbackToStack() != HID_HOST_SUCCESS) {
        HILOGE("[HIDH Service] register cb to stack failed.");
    }
}

HidHostService::~HidHostService()
{
    LOG_DEBUG("[HIDH Service]%{public}s:%{public}s Destroy", PROFILE_NAME_MAP_MSE.c_str(), Name().c_str());
}

utility::Context *HidHostService::GetContext()
{
    return this;
}

HidHostService *HidHostService::GetService()
{
    return static_cast<HidHostService *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_HID_HOST));
}

void HidHostService::RegisterObserver(HidHostObserver &hidHostObserver)
{
    LOG_INFO("[HIDH Service] Enter");
    pimpl->hidHostObservers_.Register(hidHostObserver);
}

void HidHostService::DeregisterObserver(HidHostObserver &hidHostObserver)
{
    LOG_DEBUG("[HIDH Service] Enter");
    pimpl->hidHostObservers_.Deregister(hidHostObserver);
}

void HidHostService::NotifyStateChanged(const RawAddress &device, int state, int preState, int32_t ret)
{
    LOG_DEBUG("[HIDH Service], state:%{public}d, preState:%{public}d", state, preState);
    ProcessConnectDftEvent(device, state, preState, ret);
    DoInHidThread([this, device, state, preState]() -> void {
        HILOGI("[HIDH Service], curState(%{public}d), preState(%{public}d)", state, preState);
        if (state == static_cast<int>(SleConnectState::CONNECTED)) {
            ProcessConnectedEvent(device);
        } else if (state == static_cast<int>(SleConnectState::DISCONNECTED)) {
            ProcessDisconnectedEvent(device);
        }

        pimpl->hidHostObservers_.ForEach([device, state, preState](HidHostObserver &observer) {
            observer.OnConnectionStateChanged(device, state, preState);
        });
        NearlinkHelper::NearlinkCommonEventHelper::PublishHidConnectionStateEvent(
            device.GetAddress(), state);
    });
}

void HidHostService::Enable(void)
{
    LOG_DEBUG("[HIDH Service] Enter");
    DoInHidThread([this]() -> void {
        StartUp();
    });
}

void HidHostService::Disable(void)
{
    LOG_DEBUG("[HIDH Service] Enter");
    DoInHidThread([this]() -> void {
        ShutDown();
    });
}

void HidHostService::StartUp()
{
    LOG_DEBUG("[HIDH Service]:==========<start>==========");
    if (pimpl->isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_HID_HOST, true);
        LOG_WARN("[HIDH Service]:HidHostService has already been started before.");
        return;
    }

    GetContext()->OnEnable(PROFILE_NAME_HID_HOST, true);
    pimpl->isStarted_.store(true);
    LOG_DEBUG("[HIDH Service]:HidHostService started");
}

void HidHostService::ShutDown()
{
    LOG_DEBUG("[HIDH Service]:==========<start>==========");
    if (!pimpl->isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_HID_HOST, true);
        LOG_WARN("[HIDH Service]:HidHostService has already been shutdown before.");
        return;
    }

    pimpl->isShuttingDown_.store(true);

    std::list<RawAddress> connectedDevices = GetConnectDevices();
    pimpl->uhids_.Iterate(
        [this](const RawAddress addr, std::shared_ptr<HidHostUhid> &uhid) -> void {
            int state = GetDeviceState(addr);
            if (state == static_cast<int>(SleConnectState::CONNECTING) ||
                state == static_cast<int>(SleConnectState::CONNECTED)) {
                pimpl->stackAdapter_.Disconnect(addr);
            }
            if (uhid == nullptr) {
                return;
            }
            uhid->Close();
            uhid->Destroy();
        }
    );
    ShutDownDone(true);
}

void HidHostService::ShutDownDone(bool isAllDisconnected)
{
    LOG_INFO("[HIDH Service] isAllDisconnected=%{public}d, ==========<start>==========", isAllDisconnected);
    if (!isAllDisconnected) {
        bool ret = pimpl->uhids_.Find(
            [this](const RawAddress addr, std::shared_ptr<HidHostUhid> &uhid) -> bool {
                int state = GetDeviceState(addr);
                return state != static_cast<int>(SleConnectState::DISCONNECTED);
            }
        );
        if (ret) {
            return;
        }
    }

    pimpl->uhids_.Clear();

    GetContext()->OnDisable(PROFILE_NAME_HID_HOST, true);
    pimpl->isStarted_.store(false);
    pimpl->isShuttingDown_.store(false);
    LOG_DEBUG("[HIDH Service]:HidHostService shutdown");
}

void HidHostService::ConnectHidInterface(const RawAddress &device)
{
    LOG_INFO("[HIDH Service] Enter");
    if (pimpl->isShuttingDown_.load()) {
        LOG_INFO("[HIDH Service]:HidHostService is shutting down");
        return;
    }
    if (GetConnectionsDeviceNum() >= HID_MAX_CONNECTION_NUM) {
        LOG_ERROR("[HIDH Service]:Max connection number has reached!");
        return;
    }


    DftCacheHidStart(device.GetAddress());
    pimpl->stackAdapter_.Connect(device);

    std::shared_ptr<HidHostUhid> uhid = nullptr;
    bool ret = pimpl->uhids_.GetValue(device, uhid);
    if (ret == false || uhid == nullptr) {
        uhid = std::make_shared<HidHostUhid>(device.GetAddress());
        pimpl->uhids_.EnsureInsert(device, uhid);
    }
}

int HidHostService::SendData(const HidReportInfo &reportInfo)
{
    NL_CHECK_RETURN_RET(reportInfo.dataLength_ != 0 && reportInfo.data_ != nullptr, HID_HOST_FAILURE,
        "Error : checkReportInfo error!");
    DoInHidThread([this, reportInfo]() -> void {
        uint8_t reportId = reportInfo.reportId_;
        uint16_t length = reportInfo.dataLength_;
        uint16_t offset = 0;
        std::unique_ptr<uint8_t[]> data = nullptr;
        if (reportId != 0) {
            offset++;
            data = std::make_unique<uint8_t[]>(length + offset);
            data[0] = reportId;
        } else {
            data = std::make_unique<uint8_t[]>(length);
        }
        if (memcpy_s(data.get() + offset, length, reportInfo.data_.get(), length) != EOK) {
            LOG_ERROR("[HIDH Service] memcpy error");
            return;
        }
        std::shared_ptr<HidHostUhid> uhid = nullptr;
        bool ret = pimpl->uhids_.GetValue(reportInfo.dev_, uhid);
        NL_CHECK_RETURN(ret && uhid != nullptr, "[HIDH Service] device: %{public}s uhid is nullptr",
            GET_ENCRYPT_ADDR(reportInfo.dev_));
        uhid->SendData(data.get(), length + offset);
    });
    return HID_HOST_SUCCESS;
}

int HidHostService::Connect(const RawAddress &device)
{
    DoInHidThread([this, device]() -> void {
        ConnectHidInterface(device);
    });
    return HID_HOST_SUCCESS;
}

int HidHostService::Disconnect(const RawAddress &device)
{
    LOG_DEBUG("[HIDH Service] Enter");
    DoInHidThread([this, device]() -> void {
        HILOGI("[HIDH Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->stackAdapter_.Disconnect(device);
    });
    return HID_HOST_SUCCESS;
}

int HidHostService::HidHostVCUnplug(std::string device, uint8_t id, uint16_t size, uint8_t type)
{
    return HID_HOST_SUCCESS;
}

int HidHostService::HidHostSendReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report)
{
    LOG_INFO("[HIDH Service]:start ");
    HidReportInfo reportInfo = {};
    reportInfo.dev_ = RawAddress(device);
    reportInfo.reportType_ = type;
    reportInfo.reportId_ = 0;
    if ((size > 0) && (report != nullptr)) {
        reportInfo.dataLength_ = size;
        reportInfo.data_ = std::make_unique<uint8_t[]>(size);
        if (memcpy_s(reportInfo.data_.get(), size, report, size) != EOK) {
            LOG_ERROR("[HIDH Service] memcpy error");
            return HID_HOST_FAILURE;
        }
    }
    DoInHidThread([this, reportInfo]() -> void {
        pimpl->stackAdapter_.SendReport(reportInfo);
    });
    return HID_HOST_SUCCESS;
}

int HidHostService::HidHostSendReport(std::string device, uint8_t type, uint16_t size, std::string &report)
{
    LOG_INFO("[HIDH Service]:start ");
    size_t len = report.size() / 2;
    if (len <= 0) {
        LOG_ERROR("[HIDH Service]: size is invalid!");
        return HID_HOST_FAILURE;
    }
    uint8_t hexbuf[len];
    NL_CHECK_RETURN_RET(SleUtils::ConvertHexStringToInt(report, hexbuf, len), HID_HOST_FAILURE,
        "[HIDH Service]: string is invalid!");

    return HidHostSendReport(device, type, len, hexbuf);
}

int HidHostService::HidHostSetReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report)
{
    HidReportInfo reportInfo = {};
    reportInfo.dev_ = RawAddress(device);
    reportInfo.reportType_ = type;
    reportInfo.reportId_ = 0;
    if ((size > 0) && (report != nullptr)) {
        reportInfo.dataLength_ = size;
        reportInfo.data_ = std::make_unique<uint8_t[]>(size);
        if (memcpy_s(reportInfo.data_.get(), size, report, size) != EOK) {
            LOG_ERROR("[HIDH Service] memcpy error");
            return HID_HOST_FAILURE;
        }
    }
    DoInHidThread([this, reportInfo]() -> void {
        if (GetDeviceState(reportInfo.dev_) != static_cast<int>(SleConnectState::CONNECTED)) {
            ReceiveHandShake(reportInfo.dev_, HID_HOST_HANDSHAKE_ERROR);
            LOG_ERROR("[HIDH Service] device not connected");
            return;
        }
        if (reportInfo.dataLength_ <= 1) {
            LOG_ERROR("[HIDH Service] data length is error");
            ReceiveHandShake(reportInfo.dev_, HID_HOST_HANDSHAKE_ERROR);
            return;
        }
        pimpl->stackAdapter_.SendReport(reportInfo);
    });
    return HID_HOST_SUCCESS;
}

int HidHostService::HidHostGetReport(std::string device, uint8_t id, uint16_t size, uint8_t type)
{
    HidReportInfo reportInfo = {};
    reportInfo.dev_ = RawAddress(device);
    reportInfo.reportType_ = type;
    reportInfo.reportId_ = id;
    DoInHidThread([this, reportInfo]() -> void {
        if (GetDeviceState(reportInfo.dev_) != static_cast<int>(SleConnectState::CONNECTED)) {
            ReceiveHandShake(reportInfo.dev_, HID_HOST_HANDSHAKE_ERROR);
            LOG_ERROR("[HIDH Service] device not connected");
            return;
        }
        pimpl->stackAdapter_.SendGetReport(reportInfo);
    });
    return HID_HOST_SUCCESS;
}

std::list<RawAddress> HidHostService::GetDevicesByStates(std::vector<int> states)
{
    std::list<RawAddress> devices;
    LOG_INFO("[HIDH Service] devices size(%{public}u)", devices.size());
    return devices;
}

int HidHostService::GetDeviceState(const RawAddress &device)
{
    return pimpl->stackAdapter_.GetDeviceState(device);
}

std::list<RawAddress> HidHostService::GetConnectDevices(void)
{
    std::list<RawAddress> devices = pimpl->stackAdapter_.GetConnectDevices();
    LOG_INFO("[HIDH Service] devices size(%{public}u)", devices.size());
    return devices;
}

int HidHostService::GetConnectState(void)
{
    return static_cast<int>(SleConnectState::DISCONNECTED);
}

uint8_t HidHostService::GetConnectionsDeviceNum()
{
    size_t size = pimpl->stackAdapter_.GetConnectDevices().size();
    LOG_INFO("[HIDH Service] size(%{public}zu)", size);
    return static_cast<uint8_t>(size);
}

void HidHostService::ProcessConnectedEvent(const RawAddress &device)
{
    std::shared_ptr<HidHostUhid> uhid = nullptr;
    bool ret = pimpl->uhids_.GetValue(device, uhid);
    NL_CHECK_RETURN(ret && uhid != nullptr, "[HIDH Service] device: %{public}s uhid is nullptr",
        GET_ENCRYPT_ADDR(device));
    uhid->Open();
    DftCacheHidFinish(device.GetAddress(), HID_HOST_SUCCESS);
    HidInformation hidInf = pimpl->stackAdapter_.GetRemoteHidInfo(device);
    HILOGI("ctryCode[%{public}hhu], descLength[%{public}hu]", hidInf.ctryCode, hidInf.descLength);
    uhid->SendHidInfo(hidInf);
}

void HidHostService::ProcessDisconnectedEvent(const RawAddress &device)
{
    std::shared_ptr<HidHostUhid> uhid = nullptr;
    bool ret = pimpl->uhids_.GetValue(device, uhid);
    NL_CHECK_RETURN(ret && uhid != nullptr, "[HIDH Service] device: %{public}s uhid is nullptr",
        GET_ENCRYPT_ADDR(device));
    uhid->Close();
    uhid->Destroy();
    pimpl->uhids_.Erase(device);
}

void HidHostService::ProcessConnectDftEvent(const RawAddress &device, int state, int preState, int32_t ret)
{
    DftCacheHidState(device.GetAddress(), state);
    if (preState == static_cast<int>(SleConnectState::CONNECTING) &&
        state == static_cast<int>(SleConnectState::DISCONNECTED) && ret != 0) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_HID, ret);
    }
    if (state == static_cast<int>(SleConnectState::CONNECTING)) {
        DftCachePeerInfoTime(device.GetAddress(), PEER_HID_CONN_TIME);
    } else if (state == static_cast<int>(SleConnectState::CONNECTED)) {
        DftCacheConnInfoTime(device.GetAddress(), HID_COMP_TIME);
    }
}

int HidHostService::GetHidDeviceInfo(const RawAddress &device, int infoType)
{
    SleInterfaceAdapter *adapterInterfaceSle =
        (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN_RET(adapterInterfaceSle, 0, "[HIDH Service] adapterInterfaceSle is nullptr.");
    int res = 0;
    switch (infoType) {
        case HID_DEVICE_VENDOR_ID:
            res = adapterInterfaceSle->GetDeviceVendorId(device);
            break;
        case HID_DEVICE_PROUCT_ID:
            res = adapterInterfaceSle->GetDeviceProductId(device);
            break;
        case HID_DEVICE_VERSION:
            res = adapterInterfaceSle->GetDeviceVersion(device);
            break;
        default:
            HILOGE("Invalid infoType %{public}d", infoType);
            break;
    }
    LOG_INFO("[HIDH Service] GetHidDeviceInfo:address[%{public}s], infoType[%{public}d], ret[%{public}d]",
        GetEncryptAddr(device.GetAddress()).c_str(), infoType, res);
    return res;
}

void HidHostService::ReceiveHandShake(const RawAddress &addr, uint16_t err)
{
    DoInHidThread([this, addr, err]() -> void {
        std::shared_ptr<HidHostUhid> uhid = nullptr;
        bool ret = pimpl->uhids_.GetValue(addr, uhid);
        NL_CHECK_RETURN(ret && uhid != nullptr, "[HIDH Service] device: %{public}s uhid is nullptr",
            GET_ENCRYPT_ADDR(addr));
        uhid->SendHandshake(err);
    });
}

int HidHostService::ReceiveControlData(const HidReportInfo &reportInfo)
{
    DoInHidThread([this, reportInfo]() -> void {
        if (reportInfo.dataLength_ > 0) {
            // Add report id as the first byte of the report before sending it to uhid
            uint16_t dataLength = reportInfo.dataLength_;
            uint16_t reportIdOffset = 1;
            std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(dataLength + reportIdOffset);
            data[0] = reportInfo.reportId_;
            if (memcpy_s(data.get() + 1, dataLength, reportInfo.data_.get(), dataLength) != EOK) {
                LOG_ERROR("[HIDH Service] memcpy error");
                return;
            }
            std::shared_ptr<HidHostUhid> uhid = nullptr;
            bool ret = pimpl->uhids_.GetValue(reportInfo.dev_, uhid);
            NL_CHECK_RETURN(ret && uhid != nullptr, "[HIDH Service] device: %{public}s uhid is nullptr",
                GET_ENCRYPT_ADDR(reportInfo.dev_));
            uhid->SendControlData(data.get(), dataLength + reportIdOffset);
        } else {
            LOG_ERROR("[HIDH Service] value is empty");
            ReceiveHandShake(reportInfo.dev_, HID_HOST_HANDSHAKE_ERROR);
        }
    });
    return HID_HOST_SUCCESS;
}

REGISTER_CLASS_CREATOR(HidHostService);
} // namespace Sle
} // namespace OHOS
