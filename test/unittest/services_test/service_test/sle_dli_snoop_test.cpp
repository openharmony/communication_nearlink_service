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

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>

#include "nearlink_access_token_mock.h"
#include "SleDliSnoop.h"
#include "log.h"
#include "parameters.h"

using namespace OHOS::Nearlink;
using namespace testing;
using namespace testing::ext;

namespace {
    constexpr int INVALID_FD = -1;
    constexpr size_t SNOOP_HEADER_LEN = 9; // SnoopHeader长度：timestamp(8)+isReceived(1)
    constexpr uint8_t DLI_SNOOPTYPE_CMD = 0xA1; // 发包CMD类型
    constexpr uint8_t DLI_SNOOPTYPE_EVENT = 0xA2; // 收包EVENT类型
    constexpr uint8_t DLI_SNOOPTYPE_ACB = 0xA3; // 异步连接业务数据
    constexpr uint8_t DLI_SNOOPTYPE_ICB = 0xA4; // 同步连接业务数据
    constexpr uint16_t DLI_STATUS_EVENT = 0x0001; // status事件
    constexpr uint16_t DLI_COMPLETE_EVENT = 0x0002; // complete事件
    constexpr uint16_t SENSITIVE_CMD_OPCODE = 0x1401; // CREATE_CONNECTION（黑名单内，含对端地址）
    constexpr uint16_t NORMAL_CMD_OPCODE = 0x0402; // 黑名单外指令
    constexpr uint16_t UNKNOWN_CMD_OPCODE = 0xFEED; // 模拟未来新增指令
    constexpr uint16_t SENSITIVE_EVENT_OPCODE = 0x0015; // CONNECTION_COMPLETE（黑名单内，含地址）
    const std::string DEVELOPER_MODE_KEY = "const.security.developermode.state"; // 开发者选项开关
    const std::string REMOTE_LOG_KEY = "hiviewdfx.logservice.remotelog.on"; // 远程诊断开关
    const std::string FANS_STATE_KEY = "const.product.dfx.fans.stage"; // 花粉版本标识
    std::unique_ptr<SleDliSnoop> g_dliSnoopPtr = nullptr;

    // 构造发包CMD buffer：snoop头(9)+type+opcode(2)+parLen(2)+参数
    std::vector<uint8_t> ConstructCmdBuffer(uint16_t opcode, const std::vector<uint8_t> &params)
    {
        std::vector<uint8_t> buffer(SNOOP_HEADER_LEN + 1, 0);
        buffer[SNOOP_HEADER_LEN] = DLI_SNOOPTYPE_CMD;
        buffer.push_back(static_cast<uint8_t>(opcode & 0xFF));
        buffer.push_back(static_cast<uint8_t>((opcode >> 8) & 0xFF));
        uint16_t parLen = static_cast<uint16_t>(params.size());
        buffer.push_back(static_cast<uint8_t>(parLen & 0xFF));
        buffer.push_back(static_cast<uint8_t>((parLen >> 8) & 0xFF));
        buffer.insert(buffer.end(), params.begin(), params.end());
        return buffer;
    }
}

namespace OHOS {
namespace Nearlink {
namespace TEST {
class SleDliSnoopTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleDliSnoopTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleDliSnoopTest.");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void SleDliSnoopTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleDliSnoopTest");
}

void SleDliSnoopTest::SetUp()
{
    g_dliSnoopPtr = std::make_unique<SleDliSnoop>();
    HILOGI("SetUp SleDliSnoopTest.");
}

void SleDliSnoopTest::TearDown()
{
    g_dliSnoopPtr->SnoopShutDown();
    sleep(1); // 等待snoop线程完成shutdown任务后再析构实例
    g_dliSnoopPtr = nullptr;
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "false"); // 恢复远程诊断开关
    HILOGI("TearDown SleDliSnoopTest.");
}

/**
 * @tc.name: Dli_Snoop_test_001
 * @tc.desc: SnoopStartUp test
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_001, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_001 enter");
    g_dliSnoopPtr->SnoopStartUp();
    g_dliSnoopPtr->CreateSnoopFile(true);
    sleep(1); // sleep 1s

    EXPECT_EQ(true, g_dliSnoopPtr->isLogging_);
    EXPECT_EQ(true, g_dliSnoopPtr->isModuleStarted_);
    EXPECT_EQ(false, g_dliSnoopPtr->files_.empty());
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    HILOGI("Dli_Snoop_test_001 end");
}

/**
 * @tc.name: Dli_Snoop_test_002
 * @tc.desc: SnoopShutDown test
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_002, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_002 enter");
    g_dliSnoopPtr->SnoopShutDown();

    EXPECT_EQ(false, g_dliSnoopPtr->isLogging_);
    EXPECT_EQ(false,  g_dliSnoopPtr->isModuleStarted_);
    EXPECT_EQ(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    HILOGI("Dli_Snoop_test_002 end");
}

/**
 * @tc.name: Dli_Snoop_test_003
 * @tc.desc: commercial version, CMD in blacklist anonymized to opcode only
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_003, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_003 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    std::vector<uint8_t> params = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; // 对端地址
    std::vector<uint8_t> buffer = ConstructCmdBuffer(SENSITIVE_CMD_OPCODE, params);
    g_dliSnoopPtr->AnonymizeSnoopData(buffer);
    EXPECT_EQ(SNOOP_HEADER_LEN + 1 + sizeof(uint16_t), buffer.size()); // 仅保留头+type+opcode
    HILOGI("Dli_Snoop_test_003 end");
}

/**
 * @tc.name: Dli_Snoop_test_004
 * @tc.desc: commercial version, CMD not in blacklist kept complete
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_004, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_004 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    std::vector<uint8_t> params = {0x01, 0x02, 0x03};
    std::vector<uint8_t> buffer = ConstructCmdBuffer(NORMAL_CMD_OPCODE, params);
    size_t originalSize = buffer.size();
    g_dliSnoopPtr->AnonymizeSnoopData(buffer);
    EXPECT_EQ(originalSize, buffer.size()); // 未命中黑名单完整保留
    HILOGI("Dli_Snoop_test_004 end");
}

/**
 * @tc.name: Dli_Snoop_test_005
 * @tc.desc: commercial version, unknown new CMD kept complete (default policy)
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_005, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_005 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    std::vector<uint8_t> params = {0xAA, 0xBB};
    std::vector<uint8_t> buffer = ConstructCmdBuffer(UNKNOWN_CMD_OPCODE, params);
    size_t originalSize = buffer.size();
    g_dliSnoopPtr->AnonymizeSnoopData(buffer);
    EXPECT_EQ(originalSize, buffer.size()); // 新增指令默认完整落盘，由评审保证无敏感信息
    HILOGI("Dli_Snoop_test_005 end");
}

/**
 * @tc.name: Dli_Snoop_test_006
 * @tc.desc: commercial version, status/complete event judged by carried cmdOpcode
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_006, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_006 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    // status事件携带黑名单内cmdOpcode(0x1401) → 截断到16字节（含被响应指令opcode）
    std::vector<uint8_t> bufferSensitive(SNOOP_HEADER_LEN + 1 + 2 + 2 + 2 + 2, 0);
    bufferSensitive[SNOOP_HEADER_LEN] = DLI_SNOOPTYPE_EVENT;
    bufferSensitive[SNOOP_HEADER_LEN + 1] = static_cast<uint8_t>(DLI_STATUS_EVENT & 0xFF);
    bufferSensitive[SNOOP_HEADER_LEN + 2] = static_cast<uint8_t>((DLI_STATUS_EVENT >> 8) & 0xFF);
    bufferSensitive[SNOOP_HEADER_LEN + 3] = 0x04; // len
    bufferSensitive[SNOOP_HEADER_LEN + 4] = 0x00;
    bufferSensitive[SNOOP_HEADER_LEN + 5] = static_cast<uint8_t>(SENSITIVE_CMD_OPCODE & 0xFF);
    bufferSensitive[SNOOP_HEADER_LEN + 6] = static_cast<uint8_t>((SENSITIVE_CMD_OPCODE >> 8) & 0xFF);
    g_dliSnoopPtr->AnonymizeSnoopData(bufferSensitive);
    EXPECT_EQ(SNOOP_HEADER_LEN + 1 + 2 + 2 + 2, bufferSensitive.size());
    // complete事件携带黑名单外cmdOpcode(0x0402) → 完整保留
    std::vector<uint8_t> bufferNormal(SNOOP_HEADER_LEN + 1 + 2 + 2 + 2 + 2, 0);
    bufferNormal[SNOOP_HEADER_LEN] = DLI_SNOOPTYPE_EVENT;
    bufferNormal[SNOOP_HEADER_LEN + 1] = static_cast<uint8_t>(DLI_COMPLETE_EVENT & 0xFF);
    bufferNormal[SNOOP_HEADER_LEN + 2] = static_cast<uint8_t>((DLI_COMPLETE_EVENT >> 8) & 0xFF);
    bufferNormal[SNOOP_HEADER_LEN + 3] = 0x04; // len
    bufferNormal[SNOOP_HEADER_LEN + 4] = 0x00;
    bufferNormal[SNOOP_HEADER_LEN + 5] = static_cast<uint8_t>(NORMAL_CMD_OPCODE & 0xFF);
    bufferNormal[SNOOP_HEADER_LEN + 6] = static_cast<uint8_t>((NORMAL_CMD_OPCODE >> 8) & 0xFF);
    size_t normalSize = bufferNormal.size();
    g_dliSnoopPtr->AnonymizeSnoopData(bufferNormal);
    EXPECT_EQ(normalSize, bufferNormal.size());
    HILOGI("Dli_Snoop_test_006 end");
}

/**
 * @tc.name: Dli_Snoop_test_007
 * @tc.desc: commercial version, EVENT in blacklist truncated to event+len
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_007, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_007 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    std::vector<uint8_t> buffer(SNOOP_HEADER_LEN + 1 + 2 + 2 + 2, 0);
    buffer[SNOOP_HEADER_LEN] = DLI_SNOOPTYPE_EVENT;
    buffer[SNOOP_HEADER_LEN + 1] = static_cast<uint8_t>(SENSITIVE_EVENT_OPCODE & 0xFF);
    buffer[SNOOP_HEADER_LEN + 2] = static_cast<uint8_t>((SENSITIVE_EVENT_OPCODE >> 8) & 0xFF);
    buffer[SNOOP_HEADER_LEN + 3] = 0x02; // len
    buffer[SNOOP_HEADER_LEN + 4] = 0x00;
    buffer[SNOOP_HEADER_LEN + 5] = 0x11; // 广播标识/地址数据
    buffer[SNOOP_HEADER_LEN + 6] = 0x22;
    g_dliSnoopPtr->AnonymizeSnoopData(buffer);
    EXPECT_EQ(SNOOP_HEADER_LEN + 1 + 2 + 2, buffer.size()); // 仅保留event+len
    HILOGI("Dli_Snoop_test_007 end");
}

/**
 * @tc.name: Dli_Snoop_test_008
 * @tc.desc: commercial version, ACB/ICB business data truncated to header only
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_008, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_008 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    for (uint8_t snoopType : {DLI_SNOOPTYPE_ACB, DLI_SNOOPTYPE_ICB}) {
        std::vector<uint8_t> buffer(SNOOP_HEADER_LEN + 1 + 4 + 8, 0); // 头+type+ACB头+业务数据
        buffer[SNOOP_HEADER_LEN] = snoopType;
        buffer[SNOOP_HEADER_LEN + 1] = 0x01; // lcid/handle低字节
        buffer[SNOOP_HEADER_LEN + 2] = 0x00; // lcid/handle高字节
        buffer[SNOOP_HEADER_LEN + 3] = 0x08; // len
        buffer[SNOOP_HEADER_LEN + 4] = 0x00;
        for (size_t i = 0; i < 8; ++i) {
            buffer[SNOOP_HEADER_LEN + 5 + i] = static_cast<uint8_t>(0xA0 + i); // 业务数据
        }
        g_dliSnoopPtr->AnonymizeSnoopData(buffer);
        EXPECT_EQ(SNOOP_HEADER_LEN + 1 + 4, buffer.size()); // 仅保留包头
    }
    HILOGI("Dli_Snoop_test_008 end");
}

/**
 * @tc.name: Dli_Snoop_test_009
 * @tc.desc: commercial version, EVENT not in blacklist kept complete
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_009, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_009 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    std::vector<uint8_t> buffer(SNOOP_HEADER_LEN + 1 + 2 + 2 + 2, 0);
    buffer[SNOOP_HEADER_LEN] = DLI_SNOOPTYPE_EVENT;
    buffer[SNOOP_HEADER_LEN + 1] = 0x64; // 0x0064 链路质量上报（黑名单外）
    buffer[SNOOP_HEADER_LEN + 2] = 0x00;
    buffer[SNOOP_HEADER_LEN + 3] = 0x02; // len
    buffer[SNOOP_HEADER_LEN + 4] = 0x00;
    buffer[SNOOP_HEADER_LEN + 5] = 0x11;
    buffer[SNOOP_HEADER_LEN + 6] = 0x22;
    size_t originalSize = buffer.size();
    g_dliSnoopPtr->AnonymizeSnoopData(buffer);
    EXPECT_EQ(originalSize, buffer.size());
    HILOGI("Dli_Snoop_test_009 end");
}

/**
 * @tc.name: Dli_Snoop_test_010
 * @tc.desc: commercial version, full capture chain writes anonymized data to file
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_010, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_010 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    g_dliSnoopPtr->isAnonymized_.store(true); // 强制启用匿名化
    g_dliSnoopPtr->CreateSnoopFileTask(true);
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    std::vector<uint8_t> params = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; // 对端地址
    std::vector<uint8_t> buffer = ConstructCmdBuffer(SENSITIVE_CMD_OPCODE, params);
    g_dliSnoopPtr->DliSnoopCaptureTask(buffer, true);

    std::ifstream inFile(g_dliSnoopPtr->snoopLogfilePath_);
    std::string line;
    std::getline(inFile, line);
    inFile.close();
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
    EXPECT_EQ((SNOOP_HEADER_LEN + 1 + sizeof(uint16_t)) * 2, line.length()); // 12字节hex
    EXPECT_EQ(std::string::npos, line.find("010203040506")); // 不含对端地址
    HILOGI("Dli_Snoop_test_010 end");
}

/**
 * @tc.name: Dli_Snoop_test_011
 * @tc.desc: non-commercial version keeps complete data (regression)
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_011, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_011 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(false);
    g_dliSnoopPtr->isAnonymized_.store(true); // 强制启用匿名化
    g_dliSnoopPtr->CreateSnoopFileTask(true);
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    std::vector<uint8_t> params = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}; // 对端地址
    std::vector<uint8_t> buffer = ConstructCmdBuffer(SENSITIVE_CMD_OPCODE, params);
    g_dliSnoopPtr->DliSnoopCaptureTask(buffer, true);

    std::ifstream inFile(g_dliSnoopPtr->snoopLogfilePath_);
    std::string line;
    std::getline(inFile, line);
    inFile.close();
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
    EXPECT_EQ(buffer.size() * 2, line.length()); // 完整落盘
    EXPECT_NE(std::string::npos, line.find("010203040506")); // 含对端地址
    HILOGI("Dli_Snoop_test_011 end");
}

/**
 * @tc.name: Dli_Snoop_test_012
 * @tc.desc: snoop anonymization decision by commercial version and exception switches
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_012, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_012 enter");
    // 非商用版本，禁用匿名化
    g_dliSnoopPtr->isCommercialVersion_.store(false);
    EXPECT_FALSE(g_dliSnoopPtr->IsSnoopAnonymizationEnabled());

    // 商用版本+远程诊断开（例外场景）：不进行匿名化
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "true");
    EXPECT_FALSE(g_dliSnoopPtr->IsSnoopAnonymizationEnabled());

    // 商用版本+远程诊断关：仅取决于开发者选项/花粉版本
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "false");
    bool devOrFans = OHOS::system::GetBoolParameter(DEVELOPER_MODE_KEY, false) ||
        OHOS::system::GetIntParameter(FANS_STATE_KEY, 0) == 1;
    EXPECT_EQ(!devOrFans, g_dliSnoopPtr->IsSnoopAnonymizationEnabled());
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "false");
    HILOGI("Dli_Snoop_test_012 end");
}

/**
 * @tc.name: Dli_Snoop_test_013
 * @tc.desc: remote log parameter watch dynamically updates anonymization mode and cleans files
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_013, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_013 enter");
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    g_dliSnoopPtr->isModuleStarted_ = true;
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "true");
    g_dliSnoopPtr->UpdateLogging();
    EXPECT_TRUE(g_dliSnoopPtr->isLogging_.load()); // 落盘跟随模块使能，与例外场景无关
    EXPECT_FALSE(g_dliSnoopPtr->isAnonymized_.load()); // 远程诊断开，完整落盘

    g_dliSnoopPtr->WatchRemoteLogChange();
    EXPECT_TRUE(g_dliSnoopPtr->isRemoteLogWatched_.load());

    // 写入一条完整敏感数据，制造已落盘文件
    g_dliSnoopPtr->CreateSnoopFileTask(true);
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);
    std::vector<uint8_t> buffer = ConstructCmdBuffer(SENSITIVE_CMD_OPCODE, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    g_dliSnoopPtr->DliSnoopCaptureTask(buffer, true);
    EXPECT_FALSE(g_dliSnoopPtr->files_.empty());

    // 远程诊断关，watch回调在snoop线程重估：切回匿名化并删除已有文件
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "false");
    sleep(1); // 等待watch回调触发重估
    bool devOrFans = OHOS::system::GetBoolParameter(DEVELOPER_MODE_KEY, false) ||
        OHOS::system::GetIntParameter(FANS_STATE_KEY, 0) == 1;
    if (!devOrFans) {
        EXPECT_TRUE(g_dliSnoopPtr->isAnonymized_.load());
        EXPECT_TRUE(g_dliSnoopPtr->files_.empty()); // 文件已删除
    }

    // 重新打开远程诊断开关，动态恢复完整落盘（isLogging_全程保持使能）
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "true");
    sleep(1);
    EXPECT_FALSE(g_dliSnoopPtr->isAnonymized_.load());
    EXPECT_TRUE(g_dliSnoopPtr->isLogging_.load());
    HILOGI("Dli_Snoop_test_013 end");
}

/**
 * @tc.name: Dli_Snoop_test_014
 * @tc.desc: remote log parameter watch dynamically updates anonymization mode and cleans files
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_014, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_014 enter");
    // 上一会话：商用版本+远程诊断开（例外场景）：完整落盘
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    g_dliSnoopPtr->isModuleStarted_ = true;
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "true");
    g_dliSnoopPtr->UpdateLogging();
    EXPECT_FALSE(g_dliSnoopPtr->isAnonymized_.load()); // 远程诊断开，完整落盘

    g_dliSnoopPtr->CreateSnoopFileTask(true);
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);
    std::string previousFilePath = g_dliSnoopPtr->snoopLogfilePath_;
    std::vector<uint8_t> fullBuffer = ConstructCmdBuffer(SENSITIVE_CMD_OPCODE, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
    g_dliSnoopPtr->DliSnoopCaptureTask(fullBuffer, true);
    std::ifstream previousInFile(previousFilePath);
    std::string previousLine;
    std::getline(previousInFile, previousLine);
    previousInFile.close();
    previousLine.erase(std::remove(previousLine.begin(), previousLine.end(), ' '), previousLine.end());
    EXPECT_NE(std::string::npos, previousLine.find("010203040506")); // 上一会话完整敏感数据已落盘

    // 模拟重启：旧实例关闭后重建新实例（初态：isAnonymized_为true, files为空）
    g_dliSnoopPtr->SnoopShutDown();
    sleep(1);
    g_dliSnoopPtr = std::make_unique<SleDliSnoop>();

    // 新会话：商用版本+例外开关关：启动评估匿名化时应清空上会话遗留文件
    g_dliSnoopPtr->isCommercialVersion_.store(true);
    g_dliSnoopPtr->isModuleStarted_ = true;
    (void)OHOS::system::SetParameter(REMOTE_LOG_KEY, "false");
    bool devOrFans = OHOS::system::GetBoolParameter(DEVELOPER_MODE_KEY, false) ||
        OHOS::system::GetIntParameter(FANS_STATE_KEY, 0) == 1;
    g_dliSnoopPtr->UpdateLogging();
    if (!devOrFans) {
        EXPECT_TRUE(g_dliSnoopPtr->isAnonymized_.load());
        std::ifstream removeInFile(previousFilePath);
        EXPECT_FALSE(removeInFile.is_open()); // 含完整数据的遗留文件已删除
        EXPECT_TRUE(g_dliSnoopPtr->files_.empty());
    }
    HILOGI("Dli_Snoop_test_014 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS