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

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "ipcapi_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_controller_stub.h"
#include "nearlink_sle_controller_server.h"
#include "nearlink_tws_client_stub.h"
#include "nearlink_tws_client_server.h"
#include "nearlink_vcp_client_stub.h"
#include "nearlink_vcp_client_server.h"
#include "nearlink_hadm_client_stub.h"
#include "nearlink_hadm_client_server.h"
#include "nearlink_host_server.h"
#include "nearlink_raw_address.h"
#include "nearlink_asc_audio_stream_info.h"
#include "nearlink_ASC_source.h"
#include "ThreadUtil.h"
#include "log.h"

using namespace std;
using namespace OHOS::Nearlink;

namespace OHOS {
namespace {
constexpr int IPCAPI_FUZZ_DELAY_100_MS = 100;
constexpr int IPCAPI_FUZZ_DELAY_5000_MS = 5000;
constexpr uint8_t IPCAPI_STUB_NUM = 4;
constexpr uint8_t IPCAPI_MODE_NUM = 3;
constexpr uint8_t IPCAPI_MODE_LEGAL = 0;
constexpr uint8_t IPCAPI_MODE_INVALID = 1;
constexpr uint8_t IPCAPI_MODE_OUT_OF_RANGE = 2;
constexpr uint8_t IPCAPI_INVALID_SUB_NUM = 3;
constexpr uint8_t IPCAPI_OUT_OF_RANGE_NUM = 4;
constexpr uint8_t IPCAPI_INVALID_SUB_WRONG_TOKEN = 0;
constexpr uint8_t IPCAPI_INVALID_SUB_TRUNCATED = 1;
constexpr uint8_t IPCAPI_INVALID_SUB_BAD_ADDR = 2;
constexpr uint8_t IPCAPI_STUB_SLE_CONTROLLER = 0;
constexpr uint8_t IPCAPI_STUB_TWS_CLIENT = 1;
constexpr uint8_t IPCAPI_STUB_VCP_CLIENT = 2;
constexpr uint8_t IPCAPI_STUB_HADM_CLIENT = 3;
constexpr int32_t IPCAPI_COEX_DEVICE_MAX = 4;
constexpr uint32_t IPCAPI_STREAM_ITEM_MAX = 4;
constexpr uint8_t IPCAPI_STREAM_STATE_NUM = 3;
constexpr size_t IPCAPI_FUZZ_STRING_MAX = 64;
constexpr size_t IPCAPI_MAC_BYTES = 6;

sptr<NearlinkSleControllerServer> g_sleController = nullptr;
sptr<NearlinkTwsClientServer> g_twsClient = nullptr;
sptr<NearlinkVcpClientServer> g_vcpClient = nullptr;
sptr<NearlinkHadmClientServer> g_hadmClient = nullptr;
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();

// serialization round-trip statistics (no abort on mismatch, PR-337 lesson)
uint32_t g_addrRoundTripOk = 0;
uint32_t g_addrRoundTripFail = 0;
uint32_t g_streamRoundTripOk = 0;
uint32_t g_streamRoundTripFail = 0;
} // namespace

class MockTwsClientObserverStub : public IRemoteStub<INearlinkTwsClientObserver> {
public:
    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) {}
};

class MockHadmClientCallbackStub : public IRemoteStub<INearlinkHadmClientCallback> {
public:
    MockHadmClientCallbackStub()
    {
    }
    ~MockHadmClientCallbackStub()
    {
    }
    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) {}
    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) {}
};

namespace {
sptr<INearlinkTwsClientObserver> g_twsObserver = new (std::nothrow) MockTwsClientObserverStub();
sptr<INearlinkHadmClientCallback> g_hadmCallback = new (std::nothrow) MockHadmClientCallbackStub();

// Build a valid 17-char "XX:XX:XX:XX:XX:XX" address from 6 tail-consumed bytes (uppercase hex).
std::string BuildValidMacString(FuzzedDataProvider &fdp)
{
    static const char *hexDigits = "0123456789ABCDEF";
    std::string mac = "";
    for (size_t i = 0; i < IPCAPI_MAC_BYTES; i++) {
        uint8_t byte = fdp.ConsumeIntegral<uint8_t>();
        mac += hexDigits[(byte >> 4) & 0x0F];
        mac += hexDigits[byte & 0x0F];
        if (i + 1 < IPCAPI_MAC_BYTES) {
            mac += ':';
        }
    }
    return mac;
}

void WriteRawAddressParcelable(MessageParcel &data, FuzzedDataProvider &fdp)
{
    NearlinkRawAddress addr(BuildValidMacString(fdp));
    data.WriteParcelable(&addr);
}

void WriteTwsObserver(MessageParcel &data)
{
    if (g_twsObserver != nullptr) {
        data.WriteRemoteObject(g_twsObserver->AsObject());
    }
}

void WriteHadmCallback(MessageParcel &data)
{
    if (g_hadmCallback != nullptr) {
        data.WriteRemoteObject(g_hadmCallback->AsObject());
    }
}

void WriteStreamInfoParcelable(MessageParcel &data, FuzzedDataProvider &fdp)
{
    NearlinkASCAudioStreamInfo streamInfo {};
    std::vector<struct AudioStreamInfo> streamData = {};
    uint32_t count = fdp.ConsumeIntegralInRange<uint32_t>(0, IPCAPI_STREAM_ITEM_MAX);
    for (uint32_t i = 0; i < count; i++) {
        struct AudioStreamInfo item = {};
        item.streamType = static_cast<AudioStreamType>(fdp.ConsumeIntegral<uint32_t>());
        item.streamState = static_cast<AudioStreamState>(fdp.ConsumeIntegral<uint8_t>() % IPCAPI_STREAM_STATE_NUM);
        streamData.push_back(item);
    }
    streamInfo.SetStreamState(streamData);
    data.WriteParcelable(&streamInfo);
}

// read order: COEX_PARAM = uint16 + uint8; UPDATE_INTERVAL = string + int32;
// COEX_MODE = int32 + int32 deviceSize + N * (string + int32, stub caps N at 32)
void WriteSleControllerBody(MessageParcel &data, uint8_t code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_PARAM: {
            uint16_t maxBitRate = fdp.ConsumeIntegral<uint16_t>();
            uint8_t dutyCycle = fdp.ConsumeIntegral<uint8_t>();
            data.WriteUint16(maxBitRate);
            data.WriteUint8(dutyCycle);
            break;
        }
        case NearlinkSleControllerInterfaceCode::NL_SLE_UPDATE_INTERVAL: {
            std::string device = BuildValidMacString(fdp);
            int32_t intervalType = fdp.ConsumeIntegral<int32_t>();
            data.WriteString(device);
            data.WriteInt32(intervalType);
            break;
        }
        case NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_MODE: {
            int32_t mode = fdp.ConsumeIntegral<int32_t>();
            int32_t deviceSize = fdp.ConsumeIntegralInRange<int32_t>(0, IPCAPI_COEX_DEVICE_MAX);
            data.WriteInt32(mode);
            data.WriteInt32(deviceSize);
            for (int32_t i = 0; i < deviceSize; i++) {
                std::string device = BuildValidMacString(fdp);
                int32_t param = fdp.ConsumeIntegral<int32_t>();
                data.WriteString(device);
                data.WriteInt32(param);
            }
            break;
        }
        default:
            break;
    }
}

// read order: REGISTER/DEREGISTER_APP = RemoteObject; address codes honor invalidAddress;
// SET_VAC_TYPE = string + int32 + int32; SEND_USER_SELECTION = string + Parcelable(stream info)
void WriteTwsClientBody(MessageParcel &data, uint8_t code, FuzzedDataProvider &fdp, bool invalidAddress)
{
    switch (code) {
        case NearlinkTwsClientInterfaceCode::NL_TWS_REGISTER_APP:
        case NearlinkTwsClientInterfaceCode::NL_TWS_DEREGISTER_APP:
            WriteTwsObserver(data);
            break;
        case NearlinkTwsClientInterfaceCode::NL_SEND_USER_SELECTION: {
            std::string address = invalidAddress ? fdp.ConsumeRandomLengthString(IPCAPI_FUZZ_STRING_MAX)
                : BuildValidMacString(fdp);
            data.WriteString(address);
            WriteStreamInfoParcelable(data, fdp);
            break;
        }
        case NearlinkTwsClientInterfaceCode::NL_SET_VIRTUAL_AUTO_CONNECT_TYPE: {
            std::string address = invalidAddress ? fdp.ConsumeRandomLengthString(IPCAPI_FUZZ_STRING_MAX)
                : BuildValidMacString(fdp);
            int32_t connType = fdp.ConsumeIntegral<int32_t>();
            int32_t businessType = fdp.ConsumeIntegral<int32_t>();
            data.WriteString(address);
            data.WriteInt32(connType);
            data.WriteInt32(businessType);
            break;
        }
        default: {
            // ENABLE/DISABLE_WEAR, GET_WEAR_STATE, IS_WEARING, IS_WEAR_SUP, GET_ROLE,
            // GET_AUDIO_DELAY, IS_SUPPORT_VAC, QUERY_STREAM_STATE: single address string
            std::string address = invalidAddress ? fdp.ConsumeRandomLengthString(IPCAPI_FUZZ_STRING_MAX)
                : BuildValidMacString(fdp);
            data.WriteString(address);
            break;
        }
    }
}

// read order: SET_ABS_VOLUME = Parcelable(NearlinkRawAddress) + int32 + uint8;
// GET_MEDIA/GET_CALL = Parcelable(NearlinkRawAddress)
void WriteVcpClientBody(MessageParcel &data, uint8_t code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case NearlinkVcpInterfaceCode::NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME: {
            WriteRawAddressParcelable(data, fdp);
            int32_t volumeLevel = fdp.ConsumeIntegral<int32_t>();
            uint8_t streamType = fdp.ConsumeIntegral<uint8_t>();
            data.WriteInt32(volumeLevel);
            data.WriteUint8(streamType);
            break;
        }
        case NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_MEDIA_VOLUME:
        case NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_CALL_VOLUME:
            WriteRawAddressParcelable(data, fdp);
            break;
        default:
            break;
    }
}

// read order: REGISTER_CB = RemoteObject only (hadmId is not read);
// DEREGISTER_CB = uint32 + RemoteObject; START_SOUNDING = uint32 + Parcelable + uint8;
// STOP_SOUNDING = uint32 + Parcelable; GET_FEATURE = empty
void WriteHadmClientBody(MessageParcel &data, uint8_t code, FuzzedDataProvider &fdp)
{
    switch (code) {
        case NearlinkHadmClientInterfaceCode::NL_REGISTER_HADM_CLIENT_CALLBACK:
            WriteHadmCallback(data);
            break;
        case NearlinkHadmClientInterfaceCode::NL_DE_REGISTER_HADM_CLIENT_CALLBACK: {
            uint32_t hadmId = fdp.ConsumeIntegral<uint32_t>();
            data.WriteUint32(hadmId);
            WriteHadmCallback(data);
            break;
        }
        case NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_START_SOUNDING: {
            uint32_t hadmId = fdp.ConsumeIntegral<uint32_t>();
            uint8_t toneControl = fdp.ConsumeIntegral<uint8_t>();
            data.WriteUint32(hadmId);
            WriteRawAddressParcelable(data, fdp);
            data.WriteUint8(toneControl);
            break;
        }
        case NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_STOP_SOUNDING: {
            uint32_t hadmId = fdp.ConsumeIntegral<uint32_t>();
            data.WriteUint32(hadmId);
            WriteRawAddressParcelable(data, fdp);
            break;
        }
        case NearlinkHadmClientInterfaceCode::NL_GET_HADM_FEATURE:
        default:
            break;
    }
}

void WriteStubDescriptor(MessageParcel &data, uint8_t stubSel)
{
    switch (stubSel) {
        case IPCAPI_STUB_SLE_CONTROLLER:
            data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
            break;
        case IPCAPI_STUB_TWS_CLIENT:
            data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
            break;
        case IPCAPI_STUB_VCP_CLIENT:
            data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
            break;
        default:
            data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
            break;
    }
}

void WriteStubBody(MessageParcel &data, uint8_t stubSel, uint8_t code, FuzzedDataProvider &fdp, bool invalidAddress)
{
    switch (stubSel) {
        case IPCAPI_STUB_SLE_CONTROLLER:
            WriteSleControllerBody(data, code, fdp);
            break;
        case IPCAPI_STUB_TWS_CLIENT:
            WriteTwsClientBody(data, code, fdp, invalidAddress);
            break;
        case IPCAPI_STUB_VCP_CLIENT:
            WriteVcpClientBody(data, code, fdp);
            break;
        default:
            WriteHadmClientBody(data, code, fdp);
            break;
    }
}

uint8_t GetStubCodeButt(uint8_t stubSel)
{
    switch (stubSel) {
        case IPCAPI_STUB_SLE_CONTROLLER:
            return static_cast<uint8_t>(NearlinkSleControllerInterfaceCode::NL_SLE_CONTROLLER_BUTT);
        case IPCAPI_STUB_TWS_CLIENT:
            return static_cast<uint8_t>(NearlinkTwsClientInterfaceCode::NL_TWS_CLIENT_BUTT);
        case IPCAPI_STUB_VCP_CLIENT:
            return static_cast<uint8_t>(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_BUTT);
        default:
            return static_cast<uint8_t>(NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_MANAGER_BUTT);
    }
}

int32_t IpcApiOnRemoteRequest(uint8_t stubSel, uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    MessageOption option = { MessageOption::TF_SYNC };
    switch (stubSel) {
        case IPCAPI_STUB_SLE_CONTROLLER:
            if (g_sleController == nullptr) {
                g_sleController = new (std::nothrow) NearlinkSleControllerServer();
            }
            if (g_sleController == nullptr) {
                return TRANSACTION_ERR;
            }
            return g_sleController->OnRemoteRequest(code, data, reply, option);
        case IPCAPI_STUB_TWS_CLIENT:
            if (g_twsClient == nullptr) {
                g_twsClient = new (std::nothrow) NearlinkTwsClientServer();
            }
            if (g_twsClient == nullptr) {
                return TRANSACTION_ERR;
            }
            return g_twsClient->OnRemoteRequest(code, data, reply, option);
        case IPCAPI_STUB_VCP_CLIENT:
            if (g_vcpClient == nullptr) {
                g_vcpClient = new (std::nothrow) NearlinkVcpClientServer();
            }
            if (g_vcpClient == nullptr) {
                return TRANSACTION_ERR;
            }
            return g_vcpClient->OnRemoteRequest(code, data, reply, option);
        default:
            if (g_hadmClient == nullptr) {
                g_hadmClient = new (std::nothrow) NearlinkHadmClientServer();
            }
            if (g_hadmClient == nullptr) {
                return TRANSACTION_ERR;
            }
            return g_hadmClient->OnRemoteRequest(code, data, reply, option);
    }
}

// write body if requested, then dispatch to the selected stub server
void InvokeStubRequest(uint8_t stubSel, uint8_t code, MessageParcel &data, MessageParcel &reply,
    FuzzedDataProvider &fdp, bool invalidAddress, bool writeBody)
{
    if (writeBody) {
        WriteStubBody(data, stubSel, code, fdp, invalidAddress);
    }
    int32_t ret = IpcApiOnRemoteRequest(stubSel, code, data, reply);
    if (ret != NO_ERROR) {
    }
    (void)reply.ReadInt32();
}

// stub Sel -> code -> mode matrix:
// mode 0: legal token + full params per stub read order;
// mode 1: (a) wrong descriptor, (b) truncated parcel (token only),
//         (c) invalid address string for tws address codes;
// mode 2: code beyond BUTT -> memberFuncMap_ miss -> IPCObjectStub fallback
void DispatchStubRequest(uint8_t stubSel, FuzzedDataProvider &fdp)
{
    uint8_t mode = fdp.ConsumeIntegral<uint8_t>() % IPCAPI_MODE_NUM;
    uint8_t butt = GetStubCodeButt(stubSel);
    uint8_t code = 0;
    if (mode == IPCAPI_MODE_OUT_OF_RANGE) {
        code = static_cast<uint8_t>(butt + fdp.ConsumeIntegral<uint8_t>() % IPCAPI_OUT_OF_RANGE_NUM);
    } else {
        code = fdp.ConsumeIntegral<uint8_t>() % butt;
    }

    MessageParcel data;
    MessageParcel reply;
    bool invalidAddress = false;
    bool writeBody = true;
    if (mode == IPCAPI_MODE_LEGAL) {
        WriteStubDescriptor(data, stubSel);
    } else if (mode == IPCAPI_MODE_OUT_OF_RANGE) {
        // correct token so the miss lands in memberFuncMap_ lookup, not token check
        WriteStubDescriptor(data, stubSel);
        data.WriteInt32(fdp.ConsumeIntegral<int32_t>());
        data.WriteString(fdp.ConsumeRandomLengthString(IPCAPI_FUZZ_STRING_MAX));
        writeBody = false;
    } else {
        uint8_t subMode = fdp.ConsumeIntegral<uint8_t>() % IPCAPI_INVALID_SUB_NUM;
        if (subMode == IPCAPI_INVALID_SUB_WRONG_TOKEN) {
            std::u16string wrongDescriptor = u"ohos.ipc.WRONG";
            data.WriteInterfaceToken(wrongDescriptor);
        } else if (subMode == IPCAPI_INVALID_SUB_BAD_ADDR && stubSel == IPCAPI_STUB_TWS_CLIENT) {
            // tws address codes get an invalid address string (IsValidAddress reject branch);
            // RemoteObject-only codes degrade to a valid token-only parcel
            WriteStubDescriptor(data, stubSel);
            invalidAddress = true;
        } else {
            // truncated: interface token only, Inner reads hit default/failed branches
            WriteStubDescriptor(data, stubSel);
            writeBody = false;
        }
    }
    InvokeStubRequest(stubSel, code, data, reply, fdp, invalidAddress, writeBody);
}

// one write->read round trip of NearlinkRawAddress; counts result, never aborts
void CheckAddressRoundTrip(const std::string &addrStr)
{
    NearlinkRawAddress orig(addrStr);
    MessageParcel parcel;
    if (!parcel.WriteParcelable(&orig)) {
        g_addrRoundTripFail++;
        return;
    }
    std::shared_ptr<NearlinkRawAddress> dest(parcel.ReadParcelable<NearlinkRawAddress>());
    if (dest == nullptr) {
        g_addrRoundTripFail++;
        return;
    }
    if (dest->GetAddress() == addrStr) {
        g_addrRoundTripOk++;
    } else {
        g_addrRoundTripFail++;
    }
}

// Serialization round-trip check (direct, bypasses stub): valid mac + arbitrary fuzz string
void FuzzAddressRoundTrip(FuzzedDataProvider &fdp)
{
    CheckAddressRoundTrip(BuildValidMacString(fdp));
    CheckAddressRoundTrip(fdp.ConsumeRandomLengthString(IPCAPI_FUZZ_STRING_MAX));
}

void FuzzStreamInfoRoundTrip(FuzzedDataProvider &fdp)
{
    NearlinkASCAudioStreamInfo orig {};
    std::vector<struct AudioStreamInfo> streamData = {};
    uint32_t count = fdp.ConsumeIntegralInRange<uint32_t>(0, IPCAPI_STREAM_ITEM_MAX);
    for (uint32_t i = 0; i < count; i++) {
        struct AudioStreamInfo item = {};
        item.streamType = static_cast<AudioStreamType>(fdp.ConsumeIntegral<uint32_t>());
        item.streamState = static_cast<AudioStreamState>(fdp.ConsumeIntegral<uint8_t>() % IPCAPI_STREAM_STATE_NUM);
        streamData.push_back(item);
    }
    orig.SetStreamState(streamData);

    MessageParcel parcel;
    if (!parcel.WriteParcelable(&orig)) {
        g_streamRoundTripFail++;
        return;
    }
    std::shared_ptr<NearlinkASCAudioStreamInfo> dest(parcel.ReadParcelable<NearlinkASCAudioStreamInfo>());
    if (dest == nullptr) {
        g_streamRoundTripFail++;
        return;
    }
    std::vector<struct AudioStreamInfo> outData = {};
    dest->GetStreamState(outData);
    if (outData.size() != streamData.size()) {
        g_streamRoundTripFail++;
        return;
    }
    for (size_t i = 0; i < outData.size(); i++) {
        if (outData[i].streamType != streamData[i].streamType ||
            outData[i].streamState != streamData[i].streamState) {
            g_streamRoundTripFail++;
            return;
        }
    }
    g_streamRoundTripOk++;
}
} // namespace

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // mock token; permission check keeps the real chain (device: Inner reachable;
    // no ATM environment: VerifyMultiPermissions reject branch 202 reachable)
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(IPCAPI_FUZZ_DELAY_100_MS));
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(IPCAPI_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    FuzzedDataProvider fdp(data, size);
    uint8_t stubSel = fdp.ConsumeIntegral<uint8_t>() % OHOS::IPCAPI_STUB_NUM;
    OHOS::DispatchStubRequest(stubSel, fdp);
    OHOS::FuzzAddressRoundTrip(fdp);
    OHOS::FuzzStreamInfoRoundTrip(fdp);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
