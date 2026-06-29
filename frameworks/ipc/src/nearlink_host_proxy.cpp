/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_host_proxy.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {
constexpr size_t PAIRED_DEVICE_MAX_NUM = 0xFFFF;
constexpr size_t DEVICE_UUID_MAX_NUM = 0xFFFF;

NlErrCode NearlinkHostProxy::GetProfile(const std::string &name, sptr<IRemoteObject> &remoteProfile)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(name), NL_ERR_IPC_TRANS_FAILED, "Write name error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GETPROFILE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        remoteProfile = reply.ReadRemoteObject();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::DisableSle()
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_DISABLE_SLE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::EnableSle(const SleAutoConnectPolicy autoConnPolicy)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(static_cast<int>(autoConnPolicy)),
        NL_ERR_IPC_TRANS_FAILED, "Write flag error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_ENABLE_SLE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DisableSleToOff()
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_DISABLE_SLE_TO_OFF, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::EnableSleToHalf()
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_ENABLE_SLE_TO_HALF, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::GetSleFullState(int &sleCurrentState)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_SLE_FULL_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        sleCurrentState = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsSleEnabled(bool &isSleEnabled)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_ENABLED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSleEnabled = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsSleHalfDisabled(bool &isSleHalfDisabled)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_HALF_DISABLED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSleHalfDisabled = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsSleDisabled(bool &isSleDisabled)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_DISABLED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSleDisabled = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsSleAvailableToCaller(bool &isSleAvailable)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_AVAILABLE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSleAvailable = reply.ReadBool();
    }
    return exception;
}

bool NearlinkHostProxy::IsSleAvailableToCallerInner()
{
    bool isSleAvailable = false;
    NlErrCode status = IsSleAvailableToCaller(isSleAvailable);
    return status == NL_NO_ERROR && isSleAvailable;
}

NlErrCode NearlinkHostProxy::GetAdapterConnectState(int32_t &state)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_ADAPTER_CONNECTION_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        state = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetProfileConnState(const std::string &address, int32_t &state)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_PROFILE_CONNECTION_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        state = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetLocalName(std::string &name)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_LOCAL_NAME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        name = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::SetLocalName(const std::string &name)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(name), NL_ERR_IPC_TRANS_FAILED, "Write name error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_LOCAL_NAME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::GetLocalAddress(std::string &addr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_LOCAL_ADDR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        addr = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetPairedDevices(std::vector<NearlinkRawAddress> &pairedAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_PAIRED_DEVICES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        uint32_t size = reply.ReadUint32();
        if (size > PAIRED_DEVICE_MAX_NUM) {
            HILOGE("size is too big, size=%{public}d", size);
            size = PAIRED_DEVICE_MAX_NUM;
        }
        for (uint32_t i = 0; i < size; i++) {
            std::shared_ptr<NearlinkRawAddress> rawAddress(reply.ReadParcelable<NearlinkRawAddress>());
            if (!rawAddress) {
                return NL_ERR_IPC_TRANS_FAILED;
            }
            pairedAddr.push_back(*rawAddress);
        }
    }
    return exception;
}

NlErrCode NearlinkHostProxy::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(connectionMode), NL_ERR_IPC_TRANS_FAILED, "Write connection mode error");
    NL_CHECK_RETURN_RET(data.WriteInt32(duration), NL_ERR_IPC_TRANS_FAILED, "Write duration error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_CONNECTION_MODE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RemovePair(const sptr<NearlinkRawAddress> &device)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(device.GetRefPtr()), NL_ERR_IPC_TRANS_FAILED,
        "Write device.GetRefPtr() error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_REMOVE_PAIR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RemoveAllPairs()
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_REMOVE_ALL_PAIRS, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RegisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_REGISTER_SLE_PERIPHERAL_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DeregisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_DEREGISTER_SLE_PERIPHERAL_OBSERVER, option, data,
        reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RegisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    HILOGD("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_REGISTER_SLE_ADAPTER_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    HILOGD("success");
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DeregisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_DEREGISTER_SLE_ADAPTER_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RegisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    HILOGD("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(observer != nullptr, NL_ERR_INVALID_PARAM, "observer is null");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    int32_t error = InnerTransact(NearlinkHostInterfaceCode::NL_REGISTER_DEVICE_BATTERY_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DeregisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(observer != nullptr, NL_ERR_INVALID_PARAM, "observer is null");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    int32_t error =
        InnerTransact(NearlinkHostInterfaceCode::NL_DEREGISTER_DEVICE_BATTERY_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::RegisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    HILOGD("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(observer != nullptr, NL_ERR_INVALID_PARAM, "observer is null");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
                        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    int32_t error = InnerTransact(NearlinkHostInterfaceCode::NL_REGISTER_DEVICE_RSSI_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DeregisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(observer != nullptr, NL_ERR_INVALID_PARAM, "observer is null");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_IPC_TRANS_FAILED,
                        "Write RemoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    int32_t error =
            InnerTransact(NearlinkHostInterfaceCode::NL_DEREGISTER_DEVICE_RSSI_OBSERVER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(
        NearlinkHostInterfaceCode::NL_GET_SLE_MAX_ADVERTISING_DATALENGTH, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        maxAdvDataLen = static_cast<uint32_t>(reply.ReadInt32());
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceName(int32_t transport, const std::string &address, std::string &name)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_NAME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        name = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceAlias(int32_t transport, const std::string &address, std::string &alias)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_ALIAS, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        alias = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::SetDeviceAlias(int32_t transport, const std::string &address, const std::string &alias)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");
    NL_CHECK_RETURN_RET(data.WriteString(alias), NL_ERR_IPC_TRANS_FAILED, "Write alias error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_DEVICE_ALIAS, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::GetPairState(int32_t transport, const std::string &address, int &pairState)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_PAIR_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        pairState = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::StartPair(int32_t transport, const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_START_PAIR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::StartCrediblePair(int32_t transport, const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_START_CREDIBLE_PAIR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::CancelPairing(int32_t transport, const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_CANCEL_PAIRING, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::SetPairingPassCode(int32_t transport, const std::string &address,
                                                const std::string &passCode)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");
    NL_CHECK_RETURN_RET(data.WriteString(passCode), NL_ERR_IPC_TRANS_FAILED, "Write alias error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_PASSCODE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::IsBondedFromLocal(int32_t transport, const std::string &address, bool &isBondedFromLocal)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_BONDED_FROM_LOCAL, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isBondedFromLocal = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsAcbConnected(int32_t transport, const std::string &address, bool &isAcbConnected)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_ACB_CONNECTED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isAcbConnected = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetLinkRole(int32_t transport, const std::string &address, uint8_t &role)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_LINK_ROLE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        role = reply.ReadUint8();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::SetPairingConfirmation(int32_t transport, const std::string &address, bool cfm)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");
    NL_CHECK_RETURN_RET(data.WriteBool(cfm), NL_ERR_IPC_TRANS_FAILED, "Write address error");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_CFM, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::IsAcbEncrypted(int32_t transport, const std::string &address, bool &isAcbEncrypted)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_ACB_ENCRYPTED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isAcbEncrypted = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceUuids(int32_t transport, const std::string &address,
    std::vector<std::string> &uuids)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_UUIDS, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        uint32_t size = reply.ReadUint32();
        if (size > DEVICE_UUID_MAX_NUM) {
            HILOGE("size is too big, size=%{public}u", size);
            size = DEVICE_UUID_MAX_NUM;
        }
        std::string uuid;
        for (uint32_t i = 0; i < size; i++) {
            uuid = reply.ReadString();
            uuids.push_back(uuid);
        }
    }
    return exception;
}

NlErrCode NearlinkHostProxy::PairRequestReply(int32_t transport, const std::string &address, bool accept)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_IPC_TRANS_FAILED, "Write transport error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    NL_CHECK_RETURN_RET(data.WriteBool(accept), NL_ERR_IPC_TRANS_FAILED, "Write accept error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_PAIR_REQUEST_PEPLY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::ReadRemoteRssiValue(const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_READ_REMOTE_RSSI_VALUE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceAppearance(const std::string &address, int &appearance)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_APPEARANCE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        appearance = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::ConnectAllowedProfiles(const std::string &remoteAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(remoteAddr), NL_ERR_IPC_TRANS_FAILED, "Write remoteAddr error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_CONNECT_ALLOWED_PROFILES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::DisconnectAllowedProfiles(const std::string &remoteAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(remoteAddr), NL_ERR_IPC_TRANS_FAILED, "Write remoteAddr error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_DISCONNECT_ALLOWED_PROFILES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::GetDeviceProductId(const std::string &address, uint16_t &productId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_PRODUCT_ID, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        productId = reply.ReadUint16();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceVendorId(const std::string &address, uint16_t &vendorId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_VENDOR_ID, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        vendorId = reply.ReadUint16();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceModel(const std::string &address, NearlinkDeviceModel &model)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_MODEL, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        std::shared_ptr<NearlinkDeviceModel> modelInner(reply.ReadParcelable<NearlinkDeviceModel>());
        NL_CHECK_RETURN_RET(modelInner, NL_ERR_IPC_TRANS_FAILED, "get model error");
        model = *modelInner;
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetDeviceInformation(const std::string &address, NearlinkDeviceInformation &information)
{
    HILOGI("enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_DEVICE_INFORMATION, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        std::shared_ptr<NearlinkDeviceInformation> infoInner(reply.ReadParcelable<NearlinkDeviceInformation>());
        NL_CHECK_RETURN_RET(infoInner, NL_ERR_IPC_TRANS_FAILED, "get information error");
        information = *infoInner;
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetAcbState(const std::string &address, int &acbState)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_ACB_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        acbState = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetBatteryLevel(const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_BATTERY_LEVEL, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::FactoryReset()
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_FACTORY_RESET, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(freq.size() == SLE_FREQ_HOPPING_LEN, NL_ERR_INVALID_PARAM, "Param_Freq len is invalid!");

    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteUInt8Vector(freq), NL_ERR_IPC_TRANS_FAILED, "Write freq error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_FREQ_HOPPING, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime)
{
    HILOGI("enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(protocolType), NL_ERR_IPC_TRANS_FAILED, "Write protocolType error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(pid), NL_ERR_IPC_TRANS_FAILED, "Write pid error.");
    NL_CHECK_RETURN_RET(data.WriteInt64(refuseTime), NL_ERR_IPC_TRANS_FAILED, "Write refuseTime error.");
 
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_UPDATE_REFUSE_POLICY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    HILOGI("enter");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(permission), NL_ERR_IPC_TRANS_FAILED, "Write permission error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_CHECK_PERMISSION_FOR_NAPI, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isGranted = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(cmd), NL_ERR_IPC_TRANS_FAILED, "Write cmd error");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_UPDATE_SLE_VIRTUAL_DEVICE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

ErrCode NearlinkHostProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    if (err != NO_ERROR) {
        HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
    }
    return err;
}

NlErrCode NearlinkHostProxy::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(btAddr), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_SLE_ADDR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        sleAddr = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(sleAddr), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_GET_BT_ADDR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        btAddr = reply.ReadString();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(sleAddr), NL_ERR_IPC_TRANS_FAILED, "Write sleAddr error.");
    NL_CHECK_RETURN_RET(data.WriteString(btAddr), NL_ERR_IPC_TRANS_FAILED, "Write btAddr error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_SET_BT_ADDR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHostProxy::IsFeatureSupported(int32_t feature, bool &isSupported)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(feature), NL_ERR_IPC_TRANS_FAILED, "Write feature error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_FEATURE_SUPPORTED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSupported = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkHostProxy::IsConnectionExist(bool &isConnectionExist)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHostInterfaceCode::NL_IS_SLE_CONNECTION_EXIST, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isConnectionExist = reply.ReadBool();
    }
    return exception;
}
}
}