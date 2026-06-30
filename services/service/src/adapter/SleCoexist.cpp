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
#include "SleCoexistManager.h"
#include "SleUtils.h"
#include "ThreadUtil.h"
#include "cm_api.h"

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
    (void)memcpy_s(&info.addr, sizeof(SLE_Addr_S), &param.addr, sizeof(SLE_Addr_S));

    SleCoexistManager::GetInstance()->UpdateConnectionInfo(info);

    auto* manager = SleCoexistManager::GetInstance();
    NL_CHECK_RETURN(manager, "manager is null");
    // 检查是否有多个连接
    if (manager->HasMultipleConnections() && info.interval < CM_CONN_COEXIST_INTERAL_THRED) {
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
    (void)memcpy_s(&info.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    SleCoexistManager::GetInstance()->UpdateConnectionInfo(info);
}

void SleCoexist::UpdateConnParam()
{
    HILOGD("[SleCoexist] Update params");
    auto* manager = SleCoexistManager::GetInstance();
    NL_CHECK_RETURN(manager, "manager is null");
    // 检查是否有多个连接
    if (!manager->HasMultipleConnections()) {
        HILOGD("[SleCoexist]not multiple connection");
        return;
    }
    // 遍历所有连接，更新需要调整的参数
    manager->IterateConnInfo([sleCoexist = selfWeak_.lock()](const CoexistConnInfo& info) {
        NL_CHECK_RETURN(sleCoexist, "sleCoexist is null");
        if (info.interval < CM_CONN_COEXIST_INTERAL_THRED) {
            sleCoexist->SendConnectionParam(info.timeout, info.latency, info.addr);
        }
    });
}

void SleCoexist::SendConnectionParam(uint16_t timeout, uint16_t latency, const SLE_Addr_S& addr)
{
    CM_ConnectUpdateParamReq_S updateParam;
    memset_s(&updateParam, sizeof(updateParam), 0x0, sizeof(updateParam));
    memcpy_s(&updateParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    updateParam.intervalMin = CM_CONN_COEXIST_INTERAL;
    updateParam.intervalMax = CM_CONN_COEXIST_INTERAL;
    updateParam.txRxInterval = CM_CONN_EVENT_IFS;
    updateParam.eventInterval = CM_CONN_EVENT_IFS;
    updateParam.maxLatency = latency;
    updateParam.supervisionTimeout = timeout;
    updateParam.systemTimeUnit = CM_CONN_TIME_UNIT;
    updateParam.txRxFlag = CM_CONN_T_TX_RX_FLAG;
    CM_ConnectUpdateParamReq(&updateParam);
}

} // namespace Nearlink
} // namespace OHOS
