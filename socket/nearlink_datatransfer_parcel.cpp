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

#include "nearlink_datatransfer_parcel.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
    constexpr int DATA_MAX_LEN = UINT16_MAX;
}
bool NearlinkDataTransferDataParams::WriteLengthAndData(uint8_t* out, size_t &offset, const std::string& data)
{
    size_t actualLen = data.size();
    HILOGD("write actualLen : %{public}zu", actualLen);
    // 写入字符串长度
    size_t lenNet = static_cast<size_t>(data.size());

    if (memcpy_s(out + offset, sizeof(lenNet), &lenNet, sizeof(lenNet)) != EOK) {
        return false;
    }
    offset += sizeof(lenNet);

    // 写入字符串内容
    if (memcpy_s(out + offset, lenNet, data.c_str(), data.size()) != EOK) {
        return false;
    }
    offset += actualLen;

    return true;
};

std::unique_ptr<uint8_t[]> NearlinkDataTransferDataParams::SerializeData(const DataTransferDataParams& dataParams,
    size_t& totalLen)
{
    // 计算总长度
    size_t addressLen = dataParams.address_.size();
    size_t uuidLen = dataParams.uuid_.size();
    size_t dataLen = dataParams.data_.size();

    totalLen =
        sizeof(size_t) +                       // 总长度
        sizeof(size_t) + addressLen +          // address
        sizeof(size_t) + uuidLen +             // uuid
        sizeof(dataParams.port_) +             // port
        sizeof(size_t) + dataLen;              // data
    HILOGD("totalLen: %{public}zu", totalLen);
    HILOGD("address len: %{public}zu, uuidLen len: %{public}zu, port_ len: %{public}zu, dataLen len: %{public}zu",
        sizeof(size_t) + addressLen, sizeof(size_t) + uuidLen, sizeof(dataParams.port_),  sizeof(size_t) + dataLen);

    // 分配输出缓冲区
    auto outBuffer = std::make_unique<uint8_t[]>(totalLen);
    size_t offset = 0;

    // 编码 数据包总长度 字段 (防止粘包)
    (void)memcpy_s(outBuffer.get() + offset, sizeof(totalLen), &totalLen, sizeof(totalLen));
    offset += sizeof(totalLen);

    // 编码 address_ 字段
    NL_CHECK_RETURN_RET(WriteLengthAndData(outBuffer.get(), offset, dataParams.address_),
        nullptr, "serialize address err");

    HILOGD("offset: %{public}zu", offset);
    // 编码 uuid_ 字段
    NL_CHECK_RETURN_RET(WriteLengthAndData(outBuffer.get(), offset, dataParams.uuid_),
        nullptr, "serialize uuid err");

    HILOGD("offset: %{public}zu", offset);
    // 编码 port_ 字段
    (void)memcpy_s(outBuffer.get() + offset, sizeof(dataParams.port_), &dataParams.port_, sizeof(dataParams.port_));
    offset += sizeof(dataParams.port_);

    // 编码 data_ 字段
    if (!dataParams.data_.empty()) {
        (void)memcpy_s(outBuffer.get() + offset, sizeof(dataLen), &dataLen, sizeof(dataLen));
        offset += sizeof(dataLen);
        (void)memcpy_s(outBuffer.get() + offset, dataLen, dataParams.data_.data(), dataLen);
        offset += dataLen;
    } else {
        // data_ 为空时, 填充 0
        size_t tmp = 0;
        (void)memcpy_s(outBuffer.get() + offset, sizeof(dataLen), &tmp, sizeof(tmp));
        offset += sizeof(tmp);
    }
    HILOGD("offset: %{public}zu total_len: %{public}zu", offset, totalLen);
    NL_CHECK_RETURN_RET(offset == totalLen, nullptr, "serialize dataParams length err");
    return outBuffer;
};

std::vector<DataTransferDataParams> NearlinkDataTransferDataParams::DeserializeDataList(const uint8_t *data,
    size_t length)
{
    const uint8_t *ptr = data;
    const uint8_t *end = data + length;
    size_t offset = 0;
    HILOGD("input dataList length : %{public}zu", length);
    std::vector<DataTransferDataParams> dataList;
    while (ptr < end) {
        if (static_cast<size_t>(end - ptr) < sizeof(size_t)) {
            HILOGE("deserialize data package length err");
            dataList.clear();
            return dataList;
        }
        size_t packageLen = *reinterpret_cast<const size_t*>(ptr);
        ptr += sizeof(packageLen);
        if (packageLen <= sizeof(size_t)) {
            HILOGE("package len too small: %{public}zu", packageLen);
            dataList.clear();
            return dataList;
        }
        packageLen -= sizeof(size_t);

        if (static_cast<size_t>(end - ptr) < packageLen) {
            HILOGE("deserialize data length err");
            dataList.clear();
            return dataList;
        }
        DataTransferDataParams result;
        bool ret = DeserializeData(ptr, packageLen, result);
        if (!ret) {
            HILOGE("deserialize data err");
            dataList.clear();
            return dataList;
        }
        dataList.push_back(result);
        ptr += packageLen;
        offset += packageLen;
        HILOGD("offset : %{public}zu", offset);
    }
    return dataList;
}

bool NearlinkDataTransferDataParams::DeserializeData(const uint8_t *data, size_t length,
    DataTransferDataParams &result)
{
    const uint8_t *ptr = data;
    const uint8_t *end = data + length;
    HILOGD("input data length : %{public}zu", length);

    // 1. address_
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= sizeof(size_t), false, "deserialize address length err");
    size_t addressLen = *reinterpret_cast<const size_t*>(ptr);
    ptr += sizeof(addressLen);
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= addressLen, false, "deserialize address err");
    result.address_ = std::string(reinterpret_cast<const char*>(ptr), addressLen);
    ptr += addressLen;

    // 2. uuid_
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= sizeof(size_t), false, "deserialize uuid length err");
    size_t uuidLen = *reinterpret_cast<const size_t*>(ptr);
    ptr += sizeof(uuidLen);
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= uuidLen, false, "deserialize UUID err");
    result.uuid_ = std::string(reinterpret_cast<const char*>(ptr), uuidLen);
    ptr += uuidLen;

    // 3. port_
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= sizeof(uint16_t), false, "deserialize port err");
    result.port_ = *reinterpret_cast<const uint16_t*>(ptr);
    ptr += sizeof(uint16_t);

    // 4. data_
    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= sizeof(size_t), false, "deserialize data length err");
    size_t dataLen = *reinterpret_cast<const size_t*>(ptr);
    ptr += sizeof(dataLen);
    NL_CHECK_RETURN_RET(dataLen <= DATA_MAX_LEN, false, "data length is err, length=%{public}zu", dataLen);

    NL_CHECK_RETURN_RET(static_cast<size_t>(end - ptr) >= dataLen, false, "deserialize data err");
    result.data_ = std::vector<uint8_t>(ptr, ptr + dataLen);
    ptr += dataLen;
    // 4. 最后检查是否完全解析完成
    return ptr == end;
};
}
}