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

#ifndef NEARLINK_PARCEL_SLE_UUID_H
#define NEARLINK_PARCEL_SLE_UUID_H

#include "sle_uuid.h"
#include "parcel.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief This class provides Nearlink uuid.
 *
 * @since 6
 */
class NearlinkUuidParcel : public Parcelable, public Uuid {
public:
    /**
     * @brief A constructor used to create an <b>NearlinkUuidParcel</b> instance.
     *
     * @since 6
     */
    NearlinkUuidParcel() = default;

    /**
     * @brief A constructor used to create an <b>NearlinkUuidParcel</b> instance.
     *
     * @param other Other Uuid to create a new NearlinkUuidParcel.
     * @since 6
     */
    explicit NearlinkUuidParcel(const Uuid &other) :Uuid(other)
    {}  // NOLINT(implicit)

    /**
     * @brief A destructor used to delete the <b>NearlinkUuidParcel</b> instance.
     *
     * @since 6
     */
    ~NearlinkUuidParcel() override = default;

    NearlinkUuidParcel(const NearlinkUuidParcel &other) : Uuid(other)
    {}
    NearlinkUuidParcel& operator = (const NearlinkUuidParcel &other);

    bool Marshalling(Parcel &parcel) const override;

    static NearlinkUuidParcel *Unmarshalling(Parcel &parcel);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_PARCEL_SLE_UUID_H