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

#include "VcpStackAdapter.h"
#include "log_util.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {
static VcpStackAdapter *g_vcpStackAdapter = nullptr;
struct VcpStackAdapter::impl {
    impl(VcpStackCallback &callback) : stackCbk_(callback) {}

    VcpStackCallback &stackCbk_;
};

VcpStackAdapter::VcpStackAdapter(VcpStackCallback &callback)
    : pimpl(std::make_unique<impl>(callback))
{
    g_vcpStackAdapter = this;
}

VcpStackAdapter::~VcpStackAdapter() = default;

void VcpStackAdapter::OnVolumeChangeEventTask(const RawAddress &device, uint8_t volume)
{
    pimpl->stackCbk_.OnVolumeChangeEvent(device, volume);
}

void VcpStackAdapter::OnVolumeChangeEvent(SLE_Addr_S *addr, uint8_t volume)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[VcpStackAdapter]device=%{public}s, volume=%{public}d", GET_ENCRYPT_ADDR(device), volume);
    DoInVcpThread([vcpStackAdapter = g_vcpStackAdapter, device, volume]() -> void {
        vcpStackAdapter->OnVolumeChangeEventTask(device, volume);
    });
}

void VcpStackAdapter::OnMuteStatusChangeEventTask(const RawAddress &device, uint8_t muteStatus)
{
    pimpl->stackCbk_.OnMuteStatusChangeEvent(device, muteStatus);
}

void VcpStackAdapter::OnMuteStatusChangeEvent(SLE_Addr_S *addr, uint8_t muteStatus)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[VcpStackAdapter]device=%{public}s, muteStatus=%{public}d", GET_ENCRYPT_ADDR(device), muteStatus);
    DoInVcpThread([vcpStackAdapter = g_vcpStackAdapter, device, muteStatus]() -> void {
        vcpStackAdapter->OnMuteStatusChangeEventTask(device, muteStatus);
    });
}

void VcpStackAdapter::OnNotifyVolumeChangeTask(const RawAddress &device, const std::list<StreamVolume> &streamVolumes)
{
    pimpl->stackCbk_.OnNotifyVolumeChange(device, streamVolumes);
}

void VcpStackAdapter::OnNotifyVolumeChange(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E type, void *value)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    NL_CHECK_RETURN(value, "value is null.");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[VcpStackAdapter]device=%{public}s, type=%{public}d", GET_ENCRYPT_ADDR(device), type);
    if (type == NLSTK_McpVolumePropertyType_E::NLSTK_MCP_STREAM_VOLUME_STATUS) {
        NLSTK_McpStreamVolumeStatus_S* info = static_cast<NLSTK_McpStreamVolumeStatus_S*>(value);
        std::list<StreamVolume> streamVolumes;
        StreamVolume callStreamVolume(static_cast<int32_t>(info->callVolume),
            static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_CALL), info->callInfo);
        StreamVolume mediaStreamVolume(static_cast<int32_t>(info->mediaVolume),
            static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA), info->mediaInfo);
        streamVolumes.emplace_back(callStreamVolume);
        streamVolumes.emplace_back(mediaStreamVolume);

        DoInVcpThread([vcpStackAdapter = g_vcpStackAdapter, device, streamVolumes]() -> void {
            vcpStackAdapter->OnNotifyVolumeChangeTask(device, streamVolumes);
        });
    }
}

void VcpStackAdapter::OnSetVolumeRsp(SLE_Addr_S *addr, uint8_t errorCode)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGD("[VcpStackAdapter]device=%{public}s, errorCode=%{public}d", GET_ENCRYPT_ADDR(device), errorCode);
}

void VcpStackAdapter::OnConnectStateChangeTask(const RawAddress &device, uint8_t state, uint8_t preState)
{
    pimpl->stackCbk_.OnConnectionStateChanged(device, state, preState);
}

void VcpStackAdapter::OnConnectionStateChanged(SLE_Addr_S *addr, uint8_t state, uint8_t preState)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGD("[VcpStackAdapter]device=%{public}s, state=%{public}d", GET_ENCRYPT_ADDR(device), state);
    DoInVcpThread([vcpStackAdapter = g_vcpStackAdapter, device, state, preState]() -> void {
        vcpStackAdapter->OnConnectStateChangeTask(device, state, preState);
    });
}

void VcpStackAdapter::RegisterStackCbk()
{
    NLSTK_McpVolumeClientCallBack_S clientCallbacks;
    clientCallbacks.volumeChangeEvent = &OnVolumeChangeEvent;
    clientCallbacks.muteStatusChangeEvent = &OnMuteStatusChangeEvent;
    clientCallbacks.notifyVolumeChange = &OnNotifyVolumeChange;
    clientCallbacks.setVolumeRsp = &OnSetVolumeRsp;
    clientCallbacks.stateChange = &OnConnectionStateChanged;
    uint32_t ret = NLSTK_McpRegVolumeClientCbk(&clientCallbacks);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[VcpStackAdapter]register vcp client cbk failed, result=%{public}d", ret);
    }
}

void VcpStackAdapter::DeregisterStackCbk()
{
    NLSTK_McpVolumeClientCallBack_S clientCallbacks {};
    uint32_t ret = NLSTK_McpRegVolumeClientCbk(&clientCallbacks);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[VcpStackAdapter]register vcp client cbk failed, result=%{public}d", ret);
    }
}

void VcpStackAdapter::Connect(const RawAddress &device)
{
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    device.ConvertToUint8(addr.addr);
    uint32_t ret = NLSTK_McpVolumeConnect(&addr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[VcpStackAdapter]device(%{public}s) error, ret=0x%{public}x.", GET_ENCRYPT_ADDR(device), ret);
        OnConnectStateChangeTask(device, static_cast<uint8_t>(SleConnectState::DISCONNECTED),
            static_cast<uint8_t>(SleConnectState::DISCONNECTED));
    }
}

void VcpStackAdapter::Disconnect(const RawAddress &device)
{
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    device.ConvertToUint8(addr.addr);
    uint32_t ret = NLSTK_McpVolumeDisconnect(&addr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[VcpStackAdapter]device(%{public}s) error, ret=0x%{public}x.", GET_ENCRYPT_ADDR(device), ret);
    }
}

void VcpStackAdapter::SetVolume(const RawAddress &device, int32_t volume)
{
    HILOGI("[VcpStackAdapter]device=%{public}s, volume: %{public}d", GET_ENCRYPT_ADDR(device), volume);
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    device.ConvertToUint8(addr.addr);
    uint32_t ret = NLSTK_McpSetVolume(&addr, GET_REAL_VOLUME(static_cast<uint32_t>(volume)));
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("NLSTK_McpSetVolume error 0x%{public}x.", ret);
    }
}

void VcpStackAdapter::SetStreamVolume(const RawAddress &device, const std::list<StreamVolume> &streamVolumes)
{
    HILOGI("[VcpStackAdapter]device=%{public}s", GET_ENCRYPT_ADDR(device));
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    device.ConvertToUint8(addr.addr);

    uint8_t num = static_cast<uint8_t>(streamVolumes.size());
    NLSTK_McpSetStreamVolume_S *volumeArray = new (std::nothrow) NLSTK_McpSetStreamVolume_S[num];
    NL_CHECK_RETURN(volumeArray, "volumeArray is nullptr");
    uint8_t i = 0;
    for (auto &streamVolume : streamVolumes) {
        HILOGD("[VcpStackAdapter]streamType: %{public}d , volume: %{public}d",
            streamVolume.volumeStreamType_, streamVolume.volume_);
        volumeArray[i].volume = GET_REAL_VOLUME(static_cast<uint32_t>(streamVolume.volume_));
        volumeArray[i].streamType = static_cast<NLSTK_McpSetStreamVolume_E>(streamVolume.volumeStreamType_);
        i++;
    }
    uint32_t ret = NLSTK_McpSetStreamVolume(&addr, volumeArray, num);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("NLSTK_McpSetStreamVolume error 0x%{public}x.", ret);
        delete[] volumeArray;
        volumeArray = nullptr;
        return;
    }
    delete[] volumeArray;
    volumeArray = nullptr;
}

bool VcpStackAdapter::GetStreamVolumeAbility(const RawAddress &device)
{
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    device.ConvertToUint8(addr.addr);
    return NLSTK_McpGetStreamVolumeAbility(&addr);
}
}
}
