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
#ifndef SLE_DLI_SNOOP_H
#define SLE_DLI_SNOOP_H

#pragma once

#include <stdint.h>
#include <fstream>
#include <set>
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
    void RegisterSensitiveOpcodes(const uint16_t *cmdOpcodes, uint32_t cmdNum,
        const uint16_t *evtOpcodes, uint32_t evtNum);

private:
    void DoInSnoopThread(const ThreadUtilFunc &func);
    void SnoopStartUpTask();
    void SnoopShutDownTask();
    SnoopFileInfo CreateSnoopFileTask(bool isNewTimeNeeded);
    void DliSnoopCaptureTask(std::vector<uint8_t> &buffer, bool isReceived);
    void AnonymizeSnoopData(std::vector<uint8_t> &buffer);
    // 匿名化处理：按报文类型分发，各处理函数仅在命中黑名单时截断buffer
    void AnonymizeCmdData(std::vector<uint8_t> &buffer);
    void AnonymizeEventData(std::vector<uint8_t> &buffer);
    void AnonymizeEventCarriedCmdData(std::vector<uint8_t> &buffer); // status/complete事件携带的被响应指令
    void AnonymizeAcbData(std::vector<uint8_t> &buffer); // ACB/ICB业务数据
    bool IsSensitiveCmdOpcode(uint16_t opcode);
    bool IsSensitiveEventOpcode(uint16_t event);
    bool IsExtSensitiveOpcode(uint16_t opcode, bool isCmd);
    void RegisterSensitiveOpcodesTask(const std::vector<uint16_t> &cmdOpcodes,
        const std::vector<uint16_t> &evtOpcodes); // 队列内执行体：扩展黑名单全量替换
    bool IsSnoopAnonymizationEnabled();
    void WatchRemoteLogChange();
    void UnWatchRemoteLogChange();
    static void OnRemoteLogChange(const char *key, const char *value, void *context);
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
    std::set<uint16_t> extSensitiveCmdOpcodes_; // 扩展敏感指令集，读写均在sle_dli队列内，无需锁
    std::set<uint16_t> extSensitiveEvtOpcodes_; // 扩展敏感事件集
    std::string fileNameEnableTimeStr_ = ""; // 文件名中携带的星闪adapter enable时间
    std::string snoopLogfilePath_ = "";
    bool isModuleStarted_ = false;
    std::atomic_bool isLogging_ = false; // snoop落盘总开关，跟随模块启停（捕获线程读/snoop线程写）
    std::atomic_bool isAnonymized_ = true; // snoop数据匿名化开关
    std::atomic_bool isRemoteLogWatched_ = false; // 远程诊断开关监听是否已注册
    int logFileFd_ = -1; // -1表示INVALID_FD
    std::vector<SnoopFileInfo> files_;
    uint64_t currentFileSize_ = 0;
    uint64_t totalFilesSize_ = 0;
};

#endif // SLE_DLI_SNOOP_H