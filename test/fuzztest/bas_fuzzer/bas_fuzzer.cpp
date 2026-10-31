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

#include "bas_fuzzer.h"
#include <atomic>
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "sdf_traits.h"
#include "sdf_addr.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "dli_callback.h"
#include "cp_worker.h"
#include "bas_client.h"
#include "bas_common.h"
#include "bas_stm.h"
#include "bas_ssap_cbk.h"
#include "nlstk_bas_client.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "BasClientStackAdapter.h"
#include "raw_address.h"
#include "log.h"
#include "../nl_utils/fuzztest_utils.h"

namespace {
constexpr uint8_t BAS_FUZZ_ADDR_LEN = 6;
constexpr uint8_t BAS_FUZZ_HANDLE = 0x34;
constexpr int32_t BAS_FUZZ_APPID = 5;
/* 记录桩：注册给 bas_client 的用户回调，仅记录被调次数（HID cbk 记录桩手法） */
std::atomic<uint32_t> g_basCbkSeq { 0 };

void BasFuzzReadPropertyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    (void)addr;
    (void)type;
    (void)value;
    (void)ret;
    g_basCbkSeq.fetch_add(1);
}

void BasFuzzConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
    NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    (void)addr;
    (void)curState;
    (void)prevState;
    (void)errNumb;
    g_basCbkSeq.fetch_add(1);
}

void BasFuzzPropertyChangedCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value)
{
    (void)addr;
    (void)type;
    (void)value;
    g_basCbkSeq.fetch_add(1);
}

SLE_Addr_S BuildBasAddr(FuzzedDataProvider &fdp)
{
    SLE_Addr_S addr = {0};
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (uint8_t i = 0; i < BAS_FUZZ_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    return addr;
}

/* 铺态：每对迁移使用独立设备（RemoveDeviceInfo 会析构 devInfo，严禁跨对复用） */
BasDeviceInfo_S *BuildBasDev(uint8_t state, SLE_Addr_S addr, int32_t appId)
{
    BasClientInitIfEmpty();
    BasDeviceInfo_S *info = (BasDeviceInfo_S *)SDF_MemZalloc(sizeof(BasDeviceInfo_S));
    if (info == nullptr) {
        return nullptr;
    }
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    info->appId = appId;
    info->state = state;
    SDF_Traits handleTraits = {.dtor = SDF_MemFree};
    info->indexHandle = SDF_CreateVector(handleTraits);
    if (info->indexHandle == nullptr || !BasClientAddInfoIntoVector(info)) {
        BasClearDeviceData(info);
        return nullptr;
    }
    return info;
}

void PushHandle(BasDeviceInfo_S *info, uint16_t handle)
{
    uint16_t *elem = (uint16_t *)SDF_MemZalloc(sizeof(uint16_t));
    if (elem == nullptr) {
        return;
    }
    *elem = handle;
    if (!SDF_VectorEmplaceBack(info->indexHandle, elem)) {
        SDF_MemFree(elem);
    }
}

BasStmParam_S MakeEvent(int what, void *extData)
{
    BasStmParam_S param = {.what = what, .extData = extData, .reason = 0};
    return param;
}

/* ---- bas_stm.c 铺态+迁移对：FDP 事件环（随机性整改） ----
 * 状态全域 %9、事件由合法事件表索引 %11 选择（第 11 槽为非法事件）、
 * 事件附属字段（appId、linkState、service、property、ntf）全部由 FDP 决定；
 * 旧 30 个固定迁移对的语义由 corpus/seed_stm_*.bin 保证（尾部追加消费序，
 * 各保留段自建 FDP 实例、读序不变）。
 * 内存纪律：每轮独立设备（BuildBasDev 内 AddInfoIntoVector，状态机 RemoveDeviceInfo
 * 会析构 devInfo，严禁跨轮复用）；extData 恒指向本轮栈上结构，BasClientStmCall 同步消费。 */

constexpr uint8_t BAS_FUZZ_STM_STATE_NUM = BAS_DEVICE_DISCONNECTED + 1; /* 9 态 */
constexpr uint8_t BAS_FUZZ_EVENT_SEL_NUM = BAS_STM_EVENT_BUTT + 1;      /* 10 合法 + 1 非法槽 */
constexpr uint8_t BAS_FUZZ_STM_MAX_ROUNDS = 8;
/* 合法事件静态表（PR-9 DISC_REASON_TABLE 范式）：当前枚举连续恒等，保留表形式便于剔除非连续值 */
constexpr int BAS_FUZZ_EVENT_TABLE[BAS_STM_EVENT_BUTT] = {
    BAS_ON_USER_CONNECT, BAS_ON_USER_CONNECTING, BAS_ON_REGISTERED_CBK, BAS_ON_LINK_DISCONNECTED,
    BAS_ON_LINK_STATE_CHANGE, BAS_ON_GET_SERVICE_CBK, BAS_ON_READ_PROPERTY, BAS_ON_SET_NOTIFICATION,
    BAS_ON_NOTIFY_PROPERTY, BAS_ON_USER_DISCONNECT,
};

void DriveBasStmPair(FuzzedDataProvider &fdp)
{
    SLE_Addr_S addr = BuildBasAddr(fdp);
    uint8_t stmState = fdp.ConsumeIntegral<uint8_t>() % BAS_FUZZ_STM_STATE_NUM;
    int32_t appId = fdp.ConsumeBool() ? -1 : BAS_FUZZ_APPID;
    BasDeviceInfo_S *info = BuildBasDev(stmState, addr, appId);
    NL_CHECK_RETURN(info != nullptr, "build dev fail");

    uint8_t eventSel = fdp.ConsumeIntegral<uint8_t>() % BAS_FUZZ_EVENT_SEL_NUM;
    if (eventSel >= BAS_STM_EVENT_BUTT) {
        /* 非法事件槽：BUTT 起全域（default 分支覆盖） */
        BasClientStmCall(info, MakeEvent(BAS_STM_EVENT_BUTT + fdp.ConsumeIntegral<uint8_t>(), nullptr));
        return;
    }
    int what = BAS_FUZZ_EVENT_TABLE[eventSel];

    /* 状态相关铺底（信任边界之前的准备）：READ_PROPERTY/SET_NOTIFICATION 态的句柄域 */
    if ((stmState == BAS_READ_PROPERTY || stmState == BAS_SET_NOTIFICATION) && fdp.ConsumeBool()) {
        info->devHandleInfo.remainBatPctHdl = BAS_FUZZ_HANDLE;
        PushHandle(info, BAS_FUZZ_HANDLE);
        if (stmState == BAS_READ_PROPERTY) {
            PushHandle(info, BAS_FUZZ_HANDLE + 1);
        }
    }

    switch (what) {
        case BAS_ON_USER_CONNECT:
        case BAS_ON_USER_CONNECTING:
        case BAS_ON_USER_DISCONNECT:
        case BAS_ON_LINK_DISCONNECTED:
            BasClientStmCall(info, MakeEvent(what, nullptr));
            break;
        case BAS_ON_REGISTERED_CBK: {
            int32_t cbkAppId = fdp.ConsumeIntegral<int32_t>(); /* 全域（旧 -1/5 两档交种子） */
            BasClientStmCall(info, MakeEvent(what, (void *)&cbkAppId));
            break;
        }
        case BAS_ON_LINK_STATE_CHANGE: {
            /* BasOnStateChangedAfterCreateLinkState 直接解引用 extData，恒非空（产品 P-1 已登记） */
            uint8_t state = fdp.ConsumeIntegral<uint8_t>() % (SSAP_CONNECT_STATE_BUTT + 1);
            BasClientStmCall(info, MakeEvent(what, (void *)&state));
            break;
        }
        case BAS_ON_GET_SERVICE_CBK: {
            NLSTK_SsapPrty_S property = {0};
            property.handle = BAS_FUZZ_HANDLE;
            property.uuid = BasConvertUuidToStru(BAS_REMAIN_BATTERY_PERCENTAGE_UUID);
            NLSTK_SsapServ_S service = {0};
            service.uuid = BasConvertUuidToStru(BAS_SERVICE_UUID);
            service.propertyNum = 1;
            service.properties = &property;
            BasServiceMsg_S serviceMsg = {0};
            serviceMsg.serviceNum = fdp.ConsumeBool() ? 1 : 0;
            serviceMsg.service = (serviceMsg.serviceNum != 0) ? &service : nullptr;
            serviceMsg.func = nullptr;
            BasClientStmCall(info, MakeEvent(what, (void *)&serviceMsg));
            break;
        }
        case BAS_ON_READ_PROPERTY: {
            uint8_t battery = fdp.ConsumeIntegral<uint8_t>();
            NLSTK_SsapClientReadPropertyInfo_S property = {0};
            property.handle = fdp.ConsumeBool() ? BAS_FUZZ_HANDLE : (BAS_FUZZ_HANDLE + 1); /* hit/miss 旧对语义 */
            property.value.len = 1;
            property.value.data = &battery;
            BasReadPropertyMsg_S readMsg = {0};
            readMsg.property = fdp.ConsumeBool() ? &property : nullptr;
            readMsg.ret = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
            BasClientStmCall(info, MakeEvent(what, (void *)&readMsg));
            break;
        }
        case BAS_ON_SET_NOTIFICATION: {
            info->devHandleInfo.remainBatPctHdl = BAS_FUZZ_HANDLE;
            bool enable = fdp.ConsumeBool();
            NLSTK_SsapUuid_S uuid = BasConvertUuidToStru(BAS_SERVICE_UUID);
            BasSetPropertyNtfMsg_S setNtfMsg = {0};
            setNtfMsg.uuid = &uuid;
            setNtfMsg.handle = BAS_FUZZ_HANDLE;
            setNtfMsg.enable = enable;
            setNtfMsg.ret = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
            BasClientStmCall(info, MakeEvent(what, (void *)&setNtfMsg));
            break;
        }
        case BAS_ON_NOTIFY_PROPERTY: {
            info->devHandleInfo.remainBatPctHdl = BAS_FUZZ_HANDLE;
            uint8_t battery = fdp.ConsumeIntegral<uint8_t>();
            NLSTK_SsapClientReadPropertyInfo_S property = {0};
            property.handle = BAS_FUZZ_HANDLE;
            property.value.len = fdp.ConsumeIntegral<uint8_t>(); /* 全域（旧 1/2 两档交种子） */
            property.value.data = &battery;
            BasClientStmCall(info, MakeEvent(what, (void *)&property));
            break;
        }
        default:
            BasClientStmCall(info, MakeEvent(what, nullptr));
            break;
    }
}

void FuzzBasStmRandom(FuzzedDataProvider &fdp)
{
    uint8_t rounds = fdp.ConsumeIntegralInRange<uint8_t>(1, BAS_FUZZ_STM_MAX_ROUNDS);
    for (uint8_t i = 0; i < rounds; i++) {
        DriveBasStmPair(fdp);
    }
}

/* ---- bas_ssap_cbk.c：BasGetSsapCbk 取真实 cbk 表后直调函数本体（深对先铺 appId 匹配设备） ---- */

void FuzzBasSsapCbks(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    SLE_Addr_S addr = BuildBasAddr(fdp);
    NLSTK_SsapAppClientCb_S cbk = BasGetSsapCbk();
    int32_t appId = BAS_FUZZ_APPID;
    if (cbk.onRegisterApp != nullptr) {
        NLSTK_Errcode_E regRet = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
        cbk.onRegisterApp(appId, &addr, regRet);
    }
    if (cbk.onConnectionStateChanged != nullptr) {
        uint8_t linkState = fdp.ConsumeBool() ? SSAP_CONNECT_STATE_CONNECTED : SSAP_CONNECT_STATE_DISCONNECTED;
        NLSTK_Errcode_E connRet = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
        cbk.onConnectionStateChanged(appId, linkState, connRet, 0);
    }
    NLSTK_SsapPrty_S property = {0};
    property.handle = BAS_FUZZ_HANDLE;
    uint8_t battery = fdp.ConsumeIntegral<uint8_t>();
    /* onReadProperty(s)/onPropertyChanged 形参为 NLSTK_SsapClientReadPropertyInfo_S*
     * （含 value 字段）；NLSTK_SsapPrty_S 无 value，仅用于 service.properties */
    NLSTK_SsapClientReadPropertyInfo_S readInfo = {0};
    readInfo.handle = BAS_FUZZ_HANDLE;
    readInfo.value.len = 1;
    readInfo.value.data = &battery;
    NLSTK_SsapServ_S service = {0};
    service.uuid = BasConvertUuidToStru(BAS_SERVICE_UUID);
    service.propertyNum = 1;
    service.properties = &property;
    NLSTK_SsapUuid_S uuid = BasConvertUuidToStru(BAS_SERVICE_UUID);
    if (cbk.onGetServices != nullptr) {
        cbk.onGetServices(appId, &uuid, &service, 1, nullptr);
    }
    if (cbk.onReadProperty != nullptr) {
        NLSTK_Errcode_E readRet = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
        cbk.onReadProperty(appId, &readInfo, readRet);
    }
    if (cbk.onReadProperties != nullptr) {
        NLSTK_Errcode_E readsRet = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
        cbk.onReadProperties(appId, 1, &readInfo, readsRet);
    }
    if (cbk.onPropertyChanged != nullptr) {
        cbk.onPropertyChanged(appId, &readInfo);
    }
    if (cbk.onSetPropertyNtf != nullptr) {
        NLSTK_Errcode_E ntfRet = fdp.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
        cbk.onSetPropertyNtf(appId, &uuid, BAS_FUZZ_HANDLE, true, ntfRet);
    }
}

/* 真实注册链：ssapc 调度线程回调 onRegisterApp（BasRegisterAppCbk 本体）→ 状态机 REGISTERING 迁移 */
void FuzzBasRegAppChain(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    SLE_Addr_S addr = BuildBasAddr(fdp);
    BasDeviceInfo_S *info = BuildBasDev(BAS_APPID_REGISTERING, addr, -1);
    NL_CHECK_RETURN(info != nullptr, "build dev fail");
    NLSTK_ConnParam_S connParam = {0};
    NLSTK_SsapAppClientCb_S cbk = BasGetSsapCbk();
    (void)NLSTK_SsapClientRegAppAsyn(&addr, &connParam, &cbk);
}

/* ---- bas_client.c：任务函数直调 ---- */

void FuzzBasClientTasks(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    BasClientCallBack_S cbk = {0};
    cbk.readPropertyCbk = BasFuzzReadPropertyCbk;
    cbk.connectStateChangeCbk = BasFuzzConnectStateChangeCbk;
    cbk.propertyChangedCbk = BasFuzzPropertyChangedCbk;
    BasRegClientCbk(&cbk);
    (void)BasEnable();
    SLE_Addr_S addr = BuildBasAddr(fdp);
    BasConnectTask(&addr);
    uint8_t num = 0;
    BasCountConnectedDevicesTask(&num);
    BasGetBatteryLevelTask(&addr);
    BasDisconnectTask(&addr);
}

/* ---- BasClientStackAdapter.cpp：公有构造 + 全公有方法直调 ---- */

void FuzzBasAdapter(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::Nearlink::BasClientStackAdapter adapter;
    std::string addrStr = OHOS::Nearlink::BuildAddressString(fdp);
    OHOS::Nearlink::RawAddress rawAddr(addrStr);
    (void)adapter.RegisterCallBackToStack();
    (void)adapter.Connect(rawAddr);
    (void)adapter.Disconnect(rawAddr);
    adapter.GetDeviceBatteryLevel(rawAddr);
}
} // namespace

void FuzzBasApi(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < 1) {
        return;
    }
    /* adapter 先行：把真实 OnBas*Cbk 注册进 g_basClientCbk，后续状态机驱动经
     * BasStateChangeCbk/BasReadCbk/BasNotifyCbk 回放覆盖 adapter 侧 private static 回调 */
    FuzzBasAdapter(data, size);
    FuzzBasClientTasks(data, size);
    FuzzBasRegAppChain(data, size);
    /* bas_ssap_cbk cbk 本体直调 */
    FuzzBasSsapCbks(data, size);
    /* bas_stm 事件环（新增消费，追加尾部；各保留段自建 FDP 读序不变）：
     * [尾1] rounds = InRange(1,8)；每轮自尾端依次消费
     * addr 7B、stmState(%9)、appId bool、eventSel(%11，10 号为非法槽+1B)，
     * 之后按事件消费附属字段（REGISTERED_CBK int32、LINK_STATE_CHANGE u8(%6)、
     * GET_SERVICE bool、READ_PROPERTY u8+bool+bool+bool、SET_NOTIFICATION bool+bool、
     * NOTIFY u8+u8、句柄铺底 bool） */
    {
        FuzzedDataProvider fdp(data, size);
        FuzzBasStmRandom(fdp);
    }
    BasDisable();
}

static void BasFuzzRecvAcbStub(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    /* 调度器保活链（ssapclientuapi_fuzzer 范式）：RegAppAsyn/ProfileConnect 全部经
     * SchedulePostTask 投递，InitStack ext 失败回滚后需重建调度器 */
    ScheduleEnable();
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.recvAcbHandler = BasFuzzRecvAcbStub;
    (void)DLI_SetCallback(&dliCallback);
    (void)NLSTK_InitStack();
    (void)NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    ScheduleEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *fuzzData, size_t size)
{
    if (fuzzData != nullptr && size >= 1) {
        FuzzBasApi(fuzzData, size);
    }
    return 0;
}
