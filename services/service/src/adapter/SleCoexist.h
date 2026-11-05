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
#ifndef SLE_COEXSIT_H
#define SLE_COEXSIT_H

#include "BaseDef.h"
#include "nearlink_timer.h"
#include "SleDefs.h"
#include "cm.h"
#include "cm_api.h"
#include "SleServiceFfrtLog.h"

/*
 * @brief The nearlink system.
 */
namespace OHOS {
namespace Nearlink {

struct CoexistConnInfo;

/*
 * @brief SLE 共存控制适配器
 *
 * 负责定时器管理和触发共存参数更新
 * 数据管理已迁移到 SleCoexistManager
 */
class SleCoexist : public std::enable_shared_from_this<SleCoexist> {
public:
    /**
     * @brief Constructor.
     */
    SleCoexist();

    /**
     * @brief Destructor.
     */
    virtual ~SleCoexist();

    // 初始化
    void Init();

    void ConnectionParamChanged(const CM_ConnectUpdateParamRsp_S &param);
    void ConnectionStatusChanged(uint16_t lcid, const SLE_Addr_S &addr);

private:
    // 定时器回调
    static void UpdateTimerCallback(std::weak_ptr<SleCoexist> sleCoexistImpl);

    // 执行共存参数更新 (调用 Manager)
    void UpdateConnParam();

    // 刷新设备外观及自定义参数标记
    void RefreshAppearanceAndCustomFlag(const CoexistConnInfo& info);

    // 非调速设备仅需自定义参数时的处理
    void UpdateCustomParamDevice(const CoexistConnInfo& info);

    // 占用高速槽位的设备（间隔未知或小于7.5ms）按槽位分配目标间隔
    void UpdateFastSlotDeviceParam(const CoexistConnInfo& info, int deviceCount,
        bool isNeedAdjustInterval, bool isNeedAdjustExt);

    // 发送连接参数到底层
    void SendConnectionParam(uint16_t interval, uint16_t timeout, uint16_t latency, const SLE_Addr_S& addr);

    // 定时器控制
    void StartParamUpdateTimer();
    void StopParamUpdateTimer();

    SLE_DISALLOW_COPY_AND_ASSIGN(SleCoexist);

    std::unique_ptr<NearlinkTimer> updateTimer_ = nullptr;
    int updateDuration_ = CM_CONN_UPDATE_DURATION; // ms
    std::weak_ptr<SleCoexist> selfWeak_;
    bool isNeedCustomParam_ = false;
    int appearance_ = INVALID_APPEARANCE;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_COEXSIT_H
