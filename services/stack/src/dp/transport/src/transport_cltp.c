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

#include "transport_cltp.h"

#include "securec.h"

#include "byte_codec.h"
#include "dpfwk_log.h"
#include "sdf_buff.h"
#include "transport_errno.h"

int32_t TRANS_CltpPktProc(SDF_Buff_S *buff)
{
    uint64_t dataLen = SDF_DataLenGet(buff);
    if (dataLen < sizeof(TRANS_CltpHeader_S)) {
        TP_LOGE("invalid dataLen = %llu", dataLen);
        return TRANS_FAIL;
    }
    TRANS_CltpHeader_S *cltpHeader = (TRANS_CltpHeader_S *)SDF_DataOffset(buff);
    uint8_t optionLen = cltpHeader->basic.optionLen * TRANS_OPT_LEN_MULTYPLY;
    if (optionLen == 0) {
        if (SDF_BuffTrimPrefix(buff, sizeof(TRANS_CltpHeader_S)) == NULL) {
            TP_LOGE("trim transport cltp header failed");
            return TRANS_FAIL;
        }
        return TRANS_SUCCESS;
    }

    if (dataLen < sizeof(TRANS_CltpHeader_S) + sizeof(TRANS_CltpHeaderOpts_S)) {
        TP_LOGE("invalid dataLen = %llu", dataLen);
        return TRANS_FAIL;
    }
    TRANS_CltpHeaderOpts_S *defaultOpts = (TRANS_CltpHeaderOpts_S *)cltpHeader->option;
    uint16_t optionBitmap = DECODE2BYTE_BIG((uint8_t *)&(defaultOpts->optionBitmap));
    if (!(optionBitmap & TRANS_CLTP_OPT_PAYLOAD_MASK)) {
        TP_LOGE("invalid bitmap = %04x", optionBitmap);
        return TRANS_FAIL;
    }

    uint16_t payloadLen = DECODE2BYTE_BIG((uint8_t *)&(defaultOpts->payloadLen));
    if (dataLen < sizeof(TRANS_CltpHeader_S) + optionLen + payloadLen) {
        TP_LOGE("invalid dataLen = %llu", dataLen);
        return TRANS_FAIL;
    }
    if (SDF_BuffTrimPrefix(buff, sizeof(TRANS_CltpHeader_S) + optionLen) == NULL) {
        TP_LOGE("trim transport cltp header failed");
        return TRANS_FAIL;
    }
    return TRANS_SUCCESS;
}

uint32_t TRANS_CltpHeaderBuild(uint16_t srcPort, uint16_t dstPort, SDF_Buff_S *buff)
{
    TRANS_CltpHeader_S *cltpHeader = NULL;
    uint16_t headerSize = TRANS_CltpHeaderSize();
    uint16_t payloadLen = SDF_DataLenGet(buff);

    cltpHeader = (TRANS_CltpHeader_S *)SDF_BuffPrepend(buff, headerSize);
    if (cltpHeader == NULL) {
        TP_LOGE("append failed");
        return TRANS_SEND_DATA_APPEND_FAILED;
    }
    cltpHeader->basic.version = TRANS_PROTO_VERSION;
    cltpHeader->basic.optionLen = sizeof(TRANS_CltpHeaderOpts_S) / TRANS_OPT_LEN_MULTYPLY;
    ENCODE2BYTE_BIG((uint8_t *)&(cltpHeader->basic.srcPort), srcPort);
    ENCODE2BYTE_BIG((uint8_t *)&(cltpHeader->basic.dstPort), dstPort);
    TRANS_CltpHeaderOpts_S *defaultOpts = (TRANS_CltpHeaderOpts_S *)cltpHeader->option;
    ENCODE2BYTE_BIG((uint8_t *)&(defaultOpts->optionBitmap), TRANS_CLTP_OPT_PAYLOAD_MASK);
    ENCODE2BYTE_BIG((uint8_t *)&(defaultOpts->payloadLen), payloadLen);

    return TRANS_SUCCESS;
}