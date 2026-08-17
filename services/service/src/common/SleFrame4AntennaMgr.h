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

#ifndef SLE_FRAME4_ANTENNA_MGR_H
#define SLE_FRAME4_ANTENNA_MGR_H

#include <cstdint>
#include <set>
#include "BaseDef.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief 帧4广播/帧4扫描启停计数管理器。
 * 帧4广播与帧4扫描各自维护独立的引用计数：
 * 广播计数首次从0变为1时，通过插件固定天线（ADV场景）；广播计数最后归零时，通过插件解锁天线（ADV场景）。
 * 扫描计数首次从0变为1时，通过插件固定天线（SCAN场景）；扫描计数最后归零时，通过插件解锁天线（SCAN场景）。
 * 两个计数互不影响。所有状态访问与插件调用均投递到antenna串行队列执行，保证事件按序处理，无需加锁。
 */
class SleFrame4AntennaMgr {
public:
    SleFrame4AntennaMgr() = default;
    ~SleFrame4AntennaMgr() = default;
    static SleFrame4AntennaMgr &GetInstance();

    /**
     * @brief 广播启动通知，帧4广播计入计数。
     *
     * @param advHandle 广播句柄。
     * @param primaryFrameType 主广播帧类型，参考SleAdvertiserPrimaryFrameType。
     */
    void OnAdvStarted(uint8_t advHandle, uint8_t primaryFrameType);

    /**
     * @brief 广播移除通知，已计数的帧4广播移出计数。
     *
     * @param advHandle 广播句柄。
     */
    void OnAdvRemoved(uint8_t advHandle);

    /**
     * @brief 扫描启动通知，帧4扫描计入计数。
     *
     * @param scannerId 扫描者ID。
     * @param scanMode 扫描模式，参考NLSTK_DevdScanMode_E，MONITOR模式视为停止该scanner的帧4扫描。
     * @param frameType 扫描帧类型，参考SleScanFrameType。
     */
    void OnScanStarted(uint32_t scannerId, int scanMode, uint8_t frameType);

    /**
     * @brief 扫描停止通知，已计数的帧4扫描移出计数。
     *
     * @param scannerId 扫描者ID。
     */
    void OnScanStopped(uint32_t scannerId);

    /**
     * @brief 停止所有扫描通知，全部帧4扫描移出计数。
     */
    void OnAllScanStopped();

private:
    void OnAdvStartedInner(uint8_t advHandle, uint8_t primaryFrameType);
    void OnAdvRemovedInner(uint8_t advHandle);
    void OnScanStartedInner(uint32_t scannerId, int scanMode, uint8_t frameType);
    void OnScanStoppedInner(uint32_t scannerId);
    void OnAllScanStoppedInner();
    bool RemoveScanner(uint32_t scannerId);

    int32_t frame4AdvCount_ = 0;
    int32_t frame4ScanCount_ = 0;
    std::set<uint8_t> frame4AdvHandles_ {};
    std::set<uint32_t> frame4ScannerIds_ {};
    SLE_DISALLOW_COPY_AND_ASSIGN(SleFrame4AntennaMgr);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_FRAME4_ANTENNA_MGR_H
