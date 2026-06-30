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

#include <condition_variable>
#include <memory>
#include <set>

#include "nearlink_sa_manager.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"
#include "nearlink_asc_callback_stub.h"
#include "nearlink_host.h"
#include "nearlink_host_proxy.h"
#include "nearlink_safe_map.h"
#include "nearlink_safe_set.h"
#include "nearlink_ssap_service_parcel.h"
#include "log_util.h"
#include "i_nearlink_asc.h"
#include "nearlink_ASC_source.h"

namespace OHOS {
namespace Nearlink {
#define WPTR_ASC_CBACK(cbWptr, func, ...)          \
do {                                                \
    auto cbSptr = (cbWptr).lock();                  \
    if (cbSptr) {                                   \
        cbSptr->func(__VA_ARGS__);                  \
    } else {                                        \
        HILOGE(#cbWptr ": callback is nullptr");    \
    }                                               \
} while (0)

struct SleAudioStream::impl {
    class NearlinkASCCallbackStubImpl;
    std::weak_ptr<SleAudioStreamObserver> callback_;
    sptr<NearlinkASCCallbackStubImpl> clientCallback_;

    explicit impl();
    ~impl();

    void Init(std::weak_ptr<SleAudioStream> client);
    int32_t profileRegisterId_{0};
};

class SleAudioStream::impl::NearlinkASCCallbackStubImpl : public NearlinkASCCallbackStub {
public:
    explicit NearlinkASCCallbackStubImpl(std::weak_ptr<SleAudioStream> audio) : audio_(audio)
    {
        HILOGI("NearlinkASCCallbackStubImpl");
        uid_ = getuid();
    }

    ~NearlinkASCCallbackStubImpl() override
    {}

    void OnAudioControl(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) override
    {
        AudioStreamType streamType = static_cast<AudioStreamType>(result.GetStreamType());
        int cmd = result.GetCmd();
        int controlResult = result.GetResult();
        HILOGI("cmd: %{public}d streamType: %{public}d result: %{public}d", cmd, streamType, controlResult);
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        if (cmd == NL_SLE_ASC_CONTROL_CMD_START) {
            WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnStartPlayingResult, remotDevice, streamType, controlResult);
        } else if (cmd == NL_SLE_ASC_CONTROL_CMD_STOP) {
            WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnStopPlayingResult, remotDevice, streamType, controlResult);
        } else {
            HILOGE("cmd Error %{public}d", cmd);
        }
    }

    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) override
    {
        HILOGD("[SleAudioStream] OnAddSleAudioDevice %{public}s streamType %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnAddSleAudioDevice, remotDevice, streamType, mediaVolume,
            callVolume);
    }

    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGD("[SleAudioStream] OnDeleteSleAudioDevice %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnDeleteSleAudioDevice, remotDevice);
    }

    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) override
    {
        HILOGI("[SleAudioStream] OnSleAudioDeviceActionChanged %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        std::vector<struct AudioStreamInfo> streamData = {};
        streamInfo.GetStreamState(streamData);
        WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnSleAudioDeviceActionChanged, remotDevice, streamData, action);
    }

    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) override
    {
        HILOGI("[SleAudioStream] OnAddSleVirtualAudioDevice %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnAddSleVirtualAudioDevice, remotDevice, streamType);
    }

    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGI("[SleAudioStream] OnDeleteSleVirtualAudioDevice %{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        std::shared_ptr<SleAudioStream> audioSptr = (audio_).lock();
        if (!audioSptr) {
            HILOGE("callback client is nullptr");
            return;
        }

        NearlinkRemoteDevice remotDevice(device.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        WPTR_ASC_CBACK(audioSptr->pimpl->callback_, OnDeleteSleVirtualAudioDevice, remotDevice);
    }

    uid_t GetUid() override
    {
        return uid_;
    }
private:
    std::weak_ptr<SleAudioStream> audio_;
    uid_t uid_ = 0;
};

void SleAudioStream::impl::Init(std::weak_ptr<SleAudioStream> client)
{
    clientCallback_ = sptr<NearlinkASCCallbackStubImpl>::MakeSptr(client);
    NL_CHECK_RETURN(clientCallback_, "clientCallback_ is nullptr.");
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(PROFILE_ASC);
    NL_CHECK_RETURN(info, "info is nullptr.");

    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkASC> proxy = iface_cast<INearlinkASC>(remote);

        NL_CHECK_RETURN(proxy, "proxy is nullptr.");

        int result = proxy->RegisterApplication(clientCallback_);
        if (result != NL_NO_ERROR) {
            HILOGE("Can not Register to ssap client service! result(%{public}d)", result);
        }
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

SleAudioStream::impl::impl()
{}

SleAudioStream::impl::~impl()
{
    HILOGI("ASC ~impl");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN(proxy != nullptr, "failed: no proxy");
    proxy->DeregisterApplication(clientCallback_);
}

SleAudioStream::SleAudioStream(std::shared_ptr<SleAudioStreamObserver> callback)
{
    HILOGI("create SleAudioStream start.");
    pimpl = std::make_unique<SleAudioStream::impl>();
    if (!pimpl) {
        HILOGE("create SleAudioStream failed.");
        return;
    }

    pimpl->callback_ = callback;
}

SleAudioStream::~SleAudioStream()
{}

std::shared_ptr<SleAudioStream> SleAudioStream::CreateSleAudioStream(std::shared_ptr<SleAudioStreamObserver> callback)
{
    HILOGI("CreateSleAudioStream.");
    auto instance = std::make_shared<SleAudioStream>(Pattern(), callback);
    NL_CHECK_RETURN_RET(instance->pimpl, nullptr, "pimpl is nullptr.");

    HILOGI("instance->pimpl->Init.");
    instance->pimpl->Init(instance);
    return instance;
}

void SleAudioStream::GetSleAudioDeviceList(std::vector<NearlinkRemoteDevice> &devices)
{
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN(proxy, "proxy is nullptr.");

    std::vector<NearlinkRawAddress> rawAddresses;
    NlErrCode status = proxy->GetAudioDeviceList(rawAddresses);
    NL_CHECK_RETURN(status == NL_NO_ERROR, "GetSleAudioDeviceList failed, status(%{public}d).", status);

    for (const NearlinkRawAddress& addr : rawAddresses) {
        devices.emplace_back(NearlinkRemoteDevice(addr.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    }
}

std::map<AudioStreamType, AudioStreamCodecInfo> SleAudioStream::GetSleAudioDeviceCodecInfo(
    const NearlinkRemoteDevice &device)
{
    std::map<AudioStreamType, AudioStreamCodecInfo> info;
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), info, "Invalid remote device.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, info, "proxy is nullptr.");

    NlErrCode status = proxy->GetAudioDeviceCodecInfo(
        static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())), info);
    NL_CHECK_RETURN_RET(status == NL_NO_ERROR, info, "GetAudioDeviceCodecInfo failed, status(%{public}d).", status);
    return info;
}

void SleAudioStream::GetSleVirtualAudioDeviceList(std::vector<NearlinkRemoteDevice> &devices)
{
    HILOGI("GetSleVirtualAudioDeviceList.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN(proxy, "proxy is nullptr.");

    std::vector<NearlinkRawAddress> rawAddresses;
    NlErrCode status = proxy->GetVirtualAudioDeviceList(rawAddresses);
    NL_CHECK_RETURN(status == NL_NO_ERROR, "GetVirtualAudioDeviceList failed, status(%{public}d).", status);

    for (const NearlinkRawAddress& addr : rawAddresses) {
        devices.emplace_back(NearlinkRemoteDevice(addr.GetAddress(),
            static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)));
    }
    return;
}

bool SleAudioStream::IsInBandRingOpen(const NearlinkRemoteDevice &device) const
{
    // 星闪默认是带内铃声
    return true;
}

uint32_t SleAudioStream::GetSupportStreamType(const NearlinkRemoteDevice &device) const
{
    uint32_t supportStreamType = AUDIO_STREAM_NONE;
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), supportStreamType, "Invalid remote device.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, supportStreamType, "proxy is nullptr.");

    NlErrCode result = proxy->GetSupportStreamType(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        supportStreamType);
    if (result != NL_NO_ERROR) {
        HILOGE("GetSupportStreamType failed. result(%{public}d)", result);
    }

    return supportStreamType;
}

NlErrCode SleAudioStream::SetActiveSinkDevice(const NearlinkRemoteDevice &device, uint64_t supportStreamType)
{
    HILOGI("[SleAudioStream] SetActiveSinkDevice %{public}s supportStreamType %{public}ld",
        GetEncryptAddr(device.GetDeviceAddr()).c_str(), supportStreamType);
    bool isSleSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NL_CHECK_RETURN_RET(isSleSupport, NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    bool isSleEnabled = NearlinkHost::GetInstance().IsSleAvailableToCaller();
    NL_CHECK_RETURN_RET(isSleEnabled, NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode result = proxy->SetActiveSinkDevice(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        supportStreamType);
    if (result != NL_NO_ERROR) {
        HILOGE("SetActiveSinkDevice failed. result(%{public}d)", result);
    }
    return result;
}

NlErrCode SleAudioStream::StartPlaying(const NearlinkRemoteDevice &device, AudioStreamType streamType)
{
    HILOGI("[SleAudioStream] StartPlaying %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetDeviceAddr()).c_str(), streamType);
    bool isSleSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NL_CHECK_RETURN_RET(isSleSupport, NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    bool isSleEnabled = NearlinkHost::GetInstance().IsSleAvailableToCaller();
    NL_CHECK_RETURN_RET(isSleEnabled, NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode result = proxy->AudioControl(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        streamType, NL_SLE_ASC_CONTROL_CMD_START);
    if (result != NL_NO_ERROR) {
        HILOGE("AudioControl failed. result(%{public}d)", result);
    }
    return result;
}

NlErrCode SleAudioStream::StopPlaying(const NearlinkRemoteDevice &device, AudioStreamType streamType)
{
    HILOGI("[SleAudioStream] StopPlaying %{public}s streamType %{public}d",
        GetEncryptAddr(device.GetDeviceAddr()).c_str(), streamType);
    bool isSleSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NL_CHECK_RETURN_RET(isSleSupport, NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    bool isSleEnabled = NearlinkHost::GetInstance().IsSleAvailableToCaller();
    NL_CHECK_RETURN_RET(isSleEnabled, NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode result = proxy->AudioControl(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        streamType, NL_SLE_ASC_CONTROL_CMD_STOP);
    if (result != NL_NO_ERROR) {
        HILOGE("AudioControl failed. result(%{public}d)", result);
    }
    return result;
}

bool SleAudioStream::GetDualRecordAbility(const NearlinkRemoteDevice &device)
{
    bool isSupport = false;
    HILOGI("[SleAudioStream] enter %{public}s", GetEncryptAddr(device.GetDeviceAddr()).c_str());
    bool isSleSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NL_CHECK_RETURN_RET(isSleSupport, isSupport, "nearlink is not support.");
    bool isSleEnabled = NearlinkHost::GetInstance().IsSleAvailableToCaller();
    NL_CHECK_RETURN_RET(isSleEnabled, isSupport, "nearlink is off.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, isSupport, "proxy is nullptr.");

    NlErrCode result = proxy->GetDualRecordAbility(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        isSupport);
    HILOGI("[SleAudioStream] dev %{public}s, isSupport:%{public}d",
        GetEncryptAddr(device.GetDeviceAddr()).c_str(), isSupport);
    return isSupport;
}

bool SleAudioStream::GetKaraokeAbility(const NearlinkRemoteDevice &device)
{
    bool isSupport = false;
    HILOGI("[SleAudioStream] enter %{public}s", GetEncryptAddr(device.GetDeviceAddr()).c_str());
    bool isSleSupport = NearlinkHost::GetInstance().IsNearlinkSupport();
    NL_CHECK_RETURN_RET(isSleSupport, isSupport, "nearlink is not support.");
    bool isSleEnabled = NearlinkHost::GetInstance().IsSleAvailableToCaller();
    NL_CHECK_RETURN_RET(isSleEnabled, isSupport, "nearlink is off.");
    sptr<INearlinkASC> proxy = GetProxy<INearlinkASC>(PROFILE_ASC);
    NL_CHECK_RETURN_RET(proxy, isSupport, "proxy is nullptr.");
    NlErrCode result = proxy->GetKaraokeAbility(static_cast<NearlinkRawAddress>(RawAddress(device.GetDeviceAddr())),
        isSupport);
    HILOGI("[SleAudioStream] dev %{public}s, isSupport:%{public}d",
        GetEncryptAddr(device.GetDeviceAddr()).c_str(), isSupport);
    return isSupport;
}
}
}