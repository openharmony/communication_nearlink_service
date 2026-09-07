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

#include "SleDliSnoop.h"

#include <string>
#include <set>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <charconv>
#include <iostream>
#include <chrono>
#include <sstream>
#include <dirent.h>
#include "parameters.h"
#include "parameter.h"
#include "SleDliLayerAdapter.h"
#include "SleDliThreadUtil.h"
#include "log.h"

namespace fs = std::filesystem;

namespace {
    constexpr uint32_t PROPERTY_VALUE_MAX = 128;
    const std::string VERSION_TYPE_KEY = "const.logsystem.versiontype";
    const std::string DEVELOPER_MODE_KEY = "const.security.developermode.state"; // 开发者选项开关
    const std::string REMOTE_LOG_KEY = "hiviewdfx.logservice.remotelog.on"; // 远程诊断开关
    const std::string FANS_STATE_KEY = "const.product.dfx.fans.stage"; // 花粉版本标识
    const std::string INVALID_COMMERCIAL_VERSION = "invalid";
    const std::string COMMERCIAL_VERSION = "commercial";
    constexpr uint32_t COMMERCIAL_VERSION_SIZE = 10;
    const std::string SNOOP_BASE_PATH = "/data/log/nearlink/";
    constexpr uint32_t MAX_SNOOP_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    constexpr uint32_t MAX_SNOOP_FILES_TOTAL_SIZE = 90 * 1024 * 1024; // 90MB（总文件大小限制防止超过100MB）
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
    constexpr size_t SNOOP_TYPE_OFFSET = SNOOP_HEADER_LENGTH; // buffer中类型字节偏移（发包即data[0]，收包手动添加）
    constexpr size_t SNOOP_OPCODE_OFFSET = SNOOP_HEADER_LENGTH + 1; // buffer中指令标识偏移（发包CMD opcode / 收包EVENT event）
    constexpr size_t SNOOP_EVENT_LEN_OFFSET = SNOOP_HEADER_LENGTH + 3; // buffer中收包EVENT长度字段偏移
    constexpr size_t SNOOP_EVT_CMD_OPCODE_OFFSET = SNOOP_HEADER_LENGTH + 5; // buffer中status/complete事件携带的被响应指令opcode偏移
    constexpr size_t DLI_ACB_HEADER_LEN = 4; // ACB/ICB包头长度：lcid/handle(2)+len(2)
    constexpr uint16_t DLI_STATUS_EVENT = 0x0001;
    constexpr uint16_t DLI_COMPLETE_EVENT = 0x0002;

    // 商用版本DLI日志匿名化黑名单：
    // 命中黑名单的指令data全部舍弃，仅保留指令标识（opcode/event）后落盘；未列入的指令默认完整落盘。
    // 维护约定：新增DLI指令默认完整落盘，若参数含地址/用户隐私等敏感信息，必须同步加入对应黑名单，
    // 由开发与检视人评审保证。黑名单数值为双源维护，须与services/stack/src/dli/interface/dli_opcode.h
    // 中同名指令定义保持一致：指令新增/变更时必须同步两处，防静默失配（漏同步=脱敏漏项，多同步=误截断）。
    const std::set<uint16_t> kSensitiveCmdOpcodes = {
        0x0405, // DLI_SET_PUBLIC_ADDRESS：设置本端公共地址
        0x0406, // DLI_GET_PUBLIC_ADDRESS：获取本端公共地址
        0x040C, // DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST：白名单添加设备（含地址）
        0x040D, // DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST：白名单移除设备（含地址）
        0x0C02, // DLI_SET_ADVERTISING_PARAMETERS：广播参数（含广播标识）
        0x0C03, // DLI_SET_ADVERTISING_DATA：广播数据（用户内容）
        0x0C04, // DLI_SET_SCAN_RESPONSE_DATA：扫描响应数据（用户内容）
        0x1401, // DLI_CREATE_CONNECTION：创建连接（含对端地址）
        0x1812, // DLI_SET_CONTROLLER_DATA：控制面信令数据
        0x1C01, // DLI_ENCRYPT：加密数据（密钥材料）
        0x1C03, // DLI_ENABLE_ENCRYPTION：启动链路加密
        0x1C05, // DLI_ENCRYPTION_PARAMETER_REQUEST_REPLY：加密参数回复（密钥材料）
        0x1C28, // DLI_ENABLE_IMG_ENCRYPTION：启动组播链路加密
    };

    const std::set<uint16_t> kSensitiveEventOpcodes = {
        0x0015, // DLI_CONNECTION_COMPLETE_EVT：连接完成（含对端地址+可解析随机地址）
        0x001A, // DLI_ADVERTISING_REPORT_EVT：广播上报（含广播标识+广播数据）
    };
    constexpr size_t SPACE_CHAR_OFFSET_ONE = 16; // 时间戳后加入的空格的偏移
    constexpr size_t SPACE_CHAR_OFFSET_TWO = 19; // 方向标志后加入的空格的偏移
    constexpr int INVALID_FD = -1;

    enum class DliSnoopType : uint8_t {
        DLI_SNOOPTYPE_CMD = 0xA1,
        DLI_SNOOPTYPE_EVENT = 0xA2,
        DLI_SNOOPTYPE_ACB = 0xA3,
        DLI_SNOOPTYPE_ICB = 0xA4,
    };

    uint16_t ReadSnoopUint16Le(const std::vector<uint8_t> &buffer, size_t offset)
    {
        return static_cast<uint16_t>(buffer[offset]) |
            (static_cast<uint16_t>(buffer[offset + 1]) << 8);
    }

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
        HILOGW("Commercial Version, dli snoop data will be anonymized");
        WatchRemoteLogChange(); // 监听远程诊断开关变化，运行期撤销例外时切回匿名化并清理已落盘文件
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
    UnWatchRemoteLogChange();
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
    bool shouldAnonymize = IsSnoopAnonymizationEnabled();
    if (shouldLog && shouldAnonymize) {
        UpdateFilesQueue();
        if (!files_.empty()) {
            HILOGI("files_ size(%{public}zu)", files_.size());
            RemoveSnoopFiles(static_cast<uint32_t>(files_.size()));
        }
    }
    isAnonymized_.store(shouldAnonymize);
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
    if (!isLogging_.load()) {
        return; // snoop落盘未使能，静默丢弃
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
    // 商用版本且匿名化模式开启（例外场景关）时落盘前匿名化，仅保留指令标识；非商用版本永不匿名化
    if (isCommercialVersion_.load() && isAnonymized_.load()) {
        AnonymizeSnoopData(buffer);
    }
    SnoopWriteLogHexStr(buffer);
}

void SleDliSnoop::AnonymizeSnoopData(std::vector<uint8_t> &buffer)
{
    HILOGD("enter");
    size_t buffLen = buffer.size();
    NL_CHECK_RETURN(buffLen > SNOOP_HEADER_LENGTH, "invalid buffer length: %{public}zu", buffLen);
    switch (static_cast<DliSnoopType>(buffer[SNOOP_TYPE_OFFSET])) {
        case DliSnoopType::DLI_SNOOPTYPE_CMD:
            AnonymizeCmdData(buffer);
            break;
        case DliSnoopType::DLI_SNOOPTYPE_EVENT:
            AnonymizeEventData(buffer);
            break;
        case DliSnoopType::DLI_SNOOPTYPE_ACB:
        case DliSnoopType::DLI_SNOOPTYPE_ICB:
            AnonymizeAcbData(buffer);
            break;
        default:
            // 未知报文类型结构不可安全裁剪，保持完整落盘（fail-open）；新增报文类型须经安全评审并在此补充处理
            HILOGD("unknown snoop type: 0x%{public}x", static_cast<uint32_t>(buffer[SNOOP_TYPE_OFFSET]));
            break;
    }
}

void SleDliSnoop::AnonymizeCmdData(std::vector<uint8_t> &buffer)
{
    // 发包CMD: data[0]=type, data[1..2]=opcode，命中黑名单则舍弃全部参数，仅保留指令标识
    size_t cmdTail = SNOOP_OPCODE_OFFSET + sizeof(uint16_t);
    if (buffer.size() < cmdTail) {
        return; // 长度不足无法解析opcode
    }
    uint16_t opcode = ReadSnoopUint16Le(buffer, SNOOP_OPCODE_OFFSET);
    if (IsSensitiveCmdOpcode(opcode)) {
        buffer.resize(cmdTail);
    }
}

void SleDliSnoop::AnonymizeEventData(std::vector<uint8_t> &buffer)
{
    // 收包EVENT: data[0..1]=event, data[2..3]=len；status/complete事件data[4..5]为被响应指令opcode
    size_t evtHead = SNOOP_OPCODE_OFFSET + sizeof(uint16_t);
    if (buffer.size() < evtHead) {
        return; // 长度不足无法解析event
    }
    uint16_t event = ReadSnoopUint16Le(buffer, SNOOP_OPCODE_OFFSET);
    if (event == DLI_STATUS_EVENT || event == DLI_COMPLETE_EVENT) {
        AnonymizeEventCarriedCmdData(buffer);
        return;
    }
    // 其余事件命中黑名单则保留event+len，舍弃后续数据
    if (IsSensitiveEventOpcode(event)) {
        buffer.resize(SNOOP_EVENT_LEN_OFFSET + sizeof(uint16_t));
    }
}

void SleDliSnoop::AnonymizeEventCarriedCmdData(std::vector<uint8_t> &buffer)
{
    // status/complete事件按携带的被响应指令opcode判定：命中黑名单则截断到cmdOpcode尾部
    size_t carriedTail = SNOOP_EVT_CMD_OPCODE_OFFSET + sizeof(uint16_t);
    if (buffer.size() < carriedTail) {
        return; // 未携带完整cmdOpcode
    }
    uint16_t cmdOpcode = ReadSnoopUint16Le(buffer, SNOOP_EVT_CMD_OPCODE_OFFSET);
    if (IsSensitiveCmdOpcode(cmdOpcode)) {
        buffer.resize(carriedTail);
    }
}

void SleDliSnoop::AnonymizeAcbData(std::vector<uint8_t> &buffer)
{
    // 业务数据无指令标识，仅保留包头（lcid/handle+len），data全部舍弃
    size_t acbTail = SNOOP_TYPE_OFFSET + sizeof(uint8_t) + DLI_ACB_HEADER_LEN;
    if (buffer.size() < acbTail) {
        return; // 长度不足
    }
    buffer.resize(acbTail);
}

bool SleDliSnoop::IsSensitiveCmdOpcode(uint16_t opcode)
{
    return kSensitiveCmdOpcodes.count(opcode) > 0 || IsExtSensitiveOpcode(opcode, true);
}

bool SleDliSnoop::IsSensitiveEventOpcode(uint16_t event)
{
    return kSensitiveEventOpcodes.count(event) > 0 || IsExtSensitiveOpcode(event, false);
}

bool SleDliSnoop::IsExtSensitiveOpcode(uint16_t opcode, bool isCmd)
{
    // 扩展集合的读写均在sle_dli串行队列内执行，此处无锁访问
    const std::set<uint16_t> &extSensitiveOpcodes = isCmd ? extSensitiveCmdOpcodes_ : extSensitiveEvtOpcodes_;
    return extSensitiveOpcodes.count(opcode) > 0;
}

void SleDliSnoop::RegisterSensitiveOpcodes(const uint16_t *cmdOpcodes, uint32_t cmdNum,
    const uint16_t *evtOpcodes, uint32_t evtNum)
{
    if (cmdNum > 0 && cmdOpcodes == nullptr) {
        HILOGE("invalid cmdOpcodes");
        return;
    }
    if (evtNum > 0 && evtOpcodes == nullptr) {
        HILOGE("invalid evtOpcodes");
        return;
    }
    // 先拷贝入vector再投递任务，避免调用方原始指针在任务执行前失效；注册在业务流量产生前完成即可
    std::vector<uint16_t> cmdVec(cmdOpcodes, cmdOpcodes + cmdNum);
    std::vector<uint16_t> evtVec(evtOpcodes, evtOpcodes + evtNum);
    DoInSnoopThread([this, cmdVec = std::move(cmdVec), evtVec = std::move(evtVec)]() -> void {
        RegisterSensitiveOpcodesTask(cmdVec, evtVec);
    });
}

void SleDliSnoop::RegisterSensitiveOpcodesTask(const std::vector<uint16_t> &cmdOpcodes,
    const std::vector<uint16_t> &evtOpcodes)
{
    // 全量替换语义，日志输出被替换条数使重复注册/多注册方互踩可见
    HILOGI("replace ext sensitive opcodes, old cmd:%{public}zu evt:%{public}zu, "
        "new cmd:%{public}zu evt:%{public}zu",
        extSensitiveCmdOpcodes_.size(), extSensitiveEvtOpcodes_.size(),
        cmdOpcodes.size(), evtOpcodes.size());
    extSensitiveCmdOpcodes_.clear();
    extSensitiveEvtOpcodes_.clear();
    extSensitiveCmdOpcodes_.insert(cmdOpcodes.begin(), cmdOpcodes.end());
    extSensitiveEvtOpcodes_.insert(evtOpcodes.begin(), evtOpcodes.end());
}

extern "C" void SleDliSnoopRegisterSensitiveOpcodes(const uint16_t *cmdOpcodes, uint32_t cmdNum,
    const uint16_t *evtOpcodes, uint32_t evtNum)
{
    SleDliSnoop::GetInstance().RegisterSensitiveOpcodes(cmdOpcodes, cmdNum, evtOpcodes, evtNum);
}

bool SleDliSnoop::IsSnoopAnonymizationEnabled()
{
    if (!isCommercialVersion_.load()) {
        return false; // 非商用版本不进行匿名化，完整落盘
    }
    // 商用版本：开发者选项开关、远程诊断开关、花粉版本任一开启属于例外场景，不匿名化（完整落盘）
    bool isDeveloperModeOn = OHOS::system::GetBoolParameter(DEVELOPER_MODE_KEY, false);
    bool isRemoteLogOn = OHOS::system::GetBoolParameter(REMOTE_LOG_KEY, false);
    bool isFansStateOn = OHOS::system::GetIntParameter(FANS_STATE_KEY, 0) == 1;
    HILOGI("isDeveloperModeOn: %{public}d, isRemoteLogOn: %{public}d, isFansStateOn: %{public}d",
        isDeveloperModeOn, isRemoteLogOn, isFansStateOn);
    return !(isDeveloperModeOn || isRemoteLogOn || isFansStateOn);
}

void SleDliSnoop::WatchRemoteLogChange()
{
    // 仅商用版本监听远程诊断开关变化；重复注册防护
    if (!isCommercialVersion_.load() || isRemoteLogWatched_.load()) {
        return;
    }
    int ret = WatchParameter(REMOTE_LOG_KEY.c_str(), OnRemoteLogChange, this);
    if (ret != 0) {
        // 注册失败后例外撤销（远程诊断关）不可感知，完整落盘将延续到下次SnoopStartUp重试成功
        // 启动评估本身仍保守（isAnonymized_初值true），风险仅在运行期"先开后关"路径
        HILOGE("WatchParameter failed, ret: %{public}d", ret);
        return;
    }
    isRemoteLogWatched_.store(true);
    HILOGI("watch remote log change");
}

void SleDliSnoop::UnWatchRemoteLogChange()
{
    if (!isRemoteLogWatched_.load()) {
        return;
    }
    int ret = RemoveParameterWatcher(REMOTE_LOG_KEY.c_str(), OnRemoteLogChange, this);
    if (ret != 0) {
        HILOGE("UnWatchParameter failed, ret: %{public}d", ret);
    }
    isRemoteLogWatched_.store(false);
    HILOGI("unwatch remote log change");
}

void SleDliSnoop::OnRemoteLogChange(const char *key, const char *value, void *context)
{
    (void)key;
    (void)value;
    SleDliSnoop *instance = static_cast<SleDliSnoop *>(context);
    if (instance == nullptr) {
        HILOGE("instance is nullptr");
        return;
    }
    instance->DoInSnoopThread([instance]() -> void {
        instance->UpdateLogging(); // 远程诊断开关变化后在snoop线程重估匿名化模式
    });
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