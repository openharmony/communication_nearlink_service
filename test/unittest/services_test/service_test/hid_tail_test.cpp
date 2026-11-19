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

/*
 * HID 域收尾测试：uhid 发送路径的等待与任务类型分支、超长报告守卫、
 * 管道注入的事件分发状态机（启停开关与输出/特征/设置报告三分发）、
 * 服务启停幂等、断连失败回报链与写属性回调的类型门与错误码映射。
 */

#include <gtest/gtest.h>
#include <chrono>
#include <cstring>
#include <thread>

#include "HidHostService.h"
#include "hid_client_api.h"
#include "HidHostStackAdapter.h"
#include "HidHostUhid.h"
#include "HidHostDefines.h"
#include "SleInterfaceManager.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_def_types.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

namespace {
constexpr int DELAY_1000_MS = 1000;
constexpr int DELAY_200_MS = 200;
constexpr const char *HID_ADDR = "11:22:33:44:55:66";
} // namespace

namespace OHOS {
namespace Sle {

class HidTailTest : public testing::Test {
public:
    HidTailTest() {}
    ~HidTailTest() {}

    static void SetUpTestCase(void)
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        OHOS::Nearlink::SleInterfaceManager::GetInstance()->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_1000_MS));
    }

    static void TearDownTestCase(void) {}

    void SetUp(void) {}

    void TearDown(void) {}
};

/* uhid 发送路径：未打开设备直接失败；重试等待超限后失败；就绪后按任务类型
 *   分发获取/设置回包与未知类型，任务字段分发后复位，超长报告安全拒绝 */
HWTEST_F(HidTailTest, UhidSendGuardsTest_001, TestSize.Level1)
{
    auto uhid = std::make_shared<HidHostUhid>(HID_ADDR);
    uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};

    // 未打开设备：三条发送路径都直接失败
    EXPECT_EQ(uhid->SendData(data, sizeof(data)), HID_HOST_FAILURE);
    EXPECT_EQ(uhid->SendControlData(data, sizeof(data)), HID_HOST_FAILURE);
    EXPECT_EQ(uhid->SendHandshake(0), HID_HOST_FAILURE);
    EXPECT_EQ(uhid->SendHidInfo(HidInformation()), HID_HOST_FAILURE);

    // 管道替代真实设备：等待次数超限跳过轮询，未就绪仍失败
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    uhid->fd_ = fds[0];
    uhid->waitingTimes = MAX_WAITING_TIME;
    EXPECT_EQ(uhid->SendData(data, sizeof(data)), HID_HOST_FAILURE);

    // 就绪后的普通数据写入与未知任务类型：安全完成并复位任务字段
    uhid->readyForData_ = true;
    uhid->taskType_ = -1;
    EXPECT_EQ(uhid->SendData(data, sizeof(data)), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->SendControlData(data, sizeof(data)), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->taskType_, -1);

    // 获取报告任务：走获取回包路径并复位任务字段
    uhid->taskType_ = HID_HOST_DATA_TYPE_GET_REPORT;
    uhid->taskId_ = 7;
    EXPECT_EQ(uhid->SendControlData(data, sizeof(data)), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->taskType_, -1);

    // 设置报告任务：走设置回包路径
    uhid->taskType_ = HID_HOST_DATA_TYPE_SET_REPORT;
    EXPECT_EQ(uhid->SendControlData(data, sizeof(data)), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->taskType_, -1);

    // 握手路径的任务类型分支
    uhid->taskType_ = HID_HOST_DATA_TYPE_GET_REPORT;
    EXPECT_EQ(uhid->SendHandshake(0), HID_HOST_SUCCESS);
    uhid->taskType_ = HID_HOST_DATA_TYPE_SET_REPORT;
    EXPECT_EQ(uhid->SendHandshake(0), HID_HOST_SUCCESS);
    uhid->taskType_ = -1;
    EXPECT_EQ(uhid->SendHandshake(0), HID_HOST_SUCCESS);

    // 超长获取报告回包：长度守卫拒绝写入，不崩溃且任务字段复位
    std::vector<uint8_t> huge(4097, 0x01);
    uhid->taskType_ = HID_HOST_DATA_TYPE_GET_REPORT;
    uhid->SendControlData(huge.data(), static_cast<uint16_t>(huge.size()));
    EXPECT_EQ(uhid->taskType_, -1);

    close(fds[0]);
    close(fds[1]);
}

/* uhid 事件状态机：管道注入内核事件，启停开关切换就绪标记，
 *   未知事件与输出事件族安全分发 */
HWTEST_F(HidTailTest, UhidEventStatesTest_001, TestSize.Level1)
{
    auto uhid = std::make_shared<HidHostUhid>(HID_ADDR);
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    uhid->fd_ = fds[0];

    auto feed = [&fds](const struct uhid_event &ev) {
        ASSERT_EQ(write(fds[1], &ev, sizeof(ev)), static_cast<ssize_t>(sizeof(ev)));
    };
    struct uhid_event ev = {};
    ev.type = UHID_START;
    feed(ev);
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_TRUE(uhid->readyForData_.load());

    ev = {};
    ev.type = UHID_STOP;
    feed(ev);
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_FALSE(uhid->readyForData_.load());

    ev = {};
    ev.type = UHID_OPEN;
    feed(ev);
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_TRUE(uhid->readyForData_.load());

    ev = {};
    ev.type = UHID_CLOSE;
    feed(ev);
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_FALSE(uhid->readyForData_.load());

    // 未知事件与输出事件族：安全处理不改就绪标记
    ev = {};
    ev.type = UHID_OUTPUT_EV;
    feed(ev);
    EXPECT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);

    ev = {};
    ev.type = UHID_OUTPUT;
    ev.u.output.rtype = UHID_FEATURE_REPORT;
    ev.u.output.size = 1;
    feed(ev);
    EXPECT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);

    ev = {};
    ev.type = UHID_OUTPUT;
    ev.u.output.rtype = UHID_OUTPUT_REPORT;
    ev.u.output.size = 1;
    feed(ev);
    EXPECT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);

    ev = {};
    ev.type = UHID_OUTPUT;
    ev.u.output.rtype = HID_HOST_INPUT_REPORT;
    ev.u.output.size = 1;
    feed(ev);
    EXPECT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);

    close(fds[0]);
    close(fds[1]);
}

/* 特征与设置报告事件：报告类型三分发后置任务字段，供控制回包按任务续接 */
HWTEST_F(HidTailTest, UhidFeatureAndSetReportTest_001, TestSize.Level1)
{
    auto uhid = std::make_shared<HidHostUhid>(HID_ADDR);
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    uhid->fd_ = fds[0];

    struct uhid_event ev = {};
    ev.type = UHID_FEATURE;
    ev.u.feature.id = 9;
    ev.u.feature.rtype = UHID_FEATURE_REPORT;
    ASSERT_EQ(write(fds[1], &ev, sizeof(ev)), static_cast<ssize_t>(sizeof(ev)));
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->taskType_, static_cast<int>(HID_HOST_DATA_TYPE_GET_REPORT));
    EXPECT_EQ(uhid->taskId_, 9);

    ev = {};
    ev.type = UHID_SET_REPORT;
    ev.u.set_report.id = 5;
    ev.u.set_report.rtype = UHID_OUTPUT_REPORT;
    ev.u.set_report.size = 1;
    ASSERT_EQ(write(fds[1], &ev, sizeof(ev)), static_cast<ssize_t>(sizeof(ev)));
    ASSERT_EQ(uhid->ReadUhidEvent(), HID_HOST_SUCCESS);
    EXPECT_EQ(uhid->taskType_, static_cast<int>(HID_HOST_DATA_TYPE_SET_REPORT));

    close(fds[0]);
    close(fds[1]);
}

/* HID 服务启停幂等与断连失败回报：重复启停安全，断连且带失败码时走配对失败上报链 */
HWTEST_F(HidTailTest, ServiceStatesAndDisconnReportTest_001, TestSize.Level1)
{
    HidHostService *service = HidHostService::GetService();
    ASSERT_NE(service, nullptr);

    // 重复启用与重复停用：幂等安全
    service->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    service->Enable();
    service->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
    service->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));

    // 断连且前置连接中、返回码非零：触发配对失败上报分支
    service->NotifyStateChanged(RawAddress(HID_ADDR),
        static_cast<int>(OHOS::Nearlink::SleConnectState::DISCONNECTED),
        static_cast<int>(OHOS::Nearlink::SleConnectState::CONNECTING), 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_200_MS));
}

/* 写属性回调：描述符类型直接早退；报告类型按返回码映射握手结果并下发服务 */
HWTEST_F(HidTailTest, WritePropertyCbkArmsTest_001, TestSize.Level1)
{
    SLE_Addr_S stackAddr = {};
    stackAddr.type = 0;
    (void)memset_s(stackAddr.addr, SLE_ADDR_LEN, 0x11, 1);

    // 描述符类型：不进入握手链
    HidHostStackAdapter::HidWritePropertyCbk(&stackAddr, HID_TYPE_AND_FORMAT_DESC, NLSTK_ERRCODE_SUCCESS);
    // 报告类型 + 失败返回码：错误码映射为握手错误下发
    HidHostStackAdapter::HidWritePropertyCbk(&stackAddr, HID_INPUT_REPORT_INFO, NLSTK_ERRCODE_FAIL);
    // 报告类型 + 成功返回码：正常握手下发
    HidHostStackAdapter::HidWritePropertyCbk(&stackAddr, HID_OUTPUT_REPORT_INFO, NLSTK_ERRCODE_SUCCESS);
    SUCCEED();
}

} // namespace Sle
} // namespace OHOS
