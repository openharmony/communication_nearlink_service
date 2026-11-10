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
//       socketpair+worker 线端到端; 覆盖 95/102/110/117/225/229/463/468/583/594/599 行分支。
//       225 的 transfer==nullptr 腿结构不可达(lambda 弱引用过期要求 impl 先亡, 而回调
//       存储于 impl 自身的 PortSocketManager 中), 不设用例。

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "nearlink_access_token_mock.h"

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

    NlErrCode Connect(NearlinkSleDataTransferConnectionParams &params) override
    {
        (void)params;
        return NL_NO_ERROR;
    }

    NlErrCode Disconnect(NearlinkSleDataTransferConnectionParams &params) override
    {
        (void)params;
        return NL_NO_ERROR;
    }

    NlErrCode GetConnectionState(NearlinkSleDataTransferConnectionParams &params, int32_t &connState) override
    {
        (void)params;
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
        InjectMockDataTransferService();
    }

    static void TearDownTestCase()
    {
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
 * @tc.desc: 非空源深拷贝(583 true 腿)/空源置空(583 false 腿)
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
 * @tc.desc: 自赋值不变(594 false 腿)/赋非空深拷贝(599 true 腿)/赋空重置(599 false 腿)
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
 * @tc.desc: 端口不在 callbacks_: 不触达框架回调(95 true 腿), fd=-1 不关闭
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
 * @tc.desc: 端口未注册且 fd 有效 -> fdsan 关闭该 fd(95 true 腿 + fd 关闭腿)
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
 * @tc.desc: 已连接但 fd=-1: 不建 socket(102/117 的 fd==-1 腿), 回调收到状态与 mtu
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
 * @tc.desc: 已连接且 fd 有效: SetSocketDataTransfer 落地(102 true 腿), Listen/RunThread
 *           启动(117 true 腿), SendData 可写通
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
 * @tc.desc: 断连状态走 DestroyPeerPort(110 true 腿): 先建后断, socket 被回收
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
 * @tc.desc: 反序列化成功 -> 回调 OnReceiveData(463 true 腿), 小于阈值不触发 SocketEmptyMsg(468 false 腿)
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
 * @tc.desc: 读回字节数 >= SOCKET_BUFFER_THRESHOLD_REPORT -> 通知服务侧腾空(468 true 腿)
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
 * @tc.desc: 非法字节流 -> 返回 false(229 false 腿等价路径)
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
 * @tc.desc: 端口对应 weak_ptr 已过期 -> 不触达回调但整体成功(463 false 腿)
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
 *           (225 transfer 非空腿 + 229 成功腿)
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
 * @tc.desc: 对端写入非法字节流, worker 读取后 ReceivedData 失败(229 失败腿), 线程继续存活
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
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
