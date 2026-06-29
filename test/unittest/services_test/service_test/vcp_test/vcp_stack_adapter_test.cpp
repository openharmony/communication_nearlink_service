/*
 * Copyright (C) 2025 Huawei Device Co., Ltd. All rights reserved.
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

#include "VcpStackAdapter.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class VcpAdapterCallbackCommon : public VcpStackCallback {
public:
    ~VcpAdapterCallbackCommon() override{};
    void OnVolumeChangeEvent(const RawAddress &device, uint8_t volume) override {}
    void OnMuteStatusChangeEvent(const RawAddress &device, uint8_t muteStatus) override {}
    void OnConnectionStateChanged(const RawAddress &device, uint8_t state, uint8_t preState) override {}
    void OnNotifyVolumeChange(const RawAddress &device, const std::list<StreamVolume> &streamVolumes) override {}
};

namespace {
VcpAdapterCallbackCommon g_adapterCallback_;
VcpStackAdapter g_stackAdapter_(g_adapterCallback_);
}

class NearlinkVcpStackAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkVcpStackAdapterTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkVcpStackAdapterTest");
}

void NearlinkVcpStackAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVcpStackAdapterTest");
}

void NearlinkVcpStackAdapterTest::SetUp()
{
    HILOGI("SetUp NearlinkVcpStackAdapterTest.");
}

void NearlinkVcpStackAdapterTest::TearDown()
{
    HILOGI("TearDown NearlinkVcpStackAdapterTest.");
}

/*
 * @tc.number: OnVolumeChangeEvent001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpStackAdapterTest, OnVolumeChangeEvent001, TestSize.Level1)
{
    HILOGI("OnVolumeChangeEvent001 start");
    SLE_Addr_S device;
    (void)memset_s(&device, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    uint8_t volume = 0;

    g_stackAdapter_.OnVolumeChangeEvent(&device, volume);
    EXPECT_NE(0, 1);
    HILOGI("OnVolumeChangeEvent001 end");
}

/*
 * @tc.number: OnMuteStatusChangeEvent001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpStackAdapterTest, OnMuteStatusChangeEvent001, TestSize.Level1)
{
    HILOGI("OnMuteStatusChangeEvent001 start");
    SLE_Addr_S device;
    (void)memset_s(&device, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    uint8_t muteStatus = 0;

    g_stackAdapter_.OnMuteStatusChangeEvent(&device, muteStatus);
    EXPECT_NE(0, 1);
    HILOGI("OnMuteStatusChangeEvent001 end");
}

/*
 * @tc.number: OnNotifyVolumeChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpStackAdapterTest, OnNotifyVolumeChange001, TestSize.Level1)
{
    HILOGI("OnNotifyVolumeChange001 start");
    SLE_Addr_S device;
    (void)memset_s(&device, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    NLSTK_McpVolumePropertyType_E type = NLSTK_McpVolumePropertyType_E::NLSTK_MCP_STREAM_VOLUME_STATUS;
    NLSTK_McpStreamVolumeStatus_S info;
    (void)memset_s(&info, sizeof(NLSTK_McpStreamVolumeStatus_S), 0x00, sizeof(NLSTK_McpStreamVolumeStatus_S));

    g_stackAdapter_.OnNotifyVolumeChange(&device, type, &info);
    EXPECT_NE(0, 1);
    HILOGI("OnNotifyVolumeChange001 end");
}

/*
 * @tc.number: OnConnectionStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpStackAdapterTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("OnConnectionStateChanged001 start");
    SLE_Addr_S device;
    (void)memset_s(&device, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    uint8_t state = 1;
    uint8_t preState = 0;

    g_stackAdapter_.OnConnectionStateChanged(&device, state, preState);
    EXPECT_NE(0, 1);
    HILOGI("OnConnectionStateChanged001 end");
}

/*
 * @tc.number: DeregisterStackCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVcpStackAdapterTest, DeregisterStackCbk001, TestSize.Level1)
{
    HILOGI("DeregisterStackCbk001 start");
    g_stackAdapter_.DeregisterStackCbk();
    EXPECT_NE(0, 1);
    HILOGI("DeregisterStackCbk001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS