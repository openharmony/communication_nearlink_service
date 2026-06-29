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

#include <gtest/gtest.h>

#include "log.h"
#include "nearlink_datatransfer_parcel.h"
#include "nearlink_datatransfer_def.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class NearlinkDataTransferParcelTest : public testing::Test {
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
HWTEST_F(NearlinkDataTransferParcelTest, SerializeDataAndDeserializeData001, TestSize.Level1)
{
    HILOGI("SerializeDataAndDeserializeData001 start");
    std::string address = "";
    std::string uuid = "";
    uint16_t portId = 30300;
    const std::vector<uint8_t> datas = {1, 2, 3, 4, 5, 6};
    DataTransferDataParams dataParams(address, uuid, portId, datas);
    size_t totalLen = 0;
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(dataParams, totalLen);
    EXPECT_EQ(40, totalLen);

    std::vector<DataTransferDataParams> packageList =
        NearlinkDataTransferDataParams::DeserializeDataList(packageData.get(), totalLen);

    EXPECT_EQ(1, packageList.size());
    HILOGI("SerializeDataAndDeserializeData001 end");
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS