/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SSAP_INNER_DEF_H
#define SSAP_INNER_DEF_H

#define SSAP_HASH_BYTES 16
#define ACK_OK 1
#define ACK_ERR 0

#define SSAP_MTU_DEFAULT                     251
#define SSAP_MTU_MIN                         22
#define SSAP_MTU_MAX                         1024

// SSAP_VALUE_NTF / IND PDU Message control code attribute/event indicator bit
#define NTF_IND_PROP_EVENT 0x4

namespace OHOS {
namespace Nearlink {

enum SsapOpInd : uint32_t {
    SSAP_OP_IND_READ = 1,
    SSAP_OP_IND_WRITE_CMD = 1 << 1,
    SSAP_OP_IND_WRITE_REQ = 1 << 2,
    SSAP_OP_IND_NTF = 1 << 3,
    SSAP_OP_IND_IND = 1 << 4,
    SSAP_OP_IND_ADV = 1 << 5,
    SSAP_OP_IND_DESC = 1 << 8,
};

enum ClientReq {
    REQ_NONE,
    REQ_EXCHANGE_MTU,
    REQ_FIND_STRUCTURE,
    REQ_FIND_SERVICE_BY_UUID,
    REQ_READ,
    REQ_READ_BY_UUID,
    REQ_WRITE,
    REQ_CALL,
};

enum ClientCmd {
    CMD_NONE,
    CMD_WRITE,
    CMD_CALL,
};

enum ServerInd {
    IND_NONE,
    IND_VALUE,
};

enum ServerNtf {
    NTF_NONE,
    NTF_VALUE,
};

enum ServerIvk {
    IVK_NONE,
    IVK_ADD_SERVICE,
    IVK_SET_ENTRY,
};

} // namespace Nearlink
} // namespace OHOS
#endif // SSAP_INNER_DEF_H