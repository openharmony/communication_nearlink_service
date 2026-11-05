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
#include "SleCoexist.h"
#include "IServiceManagerPlugin.h"
#include "SleCoexistManager.h"
#include "SleRemoteDeviceManager.h"
#include "SleUtils.h"
#include "ThreadUtil.h"
#include "cm_api.h"
#include "interface_scan_service.h"
#include "nearlink_def.h"
#include "raw_address.h"
#ifdef NEARLINK_POWER_MGR_ENABLE
#include "power_mgr_client.h"
#endif

namespace OHOS {
namespace Nearlink {

SleCoexist::SleCoexist()
{}

SleCoexist::~SleCoexist()
{
    StopParamUpdateTimer();
}

void SleCoexist::Init()
{
    HILOGD("[SleCoexist] Init");
    selfWeak_ = shared_from_this();
}

void SleCoexist::UpdateTimerCallback(std::weak_ptr<SleCoexist> sleCoexistImpl)
{
    DoInAdapterThread([sleCoexist = sleCoexistImpl.lock()]() {
        NL_CHECK_RETURN(sleCoexist, "sleCoexist is null");
        sleCoexist->UpdateConnParam();
    });
}

void SleCoexist::StartParamUpdateTimer()
{
    HILOGD("[SleCoexist] Start timer");
    StopParamUpdateTimer();
    if (updateDuration_ > 0) {
        if (updateTimer_ == nullptr) {
            updateTimer_ = std::make_unique<NearlinkTimer>(
                std::bind(&UpdateTimerCallback, selfWeak_));
        }
        updateTimer_->Start(updateDuration_);
    }
}

void SleCoexist::StopParamUpdateTimer()
{
    HILOGD("[SleCoexist] Stop timer");
    if (updateTimer_ != nullptr) {
        updateTimer_->Stop();
        updateTimer_ = nullptr;
    }
}

void SleCoexist::ConnectionParamChanged(const CM_ConnectUpdateParamRsp_S &param)
{
    HILOGD("[SleCoexist] ConnectionParamChanged lcid=0x%{public}x, interval=0x%{public}x, latency=%{public}d,"
        " timeout=%{public}d", param.lcid,
        param.extension.interval, param.extension.latency, param.extension.supervisionTimeout);
    NL_CHECK_RETURN(param.lcid < INVALID_LCID, "invalid icid");
    CoexistConnInfo info;
    info.lcid = param.lcid;
    info.interval = param.extension.interval;
    info.latency = param.extension.latency;
    info.timeout = param.extension.supervisionTimeout;
    info.intervalKnown = true;
    (void)memcpy_s(&info.addr, sizeof(SLE_Addr_S), &param.addr, sizeof(SLE_Addr_S));

    SleCoexistManager::GetInstance()->UpdateConnectionInfo(info);

    auto* manager = SleCoexistManager::GetInstance();
    NL_CHECK_RETURN(manager, "manager is null");
    int adjustDevCount = manager->CountCoexistAdjustDevices(CM_CONN_COEXIST_INTERAL_THRED);

    // 检查是否有多个连接
   RefreshAppearanceAndCustomFlag(info);
    bool isScreenOn = true;
#ifdef NEARLINK_POWER_MGR_ENABLE
    isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
#endif
    HILOGI("[SleCoexist] isScreen=%{public}d, isNeedCustomParam_=%{public}d, adjustDevCount=%{public}d",
        isScreenOn, isNeedCustomParam_, adjustDevCount);
    if (adjustDevCount >= CM_CONN_COEXIST_MIN_ADJUST_DEV_NUM || (isNeedCustomParam_ && isScreenOn)) {
        StartParamUpdateTimer();
    }
}

void SleCoexist::ConnectionStatusChanged(uint16_t lcid, const SLE_Addr_S &addr)
{
    CoexistConnInfo info;
    info.lcid = lcid;
    info.interval = CM_CONN_PRIVATE_MIN_INTERVAL;
    info.latency = CM_CONN_DEFAULT_LATENCY;
    info.timeout = CM_CONN_PRIVATE_TIMEOUT;
    info.intervalKnown = false; // 建链协商间隔不上报，实际间隔待参数更新事件确认
    (void)memcpy_s(&info.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    SleCoexistManager::GetInstance()->UpdateConnectionInfo(info);

    auto* manager = SleCoexistManager::GetInstance();
    NL_CHECK_RETURN(manager, "manager is null");
    // 新连接可能改变共存情况：已有多个连接时需要评估调整
    HILOGI("[SleCoexist] ConnectionStatusChanged lcid=0x%{public}x, connCount=%{public}d",
        lcid, manager->GetConnectionCount());
    if (manager->HasMultipleConnections()) {
        StartParamUpdateTimer();
    }
}

void SleCoexist::UpdateConnParam()
{
    HILOGD("[SleCoexist] Update params");
    auto* manager = SleCoexistManager::GetInstance();
    NL_CHECK_RETURN(manager, "manager is null");
    // 检查是否有多个连接
    if (!(manager->HasMultipleConnections() || isNeedCustomParam_)) {
        HILOGD("[SleCoexist]not multiple connection");
        return;
    }
    // 规则1/规则2:间隔未知或已知间隔小于等于4ms的设备数达到2个及以上，才将此类设备调整为4ms
    // 建链时若已有一个其他连接，新设备间隔未知也需按策略分配，避免2ms设备长期不调整
    int adjustDevCount = manager->CountCoexistAdjustDevices(CM_CONN_COEXIST_INTERAL_THRED);
    bool isNeedAdjustInterval = adjustDevCount >= CM_CONN_COEXIST_MIN_ADJUST_DEV_NUM;
    // 规则3：芯片仅允许3个4ms高速连接，已有3个设备小于4ms（占满4+4+4）后，
    // 第4个起间隔小于7.5ms的设备需调整为7.5ms
    bool isNeedAdjustExt = adjustDevCount >= CM_CONN_COEXIST_HIGH_SPEED_DEV_NUM
    int deviceCount = 0;
    manager->IterateConnInfo(
        [sleCoexist = selfWeak_.lock(), &deviceCount, isNeedAdjustInterval, isNeedAdjustExt](
            const CoexistConnInfo& info) {
        NL_CHECK_RETURN(sleCoexist, "sleCoexist is null");
        sleCoexist->RefreshAppearanceAndCustomFlag(info);
        bool isFastSlotDevice = !info.intervalKnown || info.interval <= CM_CONN_COEXIST_INTERAL_THRED;
        if (!isFastSlotDevice) {
            sleCoexist->UpdateCustomParamDevice(info);
            return;
        }
        sleCoexist->UpdateFastSlotDeviceParam(info, deviceCount, isNeedAdjustInterval, isNeedAdjustExt);
        deviceCount++;
    });
}

void SleCoexist::RefreshAppearanceAndCustomFlag(const CoexistConnInfo& info)
{
    RawAddress device = RawAddress::ConvertToString(info.addr.addr);
    appearance_ = SleRemoteDeviceManager::GetInstance()->GetDeviceAppearance(device);
    if (appearance_ == INVALID_APPEARANCE) {
        appearance_ = InterfaceScanService::GetInstance().GetDeviceAppearance(device.GetAddress());
    }
    ServiceManagerPluginInterface::GetInstance()->IsNeedCustomParam(isNeedCustomParam_, appearance_, info.interval);
}

void SleCoexist::UpdateCustomParamDevice(const CoexistConnInfo& info)
{
    if (!isNeedCustomParam_) {
        return;
    }
    SendConnectionParam(static_cast<uint16_t>(CM_CONN_COEXIST_INTERAL), info.timeout, info.latency, info.addr);
}

void SleCoexist::UpdateFastSlotDeviceParam(const CoexistConnInfo& info, int deviceCount,
        bool isNeedAdjustInterval, bool isNeedAdjustExt)
{
    // 规则3：已有3个设备小于4ms时，第4个起小于7.5ms的设备最高只能用7.5ms
    if (deviceCount >= CM_CONN_COEXIST_HIGH_SPEED_DEV_NUM) {
        if (isNeedAdjustExt && (!info.intervalKnown || info.interval != CM_CONN_COEXIST_INTERAL_EXT)) {
            SendConnectionParam(static_cast<uint16_t>(CM_CONN_COEXIST_INTERAL_EXT), 
                info.timeout, info.latency, info.addr)
        }
        return;
    }
    

    uint16_t targetInterval = (deviceCount < CM_CONN_COEXIST_HIGH_SPEED_DEV_NUM)
        ? static_cast<uint16_t>(CM_CONN_COEXIST_INTERAL)
        : static_cast<uint16_t>(CM_CONN_COEXIST_INTERAL_EXT);
    if (info.intervalKnown && targetInterval == info.interval) {
        return;
    }
    SendConnectionParam(targetInterval, info.timeout, info.latency, info.addr);
}

void SleCoexist::SendConnectionParam(uint16_t interval, uint16_t timeout, uint16_t latency, const SLE_Addr_S& addr)
{
    CM_ConnectUpdateParamReq_S updateParam;
    memset_s(&updateParam, sizeof(updateParam), 0x0, sizeof(updateParam));
    memcpy_s(&updateParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    updateParam.intervalMin = interval;
    updateParam.intervalMax = interval;
    updateParam.txRxInterval = CM_CONN_EVENT_IFS;
    updateParam.eventInterval = CM_CONN_EVENT_IFS;
    updateParam.maxLatency = latency;
    updateParam.supervisionTimeout = timeout;
    updateParam.systemTimeUnit = CM_CONN_TIME_UNIT;
    updateParam.txRxFlag = CM_CONN_T_TX_RX_FLAG;
    if (isNeedCustomParam_) {
        ServiceManagerPluginInterface::GetInstance()->UpdateCustomParam(updateParam.intervalMin,
            updateParam.intervalMax, appearance_);
    }
    CM_ConnectUpdateParamReq(&updateParam);
}

} // namespace Nearlink
} // namespace OHOS
