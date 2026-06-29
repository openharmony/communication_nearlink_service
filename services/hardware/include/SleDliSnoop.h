/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef SLE_DLI_SNOOP_H
#define SLE_DLI_SNOOP_H

#pragma once

#include <stdint.h>
#include <fstream>
#include <string>
#include <vector>
#include <atomic>
#include <sys/time.h>
#include <sys/stat.h>

using ThreadUtilFunc = std::function<void(void)>;

typedef struct SnoopFileInfo {
    std::string path = "";
    time_t modifyTime;
    uint32_t fileSize = 0;
} SnoopFileInfo;

typedef struct SnoopHeader {
    uint64_t timestamp;
    bool isReceived;
} SnoopHeader;

class SleDliSnoop {
public:
    SleDliSnoop()
    {}
    ~SleDliSnoop()
    {}
    static SleDliSnoop &GetInstance();

    void SnoopStartUp();
    void SnoopShutDown();
    void DliSnoopCapture(uint32_t packetType, const std::vector<uint8_t> &data, bool isReceived);
    void CreateSnoopFile(bool isNewTimeNeeded);

private:
    void DoInSnoopThread(const ThreadUtilFunc &func);
    void SnoopStartUpTask();
    void SnoopShutDownTask();
    SnoopFileInfo CreateSnoopFileTask(bool isNewTimeNeeded);
    void DliSnoopCaptureTask(std::vector<uint8_t> &buffer, bool isReceived);
    void UpdateFilesQueue();
    void AddFileInfo(const std::string &fileName, const struct stat &fileStat);
    void RemoveSnoopFiles(const uint32_t &numToDelete);
    void OpenSnoopFile();
    void UpdateLogging();
    void CheckAndRemoveFiles();
    bool AssignSnoopHeader(std::vector<uint8_t> &buffer, bool isReceived);
    void SnoopWritePacket(std::string &packetFormatedStr);
    void SnoopWriteLogHexStr(const std::vector<uint8_t> &buffer);
    void CheckFileExist();

    std::atomic_bool isCommercialVersion_ = false;
    std::string fileNameEnableTimeStr_ = ""; // 文件名中携带的星闪adapter enable时间
    std::string snoopLogfilePath_ = "";
    bool isModuleStarted_ = false;
    bool isLogging_ = false;
    int logFileFd_ = -1; // -1表示INVALID_FD
    std::vector<SnoopFileInfo> files_;
    uint64_t currentFileSize_ = 0;
    uint64_t totalFilesSize_ = 0;
};

#endif // SLE_DLI_SNOOP_H