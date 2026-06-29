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

#ifndef NEARLINK_PARCEL_DATATRANSFER_H
#define NEARLINK_PARCEL_DATATRANSFER_H

#include "nearlink_datatransfer_def.h"

namespace OHOS::Nearlink {
class NearlinkDataTransferDataParams : public DataTransferDataParams {
public:
    NearlinkDataTransferDataParams() = default;
    explicit NearlinkDataTransferDataParams(const DataTransferDataParams &other) : DataTransferDataParams(other) {}
    explicit NearlinkDataTransferDataParams(const NearlinkDataTransferDataParams &other)
        : DataTransferDataParams(other) {}
    NearlinkDataTransferDataParams &operator=(const NearlinkDataTransferDataParams &other);
    ~NearlinkDataTransferDataParams() = default;

    /**
     * @brief Serializes DataTransferDataParams object into a byte stream.
     *
     * @param dataParams The input data to be serialized.
     * @param totalLen   Size of the allocated memory for the serialized byte stream.
     * @return A unique pointer to the serialized byte stream data. Returns nullptr on failure.
     * @since 6
     */
    static std::unique_ptr<uint8_t[], std::default_delete<uint8_t[]>> SerializeData(
        const DataTransferDataParams &dataParams, size_t &totalLen);

    /**
     * @brief Deserializes byte stream data into a DataTransferDataParams object.
     *
     * @param data     The input byte stream data.
     * @param length   Length of the byte stream.
     * @param result   Reference to store the deserialized data.
     * @return True if deserialization succeeded, false otherwise.
     * @since 6
     */
    static bool DeserializeData(const uint8_t* data, size_t length, DataTransferDataParams &result);

    /**
     * @brief Deserializes a byte stream into a list of DataTransferDataParams objects.
     *
     * @param data     The input byte stream data.
     * @param length   Length of the byte stream.
     * @return A vector of deserialized DataTransferDataParams objects.
     * @since 6
     */
    static std::vector<DataTransferDataParams> DeserializeDataList(const uint8_t* data, size_t length);

    /**
     * @brief Writes a string and its length to a byte stream buffer.
     *
     * Writes the length of the string as a 8-byte header(x64), followed by the string content.
     * Updates the offset to the next available position in the buffer.
     *
     * @param out      Output byte stream buffer.
     * @param offset   Reference to the current write position (updated after writing).
     * @param data     Input string to be written.
     * @return Pointer to the next position in the buffer after writing.
     * @since 6
     */
    static uint8_t* WriteLengthAndData(uint8_t* out, size_t &offset, const std::string &data);
};
} // namespace OHOS::Nearlink

#endif // NEARLINK_PARCEL_DATATRANSFER_H