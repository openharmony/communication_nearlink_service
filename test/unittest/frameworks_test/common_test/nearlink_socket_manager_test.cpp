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

#include "log.h"
#include "nearlink_socket_manager.h"
#include "nearlink_datatransfer_def.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkSocketManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    static void TearDownTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

/**
 * @tc.name: SerializeDataAndDeserializeData001
 * @tc.desc: serialize data function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSocketManagerTest, CreateSocketPairAndSetSocket001, TestSize.Level1)
{
    HILOGI("CreateSocketPair001 start");
    std::shared_ptr<PortSocketManager> serviceManager = std::make_shared<PortSocketManager>();
    std::shared_ptr<PortSocketManager> fwkManager = std::make_shared<PortSocketManager>();
    uint16_t portId = 30300;
    std::string address = "00:00:00:00:00:00";
    const size_t totalLen = 6;
    const size_t dataLen = 3;
    const std::array<uint8_t, totalLen> data = {1, 2, 3, 4, 5, 6};
    uint16_t mtu = 65535;

    bool serviceAck = false;
    bool fwkAck = false;

    auto serviceDataCallback = [&serviceAck](std::shared_ptr<InputStream> inputStream, uint16_t port,
        std::string addr) {
        serviceAck = true;
        HILOGI("service data callback success");
    };
    auto fwkDataCallback = [&fwkAck](std::shared_ptr<InputStream> inputStream, uint16_t port, std::string addr) {
        fwkAck = true;
        HILOGI("frame data callback success");
    };
    int fd = serviceManager->CreateSocketPair(portId, address, mtu, serviceDataCallback);
    EXPECT_EQ(fd > 0, true);
    serviceManager->Listen(portId, address);
    serviceManager->RunThread();

    fwkManager->SetSocket(portId, address, mtu, fd, fwkDataCallback);
    fwkManager->Listen(portId, address);
    fwkManager->RunThread();

    fwkManager->SendData(portId, address, data.begin(), totalLen, dataLen);
    sleep(1);
    EXPECT_EQ(serviceAck, true);

    serviceManager->SendData(portId, address, data.begin(), totalLen, dataLen);
    sleep(1);
    EXPECT_EQ(fwkAck, true);

    serviceManager->DestroyPort(portId);

    fwkManager->DestroyPeerPort(portId, address);
    HILOGI("CreateSocketPair001 end");
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS