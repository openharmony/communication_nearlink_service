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

#include <list>
#include <mutex>
#include <thread>

#include "nearlink_errorcode.h"
#include "log.h"
#include "ssap_data.h"
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceProfileManager.h"
#include "nearlink_utils_server.h"
#include "SleInterfaceManager.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"
#include "nearlink_permission_manager.h"
#include "nearlink_uuid.h"
#include "nearlink_device_manager.h"
#include "nearlink_asc_server.h"
#include "remote_observer_list.h"
#include "nearlink_remote_container.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkASCServer::impl {
    /* 开关的监听 */
    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    /* 合作集Service的监听 */
    class ASCCallbackImpl;
    std::shared_ptr<InterfaceASCCallback> ascClientCallback_ = nullptr;

    /* 外部注册的监听 */
    RemoteObserverList<INearlinkASCCallback> remoteObservers_;
    /* 外部的注册信息 */
    class AscClientRemoteInfo;
    class AscClientRemoteContainer;
    // shared_ptr for deathRecipient of container
    std::shared_ptr<AscClientRemoteContainer> remoteContainer_ = std::make_shared<AscClientRemoteContainer>();

    impl();
    ~impl();

    ProfileASC *GetServicePtr()
    {
        return static_cast<ProfileASC *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    }
};

class NearlinkASCServer::impl::AscClientRemoteInfo {
public:
    AscClientRemoteInfo() : uid_(0)  {}

    explicit AscClientRemoteInfo(const uid_t uid) : uid_(uid) {}

    uid_t uid_ = 0;
};

class NearlinkASCServer::impl::AscClientRemoteContainer final
    : public NearlinkRemoteContainer<AscClientRemoteInfo> {
public:
    ~AscClientRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGW("enter OnRemoteDied");
        AscClientRemoteInfo info = RetrieveRemoteInfo(remote);
        // 如果是音频框架进程dead，触发停播
        if (info.uid_ == AUDIO_SERVER_UID) {
            ProfileASC *audioService = static_cast<ProfileASC *>(
                SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
            NL_CHECK_RETURN(audioService, "audioService invalid.");
            audioService->StopSink();
        }

        DeleteRemoteInfo(remote);
    }
};

class NearlinkASCServer::impl::SystemStateObserver final : public ISystemStateObserver {
public:
    explicit SystemStateObserver(impl *impl) : impl_(impl) {}
    ~SystemStateObserver() override = default;

    void OnSystemStateChange(const SleSystemState state) override
    {
        HILOGI("[NearlinkASCServer] system state = %{public}d", state);
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        switch (state) {
            case SleSystemState::ON: {
                ProfileASC *audioService = impl_->GetServicePtr();
                NL_CHECK_RETURN(audioService, "[NearlinkASCServer]audioService invalid.");
                NL_CHECK_RETURN(impl_->ascClientCallback_, "[NearlinkASCServer]ascClientCallback invalid.");
                audioService->RegisterApplication(impl_->ascClientCallback_);
                break;
            }
            default:
                break;
        }
    }

private:
    impl *impl_ = nullptr;
};

class NearlinkASCServer::impl::ASCCallbackImpl : public InterfaceASCCallback {
public:
    explicit ASCCallbackImpl(impl *impl) : impl_(impl)
    {
        HILOGI("[NearlinkASCServer]Enter");
    }

    ~ASCCallbackImpl() override = default;

    void OnAudioControlComplete(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) override
    {
        HILOGI("NearlinkASCServer OnAudioControlComplete ret: %{public}d", result.GetResult());
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device, &result](sptr<INearlinkASCCallback> observer) {
            observer->OnAudioControl(device, result);
        });
    }

    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) override
    {
        HILOGD("NearlinkASCServer OnAddSleAudioDevice %{public}s streamType %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device, &streamType, &mediaVolume, &callVolume](
            sptr<INearlinkASCCallback> observer) {
            observer->OnAddSleAudioDevice(device, streamType, mediaVolume, callVolume);
        });
    }

    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGD("NearlinkASCServer OnDeleteSleAudioDevice");
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device](sptr<INearlinkASCCallback> observer) {
            observer->OnDeleteSleAudioDevice(device);
        });
    }

    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) override
    {
        HILOGD("NearlinkASCServer OnSleAudioDeviceActionChanged");
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device, &streamInfo, &action](sptr<INearlinkASCCallback> observer) {
            observer->OnSleAudioDeviceActionChanged(device, streamInfo, action);
        });
    }

    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) override
    {
        HILOGI("NearlinkASCServer OnAddSleVirtualAudioDevice");
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device, &streamType](sptr<INearlinkASCCallback> observer) {
            observer->OnAddSleVirtualAudioDevice(device, streamType);
        });
    }

    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGI("NearlinkASCServer OnDeleteSleVirtualAudioDevice");
        NL_CHECK_RETURN(impl_, "[NearlinkASCServer]impl invalid.");
        impl_->remoteObservers_.ForEach([this, &device](sptr<INearlinkASCCallback> observer) {
            observer->OnDeleteSleVirtualAudioDevice(device);
        });
    }

private:
    impl *impl_ = nullptr;
};

NearlinkASCServer::impl::impl()
{
    HILOGI("[NearlinkASCServer]Enter impl");

    ascClientCallback_ = std::make_shared<ASCCallbackImpl>(this);

    ProfileASC *ascService = GetServicePtr();
    NL_CHECK_RETURN(ascService, "[NearlinkASCServer]ascService invalid.");
    ascService->RegisterApplication(ascClientCallback_);

    systemStateObserver_ = std::make_unique<SystemStateObserver>(this);
    NL_CHECK_RETURN(systemStateObserver_, "[NearlinkASCServer]make systemStateObserver invalid.");
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*systemStateObserver_);

    remoteContainer_->Init();
}

NearlinkASCServer::impl::~impl()
{
    HILOGI("[NearlinkASCServer]Enter");
}

NearlinkASCServer::NearlinkASCServer() : pimpl(new impl())
{
    HILOGI("enter");
}

NearlinkASCServer::~NearlinkASCServer()
{}

NlErrCode NearlinkASCServer::RegisterApplication(const sptr<INearlinkASCCallback> &callback)
{
    HILOGI("RegisterApplication");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is null.");
    NL_CHECK_RETURN_RET(pimpl->remoteObservers_.Size() < MAX_OBSERVER_SIZE, NL_ERR_INTERNAL_ERROR, "exceeds range");

    pimpl->remoteObservers_.Register(callback);

    impl::AscClientRemoteInfo info(callback->GetUid());
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);

    HILOGI("[NearlinkASCServer]Register Callback success");
    return NL_NO_ERROR;
}

NlErrCode NearlinkASCServer::DeregisterApplication(const sptr<INearlinkASCCallback> &callback)
{
    HILOGI("[NearlinkASCServer]Enter");
    if (callback == nullptr || pimpl->remoteContainer_ == nullptr) {
        HILOGE("observer or remoteContainer is nullptr!");
        return NL_ERR_INVALID_PARAM;
    }
    pimpl->remoteObservers_.Deregister(callback);
    pimpl->remoteContainer_->DeleteRemoteInfo(callback->AsObject());
    HILOGI("[NearlinkASCServer]DeregisterApplication success");
    return NL_NO_ERROR;
}

NlErrCode NearlinkASCServer::AudioControl(const NearlinkRawAddress &device, AudioStreamType streamType, int cmd)
{
    HILOGI("cmd: %{public}d", cmd);
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->AudioControl(device, streamType, cmd);
    HILOGI("AudioControl result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices)
{
    HILOGI("GetAudioDeviceList");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->GetAudioDeviceList(devices);
    HILOGI("GetAudioDeviceList result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices)
{
    HILOGI("GetVirtualAudioDeviceList");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->GetVirtualAudioDeviceList(devices);
    HILOGI("GetVirtualAudioDeviceList result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType)
{
    HILOGI("GetSupportStreamType");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->GetSupportStreamType(device, supportStreamType);
    HILOGI("GetSupportStreamType result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
    AudioStreamCodecInfo> &info)
{
    HILOGI("GetAudioDeviceCodecInfo");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->GetAudioDeviceCodecInfo(device, info);
    HILOGI("GetAudioDeviceCodecInfo result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::SetActiveSinkDevice(const NearlinkRawAddress &device, uint64_t supportStreamType)
{
    HILOGD("SetActiveSinkDevice");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    int result = audioService->SetActiveSinkDevice(device, supportStreamType);
    HILOGI("SetActiveSinkDevice result: %{public}d", result);
    return result == SsapStatus::SSAP_SUCCESS ? NL_NO_ERROR : NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkASCServer::GetDualRecordAbility(const NearlinkRawAddress &device, bool &isSupport)
{
    HILOGD("enter");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    isSupport = audioService->GetDualRecordAbility(device);
    return NL_NO_ERROR;
}

NlErrCode NearlinkASCServer::GetKaraokeAbility(const NearlinkRawAddress &device, bool &isSupport)
{
    HILOGD("enter");
    ProfileASC *audioService = pimpl->GetServicePtr();
    NL_CHECK_RETURN_RET(audioService, NL_ERR_INTERNAL_ERROR, "audioService invalid.");
    isSupport = audioService->GetKaraokeAbility(device);
    return NL_NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS
