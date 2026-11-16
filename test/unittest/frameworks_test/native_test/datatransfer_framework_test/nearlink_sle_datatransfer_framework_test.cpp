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

// 目标: frameworks/native/nearlink_sle_datatransfer.cpp 零覆盖分支(进程内驱动, 不依赖真实数传栈)
// 手法: 编入产品源文件(private=public) 直达 pimpl; 用产品 Stub 派生的 Mock 服务注入
//       NearlinkBaseSaManager::profileRemoteMap_; 直驱回调桩 OnConnectionStateChanged
//       状态矩阵(CONNECTED/DISCONNECTED/其他 x fd 有效/无效), 直调 ReceivedData 与
//       socketpair+worker 线端到端; 回调里的 transfer 判空分支结构不可达(弱引用过期
//       要求 impl 先亡, 而回调存储于 impl 自身的 PortSocketManager 中), 不设用例。

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "nearlink_access_token_mock.h"
#include "nearlink_test_enable_wait.h"

#include "nearlink_base_sa_manager.h"
#include "nearlink_sle_datatransfer_stub.h"
#include "nearlink_sle_datatransfer.cpp"
#include "nearlink_socket_manager.h"
#include "nearlink_datatransfer_parcel.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string TEST_DEVICE_ADDR = "11:22:33:44:55:66";
const std::string TEST_UUID = "060D";
const std::string TEST_UUID_CUSTOM = "8f1d3a2e-5b4c-4d6e-9f0a-1b2c3d4e5f60";
constexpr uint16_t TEST_PORT = 9;
constexpr uint16_t TEST_MTU = 256;
constexpr int POLL_INTERVAL_MS = 50;

// Mock 服务: 派生产品 Stub, 覆写全部接口方法为受控返回+调用记录; 注入后 GetProxy 直连本对象
class MockSleDataTransferService : public NearlinkSleDataTransferStub {
public:
    NlErrCode socketEmptyMsgRet = NL_NO_ERROR;
    NlErrCode createPortRet = NL_NO_ERROR;
    uint16_t createPortOut = TEST_PORT;
    int socketEmptyMsgCount = 0;
    uint16_t lastSocketEmptyPort = 0;
    std::string lastSocketEmptyAddress;

    NlErrCode RegisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) override
    {
        (void)callback;
        return NL_NO_ERROR;
    }

    NlErrCode DeregisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) override
    {
        (void)callback;
        return NL_NO_ERROR;
    }

    NlErrCode CreatePort(const std::string &uuid, uint16_t &port) override
    {
        (void)uuid;
        port = createPortOut;
        return createPortRet;
    }

    NlErrCode SocketEmptyMsg(uint16_t port, std::string address) override
    {
        socketEmptyMsgCount++;
        lastSocketEmptyPort = port;
        lastSocketEmptyAddress = address;
        return socketEmptyMsgRet;
    }

    NlErrCode DestroyPort(const std::string &uuid, uint16_t port) override
    {
        (void)uuid;
        (void)port;
        return NL_NO_ERROR;
    }

    int connectCount = 0;
    uint16_t lastConnectPort = 0;
    int disconnectCount = 0;
    uint16_t lastDisconnectPort = 0;
    int getStateCount = 0;
    uint16_t lastGetStatePort = 0;

    NlErrCode Connect(NearlinkSleDataTransferConnectionParams &params) override
    {
        connectCount++;
        lastConnectPort = params.GetPort();
        return NL_NO_ERROR;
    }

    NlErrCode Disconnect(NearlinkSleDataTransferConnectionParams &params) override
    {
        disconnectCount++;
        lastDisconnectPort = params.GetPort();
        return NL_NO_ERROR;
    }

    NlErrCode GetConnectionState(NearlinkSleDataTransferConnectionParams &params, int32_t &connState) override
    {
        getStateCount++;
        lastGetStatePort = params.GetPort();
        connState = static_cast<int32_t>(SleConnectState::CONNECTED);
        return NL_NO_ERROR;
    }
};

sptr<MockSleDataTransferService> g_mockService = nullptr;

void InjectMockDataTransferService()
{
    if (g_mockService == nullptr) {
        g_mockService = new MockSleDataTransferService();
    }
    NearlinkBaseSaManager::GetInstance().profileRemoteMap_.EnsureInsert(SLE_DATATRANSFER_SERVER, g_mockService);
}

// 建立 socketpair: [0] 交给框架(打 SOCKET tag + 非阻塞, 对齐 CreateSocketPair 产物), [1] 留作对端
void MakeTaggedSocketPair(int sv[2])
{
    ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
    int flags = fcntl(sv[0], F_GETFL, 0);
    fcntl(sv[0], F_SETFL, flags | O_NONBLOCK);
    fdsan_exchange_owner_tag(sv[0], 0, NEARLINK_FDSAN_TAG_SOCKET);
}

void ClosePeerFd(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}
} // namespace

// 记录型 Fake: 实现框架侧数传回调
class FakeSleDataTransferCallback : public SleDataTransferCallback {
public:
    void OnConnectionStateChanged(const ConnectionParams &result) override
    {
        connCount_++;
        lastState_ = result.GetState();
        lastMtu_ = result.GetMtu();
        lastAddress_ = result.GetAddress();
    }

    void OnReceiveData(const DataParams &result) override
    {
        receiveCount_++;
        size_t size = 0;
        const uint8_t *data = result.GetData(&size).get();
        lastDataSize_ = size;
        lastAddress_ = result.GetAddress();
        lastUuid_ = result.GetUuid();
        lastData_.assign(data, data + size);
    }

    int connCount_ = 0;
    int receiveCount_ = 0;
    int32_t lastState_ = -1;
    uint16_t lastMtu_ = 0;
    size_t lastDataSize_ = 0;
    std::string lastAddress_;
    std::string lastUuid_;
    std::vector<uint8_t> lastData_;
};

class SleDataTransferFrameworkTest : public testing::Test {
public:
    std::shared_ptr<SleDataTransfer> dt_;
    std::shared_ptr<FakeSleDataTransferCallback> fakeCallback_;

    static void SetUpTestCase()
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        if (!NearlinkHost::GetInstance().IsSleEnabled()) {
            NearlinkHost::GetInstance().EnableNl();
        }
        ASSERT_TRUE(WaitForSleEnabled()) << "SLE enable not finished in 20s";
        InjectMockDataTransferService();
    }

    static void TearDownTestCase()
    {
        NearlinkBaseSaManager::GetInstance().profileRemoteMap_.Erase(SLE_DATATRANSFER_SERVER);
        g_mockService = nullptr;
    }

    void SetUp() override
    {
        dt_ = SleDataTransfer::CreateSleDataTransfer();
        ASSERT_NE(nullptr, dt_);
        fakeCallback_ = std::make_shared<FakeSleDataTransferCallback>();
        dt_->pimpl->callbacks_.EnsureInsert(TEST_PORT, fakeCallback_);
    }

    void TearDown() override
    {
        if (dt_ != nullptr) {
            dt_->pimpl->portSocketManager_->DestroyPort(TEST_PORT);
            dt_->pimpl->callbacks_.Erase(TEST_PORT);
        }
        dt_ = nullptr;
        fakeCallback_ = nullptr;
    }

    NearlinkSleDataTransferConnectionParams MakeConnParams(uint16_t port, int32_t state)
    {
        NearlinkSleDataTransferConnectionParams params;
        params.SetAddress(TEST_DEVICE_ADDR);
        params.SetUuid(TEST_UUID);
        params.SetPort(port);
        params.SetState(state);
        params.SetMtu(TEST_MTU);
        return params;
    }

    // 序列化一帧数传数据(与产品 WriteData 下行格式一致)
    std::vector<uint8_t> MakeSerializedPayload(uint16_t port, size_t dataLen)
    {
        DataTransferDataParams src(TEST_DEVICE_ADDR, TEST_UUID, port, std::vector<uint8_t>(dataLen, 0xAB));
        size_t totalLen = 0;
        std::unique_ptr<uint8_t[]> buf = NearlinkDataTransferDataParams::SerializeData(src, totalLen);
        EXPECT_NE(nullptr, buf.get());
        return std::vector<uint8_t>(buf.get(), buf.get() + totalLen);
    }

    // 轮询等待谓言成立(默认 3s 超时)
    bool WaitFor(const std::function<bool()> &pred, int timeoutMs = 3000)
    {
        for (int waited = 0; !pred(); waited += POLL_INTERVAL_MS) {
            if (waited >= timeoutMs) {
                return false;
            }
            usleep(POLL_INTERVAL_MS * 1000);
        }
        return true;
    }
};

/**
 * @tc.number: SleDataTransferFramework_DataParams_001
 * @tc.name: DataParams 拷贝构造深拷贝矩阵
 * @tc.desc: 非空源深拷贝/空源置空
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_DataParams_001, TestSize.Level1)
{
    const uint8_t src[3] = {0x01, 0x02, 0x03};
    DataParams full(TEST_DEVICE_ADDR, TEST_UUID);
    full.SetData(src, sizeof(src));
    size_t size = 0;
    ASSERT_NE(nullptr, full.GetData(&size).get());
    ASSERT_EQ(3U, size);

    DataParams copied(full);
    size = 0;
    const uint8_t *data = copied.GetData(&size).get();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(3U, size);
    EXPECT_EQ(0, memcmp(src, data, sizeof(src)));
    EXPECT_NE(full.GetData(&size).get(), data); // 深拷贝: 缓冲独立
    EXPECT_EQ(TEST_DEVICE_ADDR, copied.GetAddress());
    EXPECT_EQ(TEST_UUID, copied.GetUuid());

    DataParams empty(TEST_DEVICE_ADDR, TEST_UUID);
    DataParams copiedEmpty(empty);
    size = 1; // 哨兵: 空源应写回 0
    EXPECT_EQ(nullptr, copiedEmpty.GetData(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.number: SleDataTransferFramework_DataParams_002
 * @tc.name: DataParams 赋值矩阵
 * @tc.desc: 自赋值不变/赋非空深拷贝/赋空重置
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_DataParams_002, TestSize.Level1)
{
    const uint8_t src[3] = {0x0A, 0x0B, 0x0C};
    DataParams p(TEST_DEVICE_ADDR, TEST_UUID);
    p.SetData(src, sizeof(src));

    DataParams &self = p;
    p = self; // 自赋值
    size_t size = 0;
    ASSERT_NE(nullptr, p.GetData(&size).get());
    ASSERT_EQ(3U, size);
    EXPECT_EQ(0, memcmp(src, p.GetData(&size).get(), sizeof(src)));

    DataParams q(TEST_DEVICE_ADDR + "2", TEST_UUID + "2");
    q = p; // 赋非空
    size = 0;
    const uint8_t *data = q.GetData(&size).get();
    ASSERT_NE(nullptr, data);
    ASSERT_EQ(3U, size);
    EXPECT_EQ(0, memcmp(src, data, sizeof(src)));
    EXPECT_EQ(TEST_DEVICE_ADDR, q.GetAddress());

    DataParams empty(TEST_DEVICE_ADDR, TEST_UUID);
    q = empty; // 赋空
    size = 1;
    EXPECT_EQ(nullptr, q.GetData(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.number: SleDataTransferFramework_DataParams_003
 * @tc.name: SetData 非法入参拒绝
 * @tc.desc: nullptr/零长度不写入
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_DataParams_003, TestSize.Level1)
{
    DataParams p(TEST_DEVICE_ADDR, TEST_UUID);
    p.SetData(nullptr, 4);
    const uint8_t buf[2] = {0x01, 0x02};
    p.SetData(buf, 0);
    size_t size = 1;
    EXPECT_EQ(nullptr, p.GetData(&size).get());
    EXPECT_EQ(0U, size);
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_001
 * @tc.name: 未注册端口连接事件被丢弃
 * @tc.desc: 端口不在 callbacks_: 不触达框架回调, fd=-1 不关闭
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_001, TestSize.Level1)
{
    NearlinkSleDataTransferConnectionParams params = MakeConnParams(TEST_PORT + 5,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(params, -1);
    EXPECT_EQ(0, fakeCallback_->connCount_);
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_002
 * @tc.name: 未注册端口携带有效 fd 时关闭 fd
 * @tc.desc: 端口未注册且 fd 有效 -> fdsan 关闭该 fd
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_002, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    NearlinkSleDataTransferConnectionParams params = MakeConnParams(TEST_PORT + 6,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(params, sv[0]);
    EXPECT_EQ(0, fakeCallback_->connCount_);
    EXPECT_EQ(-1, fcntl(sv[0], F_GETFD)); // 框架已关闭
    ClosePeerFd(sv[1]);
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_003
 * @tc.name: CONNECTED 无 fd: 仅转发状态
 * @tc.desc: 已连接但 fd=-1: 不建 socket, 回调收到状态与 mtu
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_003, TestSize.Level1)
{
    NearlinkSleDataTransferConnectionParams params = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(params, -1);

    ASSERT_EQ(1, fakeCallback_->connCount_);
    EXPECT_EQ(static_cast<int32_t>(SleConnectState::CONNECTED), fakeCallback_->lastState_);
    EXPECT_EQ(TEST_MTU, fakeCallback_->lastMtu_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fakeCallback_->lastAddress_);
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_004
 * @tc.name: 非 CONNECTED/DISCONNECTED 状态: 仅转发
 * @tc.desc: CONNECTING 状态: 不建 socket 不销毁端口, 回调收到状态
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_004, TestSize.Level1)
{
    NearlinkSleDataTransferConnectionParams params = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTING));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(params, -1);

    ASSERT_EQ(1, fakeCallback_->connCount_);
    EXPECT_EQ(static_cast<int32_t>(SleConnectState::CONNECTING), fakeCallback_->lastState_);
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_005
 * @tc.name: CONNECTED 有效 fd: 建 socket + 监听 + 工作线程
 * @tc.desc: 已连接且 fd 有效: SetSocketDataTransfer 落地, Listen/RunThread
 *           启动, SendData 可写通
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_005, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    NearlinkSleDataTransferConnectionParams params = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(params, sv[0]);

    ASSERT_EQ(1, fakeCallback_->connCount_);
    EXPECT_EQ(static_cast<int32_t>(SleConnectState::CONNECTED), fakeCallback_->lastState_);

    const uint8_t byte = 0x5A;
    EXPECT_EQ(SocketTransState::SLE_TRANS_RESULT_SUCCESS,
        dt_->pimpl->portSocketManager_->SendData(TEST_PORT, TEST_DEVICE_ADDR, &byte, 1, 1));

    ClosePeerFd(sv[1]);
    // TearDown 中 DestroyPort 关闭 sv[0] 并回收工作线程
}

/**
 * @tc.number: SleDataTransferFramework_OnConnStateChanged_006
 * @tc.name: DISCONNECTED: 销毁对端端口
 * @tc.desc: 断连状态走 DestroyPeerPort: 先建后断, socket 被回收
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_OnConnStateChanged_006, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    NearlinkSleDataTransferConnectionParams connected = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(connected, sv[0]);
    ASSERT_EQ(1, fakeCallback_->connCount_);

    NearlinkSleDataTransferConnectionParams disconnected = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::DISCONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(disconnected, -1);

    ASSERT_EQ(2, fakeCallback_->connCount_);
    EXPECT_EQ(static_cast<int32_t>(SleConnectState::DISCONNECTED), fakeCallback_->lastState_);
    EXPECT_EQ(-1, fcntl(sv[0], F_GETFD)); // 端口销毁时框架关闭
    ClosePeerFd(sv[1]);
}

/**
 * @tc.number: SleDataTransferFramework_ReceivedData_001
 * @tc.name: 直调 ReceivedData: 已注册端口小包分发
 * @tc.desc: 反序列化成功 -> 回调 OnReceiveData, 小于阈值不触发 SocketEmptyMsg
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_ReceivedData_001, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    std::vector<uint8_t> payload = MakeSerializedPayload(TEST_PORT, 4);
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));

    g_mockService->socketEmptyMsgCount = 0;
    auto input = std::make_shared<InputStream>(sv[0]);
    bool ret = dt_->pimpl->ReceivedData(input, TEST_PORT, TEST_DEVICE_ADDR);
    EXPECT_TRUE(ret);

    ASSERT_TRUE(WaitFor([this]() { return fakeCallback_->receiveCount_ >= 1; }));
    EXPECT_EQ(4U, fakeCallback_->lastDataSize_);
    EXPECT_EQ(std::vector<uint8_t>(4, 0xAB), fakeCallback_->lastData_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fakeCallback_->lastAddress_);
    EXPECT_EQ(0, g_mockService->socketEmptyMsgCount); // 未达阈值

    ClosePeerFd(sv[1]);
    fdsan_close_with_tag(sv[0], NEARLINK_FDSAN_TAG_SOCKET); // 本用例自行管理 fd, 未交给 PortSocketManager
}

/**
 * @tc.number: SleDataTransferFramework_ReceivedData_002
 * @tc.name: 直调 ReceivedData: 大包超过上报阈值
 * @tc.desc: 读回字节数 >= SOCKET_BUFFER_THRESHOLD_REPORT -> 通知服务侧腾空
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_ReceivedData_002, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    const size_t bigLen = SOCKET_BUFFER_THRESHOLD_REPORT + 512; // 序列化后仍 < SOCKET_BUFFER_SIZE
    std::vector<uint8_t> payload = MakeSerializedPayload(TEST_PORT, bigLen);
    ASSERT_LT(payload.size(), static_cast<size_t>(SOCKET_BUFFER_SIZE));
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));

    g_mockService->socketEmptyMsgCount = 0;
    auto input = std::make_shared<InputStream>(sv[0]);
    bool ret = dt_->pimpl->ReceivedData(input, TEST_PORT, TEST_DEVICE_ADDR);
    EXPECT_TRUE(ret);

    ASSERT_TRUE(WaitFor([this]() { return fakeCallback_->receiveCount_ >= 1; }));
    EXPECT_EQ(bigLen, fakeCallback_->lastDataSize_);
    ASSERT_TRUE(WaitFor([&mock = g_mockService]() { return mock->socketEmptyMsgCount >= 1; }));
    EXPECT_EQ(TEST_PORT, g_mockService->lastSocketEmptyPort);
    EXPECT_EQ(TEST_DEVICE_ADDR, g_mockService->lastSocketEmptyAddress);

    ClosePeerFd(sv[1]);
    fdsan_close_with_tag(sv[0], NEARLINK_FDSAN_TAG_SOCKET);
}

/**
 * @tc.number: SleDataTransferFramework_ReceivedData_003
 * @tc.name: 直调 ReceivedData: 垃圾数据反序列化失败
 * @tc.desc: 非法字节流 -> 返回 false
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_ReceivedData_003, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    const std::string junk = "definitely-not-a-serialized-datagram";
    ASSERT_EQ(static_cast<ssize_t>(junk.size()), write(sv[1], junk.data(), junk.size()));

    auto input = std::make_shared<InputStream>(sv[0]);
    bool ret = dt_->pimpl->ReceivedData(input, TEST_PORT, TEST_DEVICE_ADDR);
    EXPECT_FALSE(ret);
    EXPECT_EQ(0, fakeCallback_->receiveCount_);

    ClosePeerFd(sv[1]);
    fdsan_close_with_tag(sv[0], NEARLINK_FDSAN_TAG_SOCKET);
}

/**
 * @tc.number: SleDataTransferFramework_ReceivedData_004
 * @tc.name: 直调 ReceivedData: 弱引用过期回调不分发
 * @tc.desc: 端口对应 weak_ptr 已过期 -> 不触达回调但整体成功
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_ReceivedData_004, TestSize.Level1)
{
    constexpr uint16_t stalePort = TEST_PORT + 7;
    {
        std::shared_ptr<FakeSleDataTransferCallback> temp = std::make_shared<FakeSleDataTransferCallback>();
        dt_->pimpl->callbacks_.EnsureInsert(stalePort, temp);
    } // temp 释放, weak 过期

    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    std::vector<uint8_t> payload = MakeSerializedPayload(stalePort, 4);
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));

    auto input = std::make_shared<InputStream>(sv[0]);
    bool ret = dt_->pimpl->ReceivedData(input, stalePort, TEST_DEVICE_ADDR);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, fakeCallback_->receiveCount_);

    dt_->pimpl->callbacks_.Erase(stalePort);
    ClosePeerFd(sv[1]);
    fdsan_close_with_tag(sv[0], NEARLINK_FDSAN_TAG_SOCKET);
}

/**
 * @tc.number: SleDataTransferFramework_SocketLoop_001
 * @tc.name: socket 工作线程端到端: 数据到达触发读取回调
 * @tc.desc: CONNECTED 建链后对端写入序列化数据, worker 线程读取并进入框架回调链
 *
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_SocketLoop_001, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    NearlinkSleDataTransferConnectionParams connected = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(connected, sv[0]);
    ASSERT_EQ(1, fakeCallback_->connCount_);

    std::vector<uint8_t> payload = MakeSerializedPayload(TEST_PORT, 4);
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));

    EXPECT_TRUE(WaitFor([this]() { return fakeCallback_->receiveCount_ >= 1; })) << "worker data callback timeout";
    EXPECT_EQ(4U, fakeCallback_->lastDataSize_);

    ClosePeerFd(sv[1]);
    // TearDown 中 DestroyPort 回收 sv[0] 与工作线程
}

/**
 * @tc.number: SleDataTransferFramework_SocketLoop_002
 * @tc.name: socket 工作线程端到端: 垃圾数据不崩溃
 * @tc.desc: 对端写入非法字节流, worker 读取后 ReceivedData 失败, 线程继续存活
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_SocketLoop_002, TestSize.Level1)
{
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    NearlinkSleDataTransferConnectionParams connected = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(connected, sv[0]);
    ASSERT_EQ(1, fakeCallback_->connCount_);

    const std::string junk = "garbage-not-a-frame";
    ASSERT_EQ(static_cast<ssize_t>(junk.size()), write(sv[1], junk.data(), junk.size()));
    // 失败路径无外部可见效果: 等待一小段确认线程未崩, 正常数据仍可分发
    usleep(300 * 1000);

    std::vector<uint8_t> payload = MakeSerializedPayload(TEST_PORT, 4);
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));
    EXPECT_TRUE(WaitFor([this]() { return fakeCallback_->receiveCount_ >= 1; })) << "worker alive check timeout";
    EXPECT_EQ(4U, fakeCallback_->lastDataSize_);

    ClosePeerFd(sv[1]);
}

/**
 * @tc.number: SleDataTransferFramework_PortLifecycle_001
 * @tc.name: CreatePort/DestroyPort 注册与销毁闭环
 * @tc.desc: 未注册销毁被拒; 创建成功建立 uuid 到端口映射; 重复创建被拒; 销毁后映射与回调被清理
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_PortLifecycle_001, TestSize.Level1)
{
    EXPECT_EQ(NL_ERR_DATATRANSFER_NO_REGISTER, dt_->DestroyPort(TEST_UUID_CUSTOM));

    g_mockService->createPortRet = NL_NO_ERROR;
    g_mockService->createPortOut = TEST_PORT;
    EXPECT_EQ(NL_NO_ERROR, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    uint16_t port = 0;
    EXPECT_TRUE(dt_->pimpl->uuidAndPort_.GetValue(TEST_UUID_CUSTOM, port));
    EXPECT_EQ(TEST_PORT, port);

    EXPECT_EQ(NL_ERR_DATATRANSFER_DUPLICATE_REGISTER, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    EXPECT_EQ(NL_NO_ERROR, dt_->DestroyPort(TEST_UUID_CUSTOM));
    EXPECT_FALSE(dt_->pimpl->uuidAndPort_.GetValue(TEST_UUID_CUSTOM, port));
    std::weak_ptr<SleDataTransferCallback> remain;
    EXPECT_FALSE(dt_->pimpl->callbacks_.GetValue(TEST_PORT, remain));
}

/**
 * @tc.number: SleDataTransferFramework_PortLifecycle_002
 * @tc.name: CreatePort 服务端返回零端口: 现状返回成功且不建立映射
 * @tc.desc: 服务端返回成功码但端口为 0: 错误码被掩盖且未注册(已登记产品缺陷清单, 产品修复后本断言需同步改为期望错误码)
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_PortLifecycle_002, TestSize.Level1)
{
    g_mockService->createPortRet = NL_NO_ERROR;
    g_mockService->createPortOut = 0;
    EXPECT_EQ(NL_NO_ERROR, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    uint16_t port = 1; // 哨兵: 未注册应查不到
    EXPECT_FALSE(dt_->pimpl->uuidAndPort_.GetValue(TEST_UUID_CUSTOM, port));
}

/**
 * @tc.number: SleDataTransferFramework_PortLifecycle_003
 * @tc.name: Connect/Disconnect 按注册映射端口透传
 * @tc.desc: 未注册 uuid 返回内部错误; 注册后连接/断开把 uuid 映射出的端口带给服务端
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_PortLifecycle_003, TestSize.Level1)
{
    ConnectionParams unregistered;
    unregistered.SetAddress(TEST_DEVICE_ADDR);
    unregistered.SetUuid(TEST_UUID_CUSTOM);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, dt_->Connect(unregistered));

    g_mockService->createPortRet = NL_NO_ERROR;
    g_mockService->createPortOut = TEST_PORT;
    ASSERT_EQ(NL_NO_ERROR, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    g_mockService->connectCount = 0;
    g_mockService->disconnectCount = 0;
    ConnectionParams params;
    params.SetAddress(TEST_DEVICE_ADDR);
    params.SetUuid(TEST_UUID_CUSTOM);
    EXPECT_EQ(NL_NO_ERROR, dt_->Connect(params));
    EXPECT_EQ(1, g_mockService->connectCount);
    EXPECT_EQ(TEST_PORT, g_mockService->lastConnectPort);

    EXPECT_EQ(NL_NO_ERROR, dt_->Disconnect(params));
    EXPECT_EQ(1, g_mockService->disconnectCount);
    EXPECT_EQ(TEST_PORT, g_mockService->lastDisconnectPort);

    EXPECT_EQ(NL_NO_ERROR, dt_->DestroyPort(TEST_UUID_CUSTOM));
}

/**
 * @tc.number: SleDataTransferFramework_PortLifecycle_004
 * @tc.name: GetConnectionState 透传服务端连接状态
 * @tc.desc: 未注册 uuid 返回内部错误; 注册后按映射端口查询并回写服务端状态
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_PortLifecycle_004, TestSize.Level1)
{
    ConnStateParams unregistered;
    unregistered.SetAddress(TEST_DEVICE_ADDR);
    unregistered.SetUuid(TEST_UUID_CUSTOM);
    int32_t state = -1;
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, dt_->GetConnectionState(unregistered, state));

    g_mockService->createPortRet = NL_NO_ERROR;
    g_mockService->createPortOut = TEST_PORT;
    ASSERT_EQ(NL_NO_ERROR, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    g_mockService->getStateCount = 0;
    ConnStateParams params;
    params.SetAddress(TEST_DEVICE_ADDR);
    params.SetUuid(TEST_UUID_CUSTOM);
    state = -1;
    EXPECT_EQ(NL_NO_ERROR, dt_->GetConnectionState(params, state));
    EXPECT_EQ(1, g_mockService->getStateCount);
    EXPECT_EQ(TEST_PORT, g_mockService->lastGetStatePort);
    EXPECT_EQ(static_cast<int32_t>(SleConnectState::CONNECTED), state);

    EXPECT_EQ(NL_NO_ERROR, dt_->DestroyPort(TEST_UUID_CUSTOM));
}

/**
 * @tc.number: SleDataTransferFramework_PortLifecycle_005
 * @tc.name: WriteData 公共写路径: 参数校验与端到端落 socket
 * @tc.desc: 未注册 uuid 返回内部错误; 空数据被拒; 注册并建链后写入成功, 对端收到完整序列化帧
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_PortLifecycle_005, TestSize.Level1)
{
    const uint8_t frame[4] = {0x11, 0x22, 0x33, 0x44};
    DataParams unregistered(TEST_DEVICE_ADDR, TEST_UUID_CUSTOM);
    unregistered.SetData(frame, sizeof(frame));
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, dt_->WriteData(unregistered));

    g_mockService->createPortRet = NL_NO_ERROR;
    g_mockService->createPortOut = TEST_PORT;
    ASSERT_EQ(NL_NO_ERROR, dt_->CreatePort(TEST_UUID_CUSTOM, fakeCallback_));

    DataParams emptyData(TEST_DEVICE_ADDR, TEST_UUID_CUSTOM);
    EXPECT_EQ(NL_ERR_INVALID_PARAM, dt_->WriteData(emptyData));

    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    int peerFlags = fcntl(sv[1], F_GETFL, 0);
    fcntl(sv[1], F_SETFL, peerFlags | O_NONBLOCK);
    NearlinkSleDataTransferConnectionParams connected = MakeConnParams(TEST_PORT,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    dt_->pimpl->callbackImp_->OnConnectionStateChanged(connected, sv[0]);
    ASSERT_EQ(1, fakeCallback_->connCount_);

    DataParams params(TEST_DEVICE_ADDR, TEST_UUID_CUSTOM);
    params.SetData(frame, sizeof(frame));
    EXPECT_EQ(NL_NO_ERROR, dt_->WriteData(params));

    const size_t expectLen = MakeSerializedPayload(TEST_PORT, sizeof(frame)).size();
    uint8_t peerBuf[SOCKET_BUFFER_SIZE];
    ASSERT_TRUE(WaitFor([sv, expectLen, &peerBuf]() {
        return read(sv[1], peerBuf, sizeof(peerBuf)) == static_cast<ssize_t>(expectLen);
    })) << "peer frame timeout";

    EXPECT_EQ(NL_NO_ERROR, dt_->DestroyPort(TEST_UUID_CUSTOM));
    ClosePeerFd(sv[1]);
}

/**
 * @tc.number: SleDataTransferFramework_ReceivedData_005
 * @tc.name: 直调 ReceivedData: 端口未注册直接失败
 * @tc.desc: 端口不在 callbacks_: 反序列化成功也返回 false
 */
HWTEST_F(SleDataTransferFrameworkTest, SleDataTransferFramework_ReceivedData_005, TestSize.Level1)
{
    constexpr uint16_t unknownPort = TEST_PORT + 8;
    int sv[2] = {-1, -1};
    MakeTaggedSocketPair(sv);
    std::vector<uint8_t> payload = MakeSerializedPayload(unknownPort, 4);
    ASSERT_EQ(static_cast<ssize_t>(payload.size()), write(sv[1], payload.data(), payload.size()));

    auto input = std::make_shared<InputStream>(sv[0]);
    EXPECT_FALSE(dt_->pimpl->ReceivedData(input, unknownPort, TEST_DEVICE_ADDR));
    EXPECT_EQ(0, fakeCallback_->receiveCount_);

    ClosePeerFd(sv[1]);
    fdsan_close_with_tag(sv[0], NEARLINK_FDSAN_TAG_SOCKET);
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
