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

#ifndef NEARLINK_PARCEL_SLE_DATATRANSFER_CONNECTION_PARAMS_H
#define NEARLINK_PARCEL_SLE_DATATRANSFER_CONNECTION_PARAMS_H

#include "nearlink_datatransfer_def.h"
#include "parcel.h"

namespace OHOS::Nearlink {
class NearlinkSleDataTransferConnectionParams : public Parcelable, public DataTransferConnectionParams {
public:
    explicit NearlinkSleDataTransferConnectionParams() = default;
    explicit NearlinkSleDataTransferConnectionParams(const DataTransferConnectionParams &other)
        : DataTransferConnectionParams(other)
    {}
    NearlinkSleDataTransferConnectionParams(const NearlinkSleDataTransferConnectionParams &other)
        : DataTransferConnectionParams(other)
    {}
    NearlinkSleDataTransferConnectionParams &operator=(const NearlinkSleDataTransferConnectionParams &other);
    ~NearlinkSleDataTransferConnectionParams() override = default;

    bool Marshalling(Parcel &parcel) const override;
    static NearlinkSleDataTransferConnectionParams *Unmarshalling(Parcel &parcel);

    bool WriteToParcel(Parcel &parcel);
    bool ReadFromParcel(Parcel &parcel);
};
}  // namespace OHOS::Nearlink

#endif  // NEARLINK_PARCEL_SLE_DATATRANSFER_CONNECTION_PARAMS_H
