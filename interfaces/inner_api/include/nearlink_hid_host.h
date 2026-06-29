/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines nearlink hid host.
 *
 * @since 6
 */

/**
 * @file nearlink_hid_host.h
 *
 * @brief Framework nearlink hid host interface.
 *
 * @since 6
 */

#ifndef NEARLINK_HID_HOST_H
#define NEARLINK_HID_HOST_H

#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_types.h"

namespace OHOS { class IRemoteObject; }
namespace OHOS {
namespace Nearlink {

class NEARLINK_API NearlinkHidHost {
public:
    /**
     * @brief Get the instance of NearlinkHidHost object.
     *
     * @return Returns the pointer to the NearlinkHidHost instance.
     */
    static NearlinkHidHost *GetProfile();

    /**
     * @brief Nearlink Hid Host Set Report.
     *
     * @param device target device address, address is little endian order.
     * @param type hid report type.
     * @param report report data.
     */
    void HidHostSetReport(std::string device, uint8_t type, std::string &report);

private:
    NearlinkHidHost();
    ~NearlinkHidHost();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkHidHost);
    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_HID_HOST_H