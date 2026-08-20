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

#include "SleFrame4AntennaMgr.h"
#include "IServiceManagerPlugin.h"
#include "ThreadUtil.h"
#include "nearlink_def_types.h"
#include "nlstk_devd_scan_type_ext.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

SleFrame4AntennaMgr &SleFrame4AntennaMgr::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static SleFrame4AntennaMgr instance;
    return instance;
}

void SleFrame4AntennaMgr::OnAdvStarted(uint8_t advHandle, uint8_t primaryFrameType)
{
    DoInAntennaThread([this, advHandle, primaryFrameType]() {
        OnAdvStartedInner(advHandle, primaryFrameType);
    });
}

void SleFrame4AntennaMgr::OnAdvRemoved(uint8_t advHandle)
{
    DoInAntennaThread([this, advHandle]() {
        OnAdvRemovedInner(advHandle);
    });
}

void SleFrame4AntennaMgr::OnScanStarted(uint32_t scannerId, int scanMode, uint8_t frameType)
{
    DoInAntennaThread([this, scannerId, scanMode, frameType]() {
        OnScanStartedInner(scannerId, scanMode, frameType);
    });
}

void SleFrame4AntennaMgr::OnScanStopped(uint32_t scannerId)
{
    DoInAntennaThread([this, scannerId]() {
        OnScanStoppedInner(scannerId);
    });
}

void SleFrame4AntennaMgr::OnAllScanStopped()
{
    DoInAntennaThread([this]() {
        OnAllScanStoppedInner();
    });
}

void SleFrame4AntennaMgr::OnAdvStartedInner(uint8_t advHandle, uint8_t primaryFrameType)
{
    if (primaryFrameType != static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4)) {
        return;
    }
    if (!frame4AdvHandles_.insert(advHandle).second) {
        HILOGI("advHandle %{public}u already counted, advCount=%{public}d", advHandle, frame4AdvCount_);
        return;
    }
    frame4AdvCount_++;
    HILOGI("frame4 adv started, advHandle=%{public}u, advCount=%{public}d", advHandle, frame4AdvCount_);
    if (frame4AdvCount_ == 1) {
        ServiceManagerPluginInterface::GetInstance()->ControlAntennaFix(true,
            AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_ADV);
    }
}

void SleFrame4AntennaMgr::OnAdvRemovedInner(uint8_t advHandle)
{
    if (frame4AdvHandles_.erase(advHandle) == 0) {
        return;
    }
    frame4AdvCount_--;
    HILOGI("frame4 adv removed, advHandle=%{public}u, advCount=%{public}d", advHandle, frame4AdvCount_);
    if (frame4AdvCount_ == 0) {
        ServiceManagerPluginInterface::GetInstance()->ControlAntennaFix(false,
            AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_ADV);
    }
}

void SleFrame4AntennaMgr::OnScanStartedInner(uint32_t scannerId, int scanMode, uint8_t frameType)
{
    if (scanMode == SCAN_MODE_MONITOR) {
        // MONITOR模式协议栈会复位该scanner的扫描参数，视为停止该scanner的帧4扫描
        OnScanStoppedInner(scannerId);
        return;
    }
    if (frameType != static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4)) {
        // 非帧4扫描重启，若该scanner此前为帧4扫描则移出计数
        OnScanStoppedInner(scannerId);
        return;
    }
    if (!frame4ScannerIds_.insert(scannerId).second) {
        HILOGI("scannerId %{public}u already counted, scanCount=%{public}d", scannerId, frame4ScanCount_);
        return;
    }
    frame4ScanCount_++;
    HILOGI("frame4 scan started, scannerId=%{public}u, scanCount=%{public}d", scannerId, frame4ScanCount_);
    if (frame4ScanCount_ == 1) {
        ServiceManagerPluginInterface::GetInstance()->ControlAntennaFix(true,
            AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_SCAN);
    }
}

bool SleFrame4AntennaMgr::RemoveScanner(uint32_t scannerId)
{
    if (frame4ScannerIds_.erase(scannerId) == 0) {
        return false;
    }
    frame4ScanCount_--;
    HILOGI("frame4 scan stopped, scannerId=%{public}u, scanCount=%{public}d", scannerId, frame4ScanCount_);
    return true;
}

void SleFrame4AntennaMgr::OnScanStoppedInner(uint32_t scannerId)
{
    if (RemoveScanner(scannerId) && (frame4ScanCount_ == 0)) {
        ServiceManagerPluginInterface::GetInstance()->ControlAntennaFix(false,
            AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_SCAN);
    }
}

void SleFrame4AntennaMgr::OnAllScanStoppedInner()
{
    if (frame4ScannerIds_.empty()) {
        return;
    }
    HILOGI("all scan stopped, frame4 scanner num=%{public}zu", frame4ScannerIds_.size());
    frame4ScanCount_ -= static_cast<int32_t>(frame4ScannerIds_.size());
    frame4ScannerIds_.clear();
    if (frame4ScanCount_ == 0) {
        ServiceManagerPluginInterface::GetInstance()->ControlAntennaFix(false,
            AntennaFixScene::ANTENNA_FIX_SCENE_FRAME4_SCAN);
    }
}

}  // namespace Nearlink
}  // namespace OHOS
