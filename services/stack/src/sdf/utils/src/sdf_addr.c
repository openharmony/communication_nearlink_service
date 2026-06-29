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
#include "securec.h"
#include "sdf_addr.h"

#define SDF_ENC_LOG_ADDR_STR_LEN 17    // strlen("aa:bb:cc:dd:ee:ff")

struct SDF_EncryptedLogString GetEncryptAddr(const SLE_Addr_S *addr)
{
    struct SDF_EncryptedLogString res;
    if (!addr) {
        res.buf[0] = '\0';
        return res;
    }
    // 05:04:**:**:**:00大端序LOG输出地址，addr保持小端序不变
    (void)sprintf_s(res.buf, SDF_ENC_LOG_STR_LEN, "%02X:%02X:**:**:**:%02X(%u)",
        addr->addr[5], addr->addr[4], addr->addr[0], addr->type);   // 2 1 0 for macaddr index

    return res;
}

int SDF_CompareSleAddr(const void *lhs_, const void *rhs_)
{
    SLE_Addr_S *lhs = (SLE_Addr_S *)lhs_;
    SLE_Addr_S *rhs = (SLE_Addr_S *)rhs_;
    if (lhs->type != rhs->type) {
        return lhs->type - rhs->type;
    }
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        if (lhs->addr[i] != rhs->addr[i]) {
            return lhs->addr[i] - rhs->addr[i];
        }
    }
    return 0;
}
