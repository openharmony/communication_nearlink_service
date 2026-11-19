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
 * 端口套件收尾测试：数据传输参数序列化空负载分支、输出流写入门与错误路径、
 * 端口套接字对的创建/监听/线程幂等/收发回调/销毁匹配与超限告警、
 * 管道描述符的能力检查分支。
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

#include "nearlink_datatransfer_def.h"
#include "nearlink_fdsan_tag.h"
#include "nearlink_datatransfer_parcel.h"
// ponytail: 产品头文件将 SetNonblock 定义为非内联函数，两个 TU 同时含入即 ODR 重定义；
// 本测试 TU 内重命名规避（升级路径：产品侧改 inline 后可去掉本宏）。
#define SetNonblock SetNonblockSocketTailLocal
#include "nearlink_socket_manager.h"
#undef SetNonblock
#include "nearlink_socket_outputstream.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

namespace {
constexpr uint16_t TEST_PORT = 0x0102;
constexpr const char *TEST_ADDR = "11:22:33:44:55:66";
} // namespace

/* 数据传输参数序列化：带负载与空负载经列表反序列化都能取回原值 */
HWTEST(NlSocketTailTest, ParcelRoundTripTest_001, TestSize.Level1)
{
    // 序列化输出带总长度包头，须按列表入口反序列化逐包剥离包头
    DataTransferDataParams withData(TEST_ADDR, "uuid-1", TEST_PORT, {0x01, 0x02, 0x03});
    size_t totalLen = 0;
    auto bytes = NearlinkDataTransferDataParams::SerializeData(withData, totalLen);
    ASSERT_NE(bytes, nullptr);
    ASSERT_GT(totalLen, 0U);
    auto list = NearlinkDataTransferDataParams::DeserializeDataList(bytes.get(), totalLen);
    ASSERT_EQ(list.size(), 1U);
    EXPECT_EQ(list[0].address_, TEST_ADDR);
    EXPECT_EQ(list[0].uuid_, std::string("uuid-1"));
    EXPECT_EQ(list[0].port_, TEST_PORT);
    EXPECT_EQ(list[0].data_.size(), 3U);

    // 空负载：长度字段为零，往返后负载保持为空
    DataTransferDataParams empty(TEST_ADDR, "uuid-2", TEST_PORT, {});
    size_t emptyLen = 0;
    auto emptyBytes = NearlinkDataTransferDataParams::SerializeData(empty, emptyLen);
    ASSERT_NE(emptyBytes, nullptr);
    auto emptyList = NearlinkDataTransferDataParams::DeserializeDataList(emptyBytes.get(), emptyLen);
    ASSERT_EQ(emptyList.size(), 1U);
    EXPECT_TRUE(emptyList[0].data_.empty());
}

/* 输出流写入门：有效描述符小包写入成功，无效描述符按内部错误返回 */
HWTEST(NlSocketTailTest, OutputStreamWriteTest_001, TestSize.Level1)
{
    int fds[2] = {-1, -1};
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    OutputStream out(fds[0]);
    uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
    EXPECT_EQ(out.Write(data, sizeof(data)), SLE_TRANS_RESULT_SUCCESS);

    // 已关闭的描述符：入口判空直接按内部错误返回
    OutputStream closed(-1);
    EXPECT_EQ(closed.Write(data, sizeof(data)), SLE_TRANS_RESULT_INTERNAL_FAULT);

    // 失效但非负的描述符：容量检查拿不到选项，写入失败同样按内部错误返回
    int dead = open("/dev/null", O_RDONLY);
    ASSERT_GE(dead, 0);
    close(dead);
    OutputStream broken(dead);
    EXPECT_EQ(broken.Write(data, sizeof(data)), SLE_TRANS_RESULT_INTERNAL_FAULT);
    close(fds[0]);
    close(fds[1]);
}

/* 端口套接字对：创建后线程幂等，监听后对端写入触发数据回调，
 *   超过协商最大传输单元的数据仍告警放行，销毁按端口与地址精确匹配 */
HWTEST(NlSocketTailTest, SocketPairLifecycleTest_001, TestSize.Level1)
{
    PortSocketManager manager;
    std::atomic<int> gotFrames(0);
    std::atomic<bool> gotData(false);
    auto callback = [&](std::shared_ptr<InputStream> stream, uint16_t port, std::string addr) {
        gotFrames++;
        uint8_t buf[8] = {0};
        if (stream != nullptr && stream->Read(buf, sizeof(buf)) > 0) {
            gotData = true;
        }
    };

    int fd = manager.CreateSocketPair(TEST_PORT, TEST_ADDR, 4, callback);
    ASSERT_GE(fd, 0);
    // 线程幂等：重复启动直接返回
    manager.RunThread();
    manager.RunThread();
    manager.Listen(TEST_PORT, TEST_ADDR);

    // 对端写入：回调触发并读回数据
    uint8_t payload[2] = {0xAA, 0xBB};
    ASSERT_EQ(write(fd, payload, sizeof(payload)), static_cast<ssize_t>(sizeof(payload)));
    for (int i = 0; i < 100 && !gotData.load(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(gotData.load());

    // 超过最大传输单元的数据：告警但仍放行写入
    uint8_t big[8] = {0};
    EXPECT_EQ(manager.SendData(TEST_PORT, TEST_ADDR, big, sizeof(big), sizeof(big)),
        SLE_TRANS_RESULT_SUCCESS);

    // 未知端口与不匹配地址的销毁：精确匹配不误删
    manager.DestroyPort(TEST_PORT + 1);
    manager.DestroyPeerPort(TEST_PORT, "22:22:22:22:22:22");
    manager.DestroyPeerPort(TEST_PORT, TEST_ADDR);
    manager.DestroyPort(TEST_PORT);
    // 返回 fd 已带套接字 tag：按配对方式关闭
    fdsan_close_with_tag(fd, NEARLINK_FDSAN_TAG_SOCKET);
}

/* 管道描述符注入：只读端与只写端分别走能力检查的对应分支，均安全完成 */
HWTEST(NlSocketTailTest, SetSocketCapabilityTest_001, TestSize.Level1)
{
    PortSocketManager manager;
    auto callback = [](std::shared_ptr<InputStream> stream, uint16_t port, std::string addr) {};
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    // 管道描述符移交前先打套接字 tag，销毁路径按同 tag 配对关闭
    fdsan_exchange_owner_tag(fds[0], 0, NEARLINK_FDSAN_TAG_SOCKET);
    fdsan_exchange_owner_tag(fds[1], 0, NEARLINK_FDSAN_TAG_SOCKET);
    manager.SetSocket(TEST_PORT, TEST_ADDR, 4, fds[0], callback);
    manager.SetSocket(TEST_PORT + 1, TEST_ADDR, 4, fds[1], callback);
    manager.DestroyPort(TEST_PORT);
    manager.DestroyPort(TEST_PORT + 1);
    SUCCEED();
}
