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

#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "cdsmservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "nearlink_host_server.h"
#include "nearlink_cdsm_client_server.h"
#include "nearlink_raw_address.h"
#include "CdsmService.h"
#include "CdsmClient.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkCdsmClientCallbackStub : public IRemoteStub<INearlinkCdsmClientCallback> {
public:
    void OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo) {}
};

class FuzzCdsmClientCallback : public InterfaceCdsmClientServiceCallback {
public:
    void OnCdsmClientStateChange(const RawAddress &remoteAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo,
        std::optional<uint64_t> tokenId) {}
};

namespace {
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    sptr<NearlinkCdsmClientServer> g_ascClient = new (std::nothrow) NearlinkCdsmClientServer();
    sptr<INearlinkCdsmClientCallback> g_ascClientCb = new (std::nothrow) MockNearlinkCdsmClientCallbackStub();
    ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t CdsmClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ascClient, TRANSACTION_ERR, "cdsmClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("CdsmClientOnRemoteRequest, cmd(%{public}d)", code);
    return g_ascClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());
    if (g_ascClientCb != nullptr) {
        data.WriteRemoteObject(g_ascClientCb->AsObject());
    }

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK,data, reply);
    if (ret != NL_NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetCdsmInfoFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_CDSM_CLIENT_GET_CDS_INFO, data, reply);
    if (ret != NL_NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void DeregisterApplicationFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    if (ret != NL_NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

/* CdsmService 会话生命周期直调：Enable->注册回调->建组->Connect/Disconnect->成员/组查询->回调触发->删组->Disable */
void CdsmSessionLifecycleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService == nullptr) {
        HILOGI("CdsmSessionLifecycleFuzzTest: cdsmService is nullptr");
        return;
    }
    FuzzedDataProvider provider(fuzzData, size);
    cdsmService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(HOST_FUZZ_DELAY_100_MS));
    (void)cdsmService->RegisterCdsmClientCallback(std::make_shared<FuzzCdsmClientCallback>());

    const std::string reportAddrStr = BuildAddressString(provider);
    const RawAddress reportAddr(reportAddrStr);
    std::vector<RawAddress> members;
    constexpr size_t maxMemberCount = 4;
    size_t memberCount = provider.ConsumeIntegral<uint8_t>() % maxMemberCount + 1;
    for (size_t i = 0; i < memberCount; i++) {
        members.push_back(RawAddress(BuildAddressString(provider)));
    }
    (void)cdsmService->CdsmCreateGroup(reportAddr, members, provider.ConsumeBool());

    const RawAddress &firstMember = members.front();
    for (size_t i = 0; i < members.size(); i++) {
        (void)cdsmService->Connect(members[i]);
        (void)cdsmService->Disconnect(members[i]);
    }

    std::vector<NearlinkCdsmInfo> cdsmInfos;
    (void)cdsmService->CdsmGetAllMemberInfo(firstMember, cdsmInfos);
    (void)cdsmService->CdsmCheckIsCooperationMember(firstMember);
    (void)cdsmService->CdsmCheckIsCooperationDevice(reportAddr);
    (void)cdsmService->CdsmCheckIsCooperationReport(reportAddr);
    (void)cdsmService->CdsmCheckIsSameCooperation(members);
    RawAddress outAddr;
    (void)cdsmService->CdsmGetReportAddr(firstMember, outAddr);
    (void)cdsmService->CdsmGetOtherAddr(firstMember, outAddr);
    uint32_t groupId = 0;
    (void)cdsmService->CdsmGetGroupId(groupId, firstMember.GetAddress());
    cdsmService->CdsmStopInviteAdv(reportAddr, provider.ConsumeBool());

    cdsmService->HandleCdsmClientCallback(firstMember);
    constexpr uint8_t cdsmStateCount = 4; // CDSM_STATE_DISCONNECTED..CDSM_STATE_CONNECTED
    CdsmClientState preState = static_cast<CdsmClientState>(provider.ConsumeIntegral<uint8_t>() % cdsmStateCount);
    CdsmClientState curState = static_cast<CdsmClientState>(provider.ConsumeIntegral<uint8_t>() % cdsmStateCount);
    cdsmService->NotifyStateChanged(firstMember, curState, preState);
    constexpr uint8_t connectStateCount = 5; // SleConnectState::CONNECTING..INVALID_STATE
    cdsmService->UpdateCdsmMemberProfileConnectState(firstMember,
        static_cast<SleConnectState>(provider.ConsumeIntegral<uint8_t>() % connectStateCount));

    cdsmService->CdsmDeleteGroup(reportAddr);
    (void)cdsmService->DeregisterCdsmClientCallback();
    cdsmService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(HOST_FUZZ_DELAY_100_MS));
}

/* CdsmClient 直调：CreateCdsmClient 判空后驱动连接/状态/广播停止 */
void CdsmClientDirectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    const std::string addrStr = BuildAddressString(provider);
    std::shared_ptr<CdsmClient> cdsmClient = CdsmClient::CreateCdsmClient(addrStr);
    if (cdsmClient == nullptr) {
        HILOGI("CdsmClientDirectFuzzTest: cdsmClient is nullptr");
        return;
    }
    (void)cdsmClient->CdsmClientStartConnect();
    constexpr uint8_t cdsmStateCount = 4; // CDSM_STATE_DISCONNECTED..CDSM_STATE_CONNECTED
    CdsmClientState state = static_cast<CdsmClientState>(provider.ConsumeIntegral<uint8_t>() % cdsmStateCount);
    cdsmClient->CdsmClientUpdateState(state);
    (void)cdsmClient->CdsmClientGetState();
    (void)cdsmClient->CdsmClientStartDisconnect();
    cdsmClient->CdsmStopInviteAdv();
}

}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    HILOGI("VcpFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("VcpFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterApplicationFuzzTest(data, size);
    OHOS::GetCdsmInfoFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest();
    OHOS::CdsmSessionLifecycleFuzzTest(data, size);
    OHOS::CdsmClientDirectFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
