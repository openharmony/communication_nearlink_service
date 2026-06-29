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

#include "SleDliSnoop.h"

#include <string>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <charconv>
#include <iostream>
#include <chrono>
#include <sstream>
#include <dirent.h>
#include "parameters.h"
#include "SleDliLayerAdapter.h"
#include "SleDliThreadUtil.h"
#include "log.h"

namespace fs = std::filesystem;

namespace {
    constexpr uint32_t PROPERTY_VALUE_MAX = 128;
    const std::string VERSION_TYPE_KEY = "const.logsystem.versiontype";
    const std::string INVALID_COMMERCIAL_VERSION = "invalid";
    const std::string COMMERCIAL_VERSION = "commercial";
    constexpr uint32_t COMMERCIAL_VERSION_SIZE = 10;
    const std::string SNOOP_BASE_PATH = "/data/log/nearlink/";
    constexpr uint32_t MAX_SNOOP_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    constexpr uint32_t MAX_SNOOP_FILES_TOTAL_SIZE = 100 * 1024 * 1024; // 100MB
    constexpr uint32_t MAX_TOTAL_SNOOP_FILES = 100;
    constexpr size_t MAX_SNOOP_DATA_LEN = UINT16_MAX;
    constexpr size_t MAX_SNOOP_LOG_LEN = 200; // 单条日志能显示的码流最大长度（字节）
    constexpr uint32_t FILES_NUM_TO_DELETE = 1; // 文件数量超出限制时，每次删除的旧文件个数
    const std::string SNOOP_FILE_BASE_NAME = "nearlink_dli_";
    const std::string SNOOP_FILE_TIME_DEFAULT = "00000000-000000"; // YYYYmmdd-HHMMSS 年月日-时分秒
    const std::string SNOOP_FILE_NAME_TAIL = ".log";
    const std::string SNOOP_TO_ADD_AFTER_CUT = "..."; // 单条日志超码流截断后，后缀增加内容
    constexpr uint32_t SNOOP_FILE_TIME_LENGTH = 15;
    constexpr uint32_t SNOOP_FILE_NAME_TAIL_LENGTH = 4; // 文件名后缀：.log
    constexpr uint32_t SNOOP_HEADER_LENGTH = 9; // SnoopHeader的长度
    constexpr size_t SPACE_CHAR_OFFSET_ONE = 16; // 时间戳后加入的空格的偏移
    constexpr size_t SPACE_CHAR_OFFSET_TWO = 19; // 方向标志后加入的空格的偏移
    constexpr int INVALID_FD = -1;

    enum class DliSnoopType : uint8_t {
        DLI_SNOOPTYPE_CMD = 0xA1,
        DLI_SNOOPTYPE_EVENT = 0xA2,
        DLI_SNOOPTYPE_ACB = 0xA3,
        DLI_SNOOPTYPE_ICB = 0xA4,
    };

    bool IsVendorCommercialVersion()
    {
        std::string versionValue = OHOS::system::GetParameter(VERSION_TYPE_KEY, INVALID_COMMERCIAL_VERSION);
        if (versionValue == INVALID_COMMERCIAL_VERSION) {
            HILOGE("failed to get nearlink switch_enable parameter");
            return true; // 默认返回是商用版本
        }
        bool isCommercial = (versionValue == COMMERCIAL_VERSION ? true : false);
        HILOGI("IsVendorCommercialVersion: %{public}s", isCommercial ? "true" : "false");
        return isCommercial;
    }

    uint64_t GetMillTimestamp()
    {
        struct timespec ts = {0};
        int ret = 0;
        ret = clock_gettime(CLOCK_REALTIME, &ts);
        NL_CHECK_RETURN_RET(ret == 0, 0, "clock_gettime failed, err: %{public}d", ret);
        uint64_t startTime = static_cast<uint64_t>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        return startTime;
    }

    std::string SnoopInsertSpaceChar(const std::string &str)
    {
        size_t maxLen = MAX_SNOOP_LOG_LEN * 2; // 每字节码流对应2个字符
        size_t strLen = str.length();
        if (strLen <= SPACE_CHAR_OFFSET_TWO) {
            HILOGD("invalid strLen: %{public}zu", strLen);
            return "";
        }

        std::string formatedStr = str;
        formatedStr.insert(SPACE_CHAR_OFFSET_ONE, " "); // 时间戳后面插入空格
        formatedStr.insert(SPACE_CHAR_OFFSET_TWO, " "); // 方向标志后面插入空格
        return formatedStr;
    }

    std::string GetCurrentTimeStr()
    {
        HILOGI("enter");
        std::time_t now = std::time(nullptr);
        struct std::tm localTime;
        NL_CHECK_RETURN_RET(localtime_r(&now, &localTime) != nullptr, "", "get local time failed");
        std::stringstream ss;
        ss << std::put_time(&localTime, "%Y%m%d-%H%M%S");
        std::string timeStr = ss.str();
        NL_CHECK_RETURN_RET(timeStr.length() == SNOOP_FILE_TIME_LENGTH, SNOOP_FILE_TIME_DEFAULT,
            "invalid time string length: %{public}zu", timeStr.length());
        return timeStr;
    }
} // namespace

SleDliSnoop &SleDliSnoop::GetInstance()
{
    static SleDliSnoop instance;
    return instance;
}

void SleDliSnoop::DoInSnoopThread(const ThreadUtilFunc &func)
{
    SleDliThreadUtil::GetInstance().PostTask(func);
}

void SleDliSnoop::SnoopStartUp()
{
    HILOGI("enter");
    bool isCommercialVersion = IsVendorCommercialVersion();
    isCommercialVersion_.store(isCommercialVersion);
    if (isCommercialVersion) {
        HILOGW("Commercial Version, dli snoop is unavailable");
        return;
    }

    DoInSnoopThread([this]() -> void {
        SnoopStartUpTask();
    });
}

void SleDliSnoop::SnoopStartUpTask()
{
    HILOGI("enter");
    isModuleStarted_ = true;
    UpdateLogging();
}

void SleDliSnoop::SnoopShutDown()
{
    HILOGI("enter");
    if (isCommercialVersion_.load()) {
        HILOGW("Commercial Version, dli snoop is unavailable");
        return;
    }

    DoInSnoopThread([this]() -> void {
        SnoopShutDownTask();
    });
}

void SleDliSnoop::SnoopShutDownTask()
{
    HILOGI("enter");
    if (logFileFd_ != INVALID_FD) {
        int ret = close(logFileFd_);
        if (ret == -1) {
            HILOGE("close file fail, errno:%{public}s", strerror(errno));
        }
    }
    logFileFd_ = INVALID_FD;

    isModuleStarted_ = false;
    UpdateLogging();
}

void SleDliSnoop::UpdateFilesQueue()
{
    HILOGI("enter");
    totalFilesSize_ = 0;
    files_.clear();

    struct stat fileStat;
    DIR* dirP = opendir(SNOOP_BASE_PATH.c_str());
    NL_CHECK_RETURN(dirP != nullptr, "unable to open %{public}s", SNOOP_BASE_PATH.c_str());
    struct dirent* direntP = readdir(dirP);

    while (direntP != nullptr) {
        // exclude file '.' and '..'
        if (direntP->d_name[0] == '.') {
            direntP = readdir(dirP);
            continue;
        }

        std::string fileName = std::string(direntP->d_name);
        std::string filePath = SNOOP_BASE_PATH + fileName;
        if (stat(filePath.c_str(), &fileStat) == 0) {
            AddFileInfo(fileName, fileStat);
        } else {
            HILOGE("Get file status failed!: %{public}s", filePath.c_str());
        }

        // get next file structure
        direntP = readdir(dirP);
    }

    closedir(dirP);
    sort(files_.begin(), files_.end(), [](const SnoopFileInfo& a, const SnoopFileInfo& b) -> bool {
        return a.modifyTime < b.modifyTime;
    });
}

void SleDliSnoop::AddFileInfo(const std::string &fileName, const struct stat &fileStat)
{
    if (!S_ISREG(fileStat.st_mode)) { // 如果不是常规文件
        return;
    }
    if (fileName.find(SNOOP_FILE_BASE_NAME) == 0 && fileName.length() > SNOOP_FILE_NAME_TAIL_LENGTH &&
        fileName.substr(fileName.length() - SNOOP_FILE_NAME_TAIL_LENGTH) == SNOOP_FILE_NAME_TAIL) {
        SnoopFileInfo snoopFile;
        snoopFile.path = SNOOP_BASE_PATH + fileName;
        snoopFile.modifyTime = fileStat.st_mtime;
        snoopFile.fileSize = static_cast<uint32_t>(fileStat.st_size);
        totalFilesSize_ += snoopFile.fileSize;
        files_.push_back(snoopFile);
    }
}

void SleDliSnoop::RemoveSnoopFiles(const uint32_t &numToDelete)
{
    HILOGI("fizes_ (%{public}zu)", files_.size());
    NL_CHECK_RETURN(numToDelete > 0, "invalid numToDelete: %{public}d", numToDelete);
    uint32_t count = 0;
    while (count < numToDelete && !files_.empty()) {
        std::string filePath = files_.front().path;
        if (fs::exists(filePath)) {
            int ret = fs::remove(filePath);
            if (ret == 0) {
                HILOGI("remove file '%{public}s' success, files_.front().fileSize(%{public}u)",
                    filePath.c_str(), files_.front().fileSize);
            } else {
                HILOGE("remove file '%{public}s' fail, errno:%{public}s", filePath.c_str(), strerror(errno));
            }
            totalFilesSize_ -= files_.front().fileSize;
            ++count;
            files_.erase(files_.begin());
        } else {
            HILOGE("file '%{public}s' does not exist, files_.front().fileSize(%{public}u)",
                filePath.c_str(), files_.front().fileSize);
            totalFilesSize_ -= files_.front().fileSize;
            files_.erase(files_.begin());
        }
    }
}

void SleDliSnoop::CreateSnoopFile(bool isNewTimeNeeded)
{
    HILOGI("enter");
    if (isCommercialVersion_.load()) {
        HILOGW("Commercial Version, dli snoop is unavailable");
        return;
    }
   
    DoInSnoopThread([this, isNewTimeNeeded]() -> void {
        CreateSnoopFileTask(isNewTimeNeeded);
    });
}

SnoopFileInfo SleDliSnoop::CreateSnoopFileTask(bool isNewTimeNeeded)
{
    HILOGI("files_ size(%{public}zu)", files_.size());
    std::string fileNameCreateTimeStr = GetCurrentTimeStr();
    if (isNewTimeNeeded || fileNameEnableTimeStr_.empty()) {
        fileNameEnableTimeStr_ = fileNameCreateTimeStr; // 文件名中携带本次adapter enable时的时间
    }
    SnoopFileInfo newSnoopFile{};
    std::string newFileName =
        SNOOP_FILE_BASE_NAME + fileNameEnableTimeStr_ + "_" + fileNameCreateTimeStr + ".log";
    std::string newFilePath = SNOOP_BASE_PATH + newFileName;
    snoopLogfilePath_ = newFilePath;
    OpenSnoopFile();
    NL_CHECK_RETURN_RET(logFileFd_ != INVALID_FD, newSnoopFile, "OpenSnoopFile failed");
    struct stat fileStat;
    NL_CHECK_RETURN_RET(stat(newFilePath.c_str(), &fileStat) != -1, newSnoopFile, "get file stat failed");

    newSnoopFile.path = newFilePath;
    newSnoopFile.modifyTime = fileStat.st_mtime;
    newSnoopFile.fileSize = static_cast<uint32_t>(fileStat.st_size);
    currentFileSize_ = newSnoopFile.fileSize; // 初始都是0
    files_.push_back(newSnoopFile);
    CheckAndRemoveFiles();
    return newSnoopFile;
}

void SleDliSnoop::OpenSnoopFile()
{
    HILOGI("enter");
    NL_CHECK_RETURN(!snoopLogfilePath_.empty(), "SleDliSnoop path empty");
    if (logFileFd_ != INVALID_FD) {
        HILOGI("close file");
        close(logFileFd_);
        logFileFd_ = INVALID_FD;
    }

    mode_t prevmask = umask(0);
    // 打开方式：只写、文件不存在则创建、文件已存在则覆写；文件权限：rw-rw-r--
    logFileFd_ =
        open(snoopLogfilePath_.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (logFileFd_ < 0) {
        HILOGE("unable to open '%{public}s', err:%{public}s", snoopLogfilePath_.c_str(), strerror(errno));
        logFileFd_ = INVALID_FD;
        isLogging_ = false;
        umask(prevmask);
        return;
    }
    umask(prevmask);
}

void SleDliSnoop::UpdateLogging()
{
    HILOGI("enter");

    bool shouldLog = isModuleStarted_;
    if (shouldLog == isLogging_) {
        return;
    }
    isLogging_ = shouldLog;
    if (!shouldLog) {
        HILOGI("should not log");
        if (logFileFd_ != INVALID_FD) {
            HILOGI("close file");
            close(logFileFd_);
            logFileFd_ = INVALID_FD;
        }
        return;
    }

    UpdateFilesQueue();
    CheckAndRemoveFiles();
}

void SleDliSnoop::CheckAndRemoveFiles()
{
    HILOGI("files_ size(%{public}zu)), totalFilesSize_(%{public}lu)", files_.size(), totalFilesSize_);
    while (totalFilesSize_ > MAX_SNOOP_FILES_TOTAL_SIZE || files_.size() > MAX_TOTAL_SNOOP_FILES) {
        if (files_.size() == 0) {
            HILOGE("files_ is empty, totalFilesSize_(%{public}lu)", totalFilesSize_);
            break;
        }
        RemoveSnoopFiles(FILES_NUM_TO_DELETE);
    }
    return;
}

void SleDliSnoop::DliSnoopCapture(uint32_t packetType, const std::vector<uint8_t> &data, bool isReceived)
{
    if (isCommercialVersion_.load()) {
        return;
    }
    size_t dataLen = data.size();
    NL_CHECK_RETURN(dataLen > 0 && dataLen <= MAX_SNOOP_DATA_LEN, "invalid data length: %{public}zu", dataLen);
    uint8_t snoopType = static_cast<uint8_t>(packetType & 0xFF); // 32位SlePacketType转换为8位DLI_DATA_TYPE
    uint32_t snoopTypeOffset = SNOOP_HEADER_LENGTH;
    uint32_t dataOffset = isReceived? snoopTypeOffset + sizeof(snoopType) : snoopTypeOffset; // 收包需要在头部添加类型
    std::vector<uint8_t> buffer(dataOffset, 0);
    buffer.insert(buffer.end(), data.begin(), data.end());

    if (isReceived) {
        buffer[snoopTypeOffset] = snoopType; // 收包data头部不包含类型，需要手动添加
    }
    DoInSnoopThread([this, dataToBeHeaded = std::move(buffer), isReceived]() -> void {
        std::vector<uint8_t> dataBuffer(dataToBeHeaded);
        DliSnoopCaptureTask(dataBuffer, isReceived);
    });
}

void SleDliSnoop::DliSnoopCaptureTask(std::vector<uint8_t> &buffer, bool isReceived)
{
    HILOGD("enter");
    size_t buffLen = buffer.size();
    NL_CHECK_RETURN(buffLen > 0 && buffLen <= MAX_SNOOP_DATA_LEN, "invalid buffer length: %{public}zu", buffLen);
    NL_CHECK_RETURN(AssignSnoopHeader(buffer, isReceived), "AssignSnoopHeader failed");
    SnoopWriteLogHexStr(buffer);
}

bool SleDliSnoop::AssignSnoopHeader(std::vector<uint8_t> &buffer, bool isReceived)
{
    HILOGD("enter");
    uint32_t buffLen = buffer.size();
    NL_CHECK_RETURN_RET(buffLen > SNOOP_HEADER_LENGTH && buffLen <= MAX_SNOOP_DATA_LEN, false,
        "invalid buffer length: %{public}u", buffLen);

    uint64_t timestamp = GetMillTimestamp();
    uint32_t timestampLen = sizeof(timestamp);
    errno_t ret = memcpy_s(buffer.data(), timestampLen, &timestamp, timestampLen); // 填充时间戳
    NL_CHECK_RETURN_RET(ret == EOK, false,
        "timestamp memcpy failed, error code is %{public}d", static_cast<int>(ret));

    buffer[timestampLen] = static_cast<uint8_t>(isReceived & 0x01); // 填充数据方向标志
    return true;
}

void SleDliSnoop::SnoopWriteLogHexStr(const std::vector<uint8_t> &buffer)
{
    HILOGD("enter");
    const uint32_t buffLen = buffer.size();
    NL_CHECK_RETURN(buffLen > SNOOP_HEADER_LENGTH && buffLen <= MAX_SNOOP_DATA_LEN,
        "invalid buffer length: %{public}u", buffLen);
    char packetChars[MAX_SNOOP_DATA_LEN * 2 + 1] = { 0 }; // 每字节转换为2字符
    for (uint32_t i = 0; i < buffLen; ++i) {
        uint8_t temp = buffer[i];
        (void)sprintf_s(&packetChars[2 * i], (MAX_SNOOP_DATA_LEN - i) * 2, "%02x", temp); // 2 hex char
    }

    std::string packetStr = std::string(packetChars);
    std::string packetFormatedStr = SnoopInsertSpaceChar(packetStr);
    HILOGD("packetFormatedLen = %{public}zu, snoop packet = %{public}s",
        packetFormatedStr.length(), packetFormatedStr.c_str());
    SnoopWritePacket(packetFormatedStr);
}

void SleDliSnoop::SnoopWritePacket(std::string &packetFormatedStr)
{
    size_t strLen = packetFormatedStr.length();
    if (strLen <= SNOOP_HEADER_LENGTH) {
        HILOGD("invalid packet length: %{public}zu", strLen);
        return;
    } else if (strLen > MAX_SNOOP_LOG_LEN * 2) { // 2 hex char
        HILOGD("too long packet length: %{public}zu", strLen);
        packetFormatedStr = packetFormatedStr.substr(0, MAX_SNOOP_LOG_LEN * 2 // 2 hex char
            - SNOOP_TO_ADD_AFTER_CUT.length());
        packetFormatedStr += SNOOP_TO_ADD_AFTER_CUT;
    }
    CheckFileExist();
    NL_CHECK_RETURN(logFileFd_ != INVALID_FD, "logFileFd_ is INVALID_FD");

    packetFormatedStr.push_back('\n'); // 每包末尾添加换行符
    std::vector<uint8_t> packetFormatedBuff =
        std::vector<uint8_t>(packetFormatedStr.begin(), packetFormatedStr.end());
    uint32_t buffLen = packetFormatedBuff.size();
    if (currentFileSize_ + buffLen > MAX_SNOOP_FILE_SIZE) {
        SnoopFileInfo newSnoopFile = CreateSnoopFileTask(false);
        NL_CHECK_RETURN(!newSnoopFile.path.empty(), "CreateSnoopFile failed");
    }

    int ret = 0;
    ret = write(logFileFd_, packetFormatedBuff.data(), buffLen);
    NL_CHECK_RETURN(ret >= 0, "write failed, err:%{public}s", strerror(errno));
    uint32_t writeLen = static_cast<uint32_t>(ret);
    currentFileSize_ += writeLen;
    totalFilesSize_ += writeLen;
    if (!files_.empty() && snoopLogfilePath_ == files_.back().path) {
        files_.back().fileSize = currentFileSize_;
    }
}

void SleDliSnoop::CheckFileExist()
{
    struct stat fileStat;
    if (stat(snoopLogfilePath_.c_str(), &fileStat) != 0) {
        HILOGW("current file is not exist, create a new file");
        CreateSnoopFileTask(false);
    }
}