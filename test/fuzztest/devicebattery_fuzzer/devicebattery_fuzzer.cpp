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

/* Z-2 common 残留族：DeviceBatteryManager（4 公有方法直调，单例）+
 * SleCommonEventSubscriber（构造 + OnReceiveEvent 直调，不经 CES 服务订阅）。
 * 两文件合计 6 接口（≤10）；与 AdapterStateMachine（5 接口）合并会超限，故独立成工程。 */

#include <chrono>
#include <string>
#include <thread>
#include "devicebattery_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "raw_address.h"
#include "DeviceBatteryManager.h"
#include "SleCommonEventSubscriber.h"
#include "want.h"

using namespace OHOS::Nearlink;

namespace OHOS {
namespace {
constexpr int DEVICE_BATTERY_FUZZ_DELAY_50_MS = 50;
constexpr uint8_t DEVICE_BATTERY_FUZZ_CHARGE_MOD = 4;    // 充电状态 0~3
constexpr uint8_t DEVICE_BATTERY_FUZZ_CONN_STATE_MOD = 5; // SleConnectState 0~4
constexpr size_t DEVICE_BATTERY_FUZZ_INVALID_ADDR_MAX = 20;
constexpr size_t DEVICE_BATTERY_FUZZ_ACTION_MAX = 32;
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance(); // 构造即把全部线程置 ENABLED，勿再 per-round Init/Clear

// A. DeviceBatteryManager 直调（单例，全部 4 个公有方法；每调用独立取地址，铺生命周期组合）
void BatteryManagerFuzzTest(FuzzedDataProvider &fdp, int32_t batteryLevel)
{
    DeviceBatteryManager &batteryMgr = DeviceBatteryManager::GetInstance();

    RawAddress publishAddr = BuildRawAddress(fdp);
    BatteryInfo batteryInfo = {};
    batteryInfo.devBattery_ = fdp.ConsumeIntegral<uint8_t>();
    batteryInfo.leftBattery_ = fdp.ConsumeIntegral<uint8_t>();
    batteryInfo.rightBattery_ = fdp.ConsumeIntegral<uint8_t>();
    batteryInfo.leftCharge_ = fdp.ConsumeIntegral<uint8_t>() % DEVICE_BATTERY_FUZZ_CHARGE_MOD;
    batteryInfo.rightCharge_ = fdp.ConsumeIntegral<uint8_t>() % DEVICE_BATTERY_FUZZ_CHARGE_MOD;
    batteryInfo.boxBattery_ = fdp.ConsumeIntegral<uint8_t>();
    /* ponytail: 裸进程无音频设备，IsAudioDevice 恒 false → 提前返回分支；深路径
     * （GetReportAddr/SetDeviceSupportBattery）需 TWS profile 实例，超出本文件单例能力 */
    batteryMgr.PublishBatteryLevel(publishAddr, batteryInfo);

    bool stateAddrValid = fdp.ConsumeBool();
    std::string stateAddr = stateAddrValid ? BuildAddressString(fdp)
                                           : fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_INVALID_ADDR_MAX);
    int32_t connState = static_cast<int32_t>(fdp.ConsumeIntegral<uint8_t>() % DEVICE_BATTERY_FUZZ_CONN_STATE_MOD);
    /* CONNECTED→AddConnectedDevice / DISCONNECTED→erase 链 / 其它→"no need process" */
    batteryMgr.ProcessProfileStateChanged(stateAddr, connState);

    bool activeAddrValid = fdp.ConsumeBool();
    std::string activeAddr = activeAddrValid ? BuildAddressString(fdp)
                                             : fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_INVALID_ADDR_MAX);
    /* 有效/无效地址两分支（IsValidAddress），内部走 GetCurrentBatteryDevice+NotifyForDeviceChanged */
    batteryMgr.ProcessActiveDeviceChanged(activeAddr);

    bool reportAddrValid = fdp.ConsumeBool();
    std::string reportAddr = reportAddrValid ? BuildAddressString(fdp)
                                             : fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_INVALID_ADDR_MAX);
    batteryMgr.ReportDeviceBatteryInfo(reportAddr, batteryLevel);

    std::this_thread::sleep_for(std::chrono::milliseconds(DEVICE_BATTERY_FUZZ_DELAY_50_MS));
}

// B. SleCommonEventSubscriber：构造 + OnReceiveEvent 直调（action 匹配/不匹配两分支）
void CommonEventSubscriberFuzzTest(FuzzedDataProvider &fdp)
{
    int callbackCount = 0;
    AAFwk::Want want;
    want.SetAction(fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_ACTION_MAX));
    bool actionMatch = fdp.ConsumeBool();
    if (!actionMatch) {
        want.SetAction(fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_ACTION_MAX)); // 换串 → 不匹配分支
    }
    std::string ctorAction = want.GetAction(); // 匹配分支：与 want 同串，保证 callback 触发

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(fdp.ConsumeRandomLengthString(DEVICE_BATTERY_FUZZ_ACTION_MAX));
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    SleCommonEventSubscriber subscriber(subscribeInfo, ctorAction,
        [&callbackCount](const EventFwk::CommonEventData &data) {
            callbackCount++;
            HILOGI("devicebattery_fuzzer subscriber callback, count(%{public}d)", callbackCount);
        });

    EventFwk::CommonEventData data;
    data.SetWant(want);
    data.SetCode(fdp.ConsumeIntegral<int32_t>());
    subscriber.OnReceiveEvent(data); // 匹配 → callback_；不匹配 → 仅日志
    HILOGI("devicebattery_fuzzer OnReceiveEvent done, callbackCount(%{public}d)", callbackCount);

    std::this_thread::sleep_for(std::chrono::milliseconds(DEVICE_BATTERY_FUZZ_DELAY_50_MS));
}
} // namespace

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    /* FDP 读序（单流；字符串自头部消费、整型自尾端消费；后续扩展只允许追加尾部，保旧 corpus 兼容）：
     * [尾1] batteryLevel: ReportDeviceBatteryInfo 电量值（含 0xFF→-1 无效电量语义邻域）
     * 段A BatteryManagerFuzzTest：publishAddr(6B)+电量6字段+stateAddrValid(1B)+stateAddr
     *   +connState(1B)+activeAddrValid(1B)+activeAddr+reportAddrValid(1B)+reportAddr
     * 段B CommonEventSubscriberFuzzTest：want 首串(头部长度前缀)+actionMatch(1B)
     *   [+不匹配时第二串]+skills 串(头部)+eventCode(4B) */
    int32_t batteryLevel = static_cast<int32_t>(fdp.ConsumeIntegral<uint8_t>());
    OHOS::BatteryManagerFuzzTest(fdp, batteryLevel);
    OHOS::CommonEventSubscriberFuzzTest(fdp);
    return 0;
}
} // namespace OHOS
