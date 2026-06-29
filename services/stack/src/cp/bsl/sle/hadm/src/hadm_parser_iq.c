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
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "securec.h"
#include "dli_event_struct.h"
#include "hadm_parser_iq.h"

#define HADM_IQ_LOW_BIT 0x0F
#define HADM_IQ_HIGH_BIT 0xF0
#define HADM_OFFSET_8BIT 0x8
#define HADM_OFFSET_4BIT 0x4
#define HADM_IQ_LEN      0x3
#define HADM_MULTI_CHNL_NUM 4
#define HADM_CHNL_MAP_INDEX_MAXNUM 9
#define HADM_CHNL_HIGH_SEVEN_BIT 0x7F

#define PARSER_DATA_BY_LEN(TAG, DATA, LEN) do { \
    if (memcpy_s(TAG, LEN, DATA, LEN) != EOK) { \
        NLSTK_LOG_ERROR("[HADM] %s memcpy_s %s fail", __func__, #TAG); \
    }   \
    (DATA) += (LEN); \
} while (0)

static uint32_t HadmCountSetBits(const uint8_t *chmap)
{
    uint32_t setBits = 0;
    for (int i = 0; i < HADM_CHNL_MAP_INDEX_MAXNUM; i++) {
        uint8_t byteVal = chmap[i];
        while (byteVal > 0) {
            setBits += byteVal & 1;
            byteVal >>= 1;
        }
    }
    uint8_t byteVal = chmap[HADM_CHNL_MAP_INDEX_MAXNUM] & HADM_CHNL_HIGH_SEVEN_BIT;
    while (byteVal > 0) {
        setBits += byteVal & 1;
        byteVal >>= 1;
    }
    return setBits;
}

static uint32_t HadmGetChnlNum(HadmSlemChmap_S *slemChmapFromDli, uint8_t isMultiTone)
{
    uint32_t iqChnlNum = 0;
    if (slemChmapFromDli == NULL) {
        return iqChnlNum;
    }
    iqChnlNum = HadmCountSetBits(slemChmapFromDli->chmap);
    if (isMultiTone) {
        iqChnlNum *= HADM_MULTI_CHNL_NUM;
    }
    return iqChnlNum;
}

void HadmFreeIqInfo(HadmIqInfoFromDli_S *iqInfo)
{
    if (iqInfo == NULL) {
        return;
    }
    if (iqInfo->iqData != NULL) {
        SDF_MemFree(iqInfo->iqData);
        iqInfo->iqData = NULL;
    }
    SDF_MemFree(iqInfo);
    return;
}

static HadmIqValue_S *HadmGetIqData(uint8_t *data, uint32_t iqChnlNum)
{
    HadmIqValue_S *iqDataArray = (HadmIqValue_S *)SDF_MemZalloc(sizeof(HadmIqValue_S) * iqChnlNum);
    NLSTK_CHECK_RETURN(iqDataArray != NULL, NULL, "[HADM] HadmPaserIqInfoFromDli malloc iqData fail");
    uint8_t *iqInfoFromDli = data;
    for (uint32_t i = 0; i < iqChnlNum; i++) {
        HadmIqValue_S *iqData = &(iqDataArray[i]);
        uint8_t firstByte = *(uint8_t *)iqInfoFromDli;
        iqInfoFromDli += sizeof(uint8_t);
        uint8_t secondByte = *(uint8_t *)iqInfoFromDli;
        iqInfoFromDli += sizeof(uint8_t);
        uint8_t thirdByte = *(uint8_t *)iqInfoFromDli;
        iqInfoFromDli += sizeof(uint8_t);
        iqData->iData = ((uint16_t)firstByte) | ((uint16_t)(secondByte & HADM_IQ_LOW_BIT) << HADM_OFFSET_8BIT);
        iqData->qData = ((uint16_t)thirdByte << HADM_OFFSET_4BIT) |
                        ((uint16_t)(secondByte & HADM_IQ_HIGH_BIT) >> HADM_OFFSET_4BIT);
    }
    return iqDataArray;
}

HadmIqInfoFromDli_S *HadmPaserIqInfoFromDli(uint8_t *data, size_t dataLen)
{
    NLSTK_CHECK_RETURN(data != NULL, NULL, "[HADM] HadmPaserIqInfoFromDli data is NULL dataLen(%d) ", dataLen);
    uint8_t *iqInfoFromDli = data;
    HadmIqInfoFromDli_S *iqInfo = (HadmIqInfoFromDli_S *)SDF_MemZalloc(sizeof(HadmIqInfoFromDli_S));
    NLSTK_CHECK_RETURN(iqInfo != NULL, NULL, "[HADM] HadmPaserIqInfoFromDli malloc iqInfo fail");
    // 1. 解析头部信息
    PARSER_DATA_BY_LEN(&iqInfo->status, iqInfoFromDli, sizeof(uint8_t));
    PARSER_DATA_BY_LEN(&iqInfo->connHandle, iqInfoFromDli, sizeof(uint16_t));
    PARSER_DATA_BY_LEN(&iqInfo->slemIdx, iqInfoFromDli, sizeof(uint8_t));
    PARSER_DATA_BY_LEN(&iqInfo->slemInfoType, iqInfoFromDli, sizeof(DLI_SlemInfoType));
    PARSER_DATA_BY_LEN(&iqInfo->timeStampSn, iqInfoFromDli, sizeof(uint32_t));
    PARSER_DATA_BY_LEN(&(iqInfo->slemChmap), iqInfoFromDli, sizeof(HadmSlemChmap_S));
    // 2. 判断是否为多音
    uint16_t slemInfoTypeValue = *(uint16_t *)&iqInfo->slemInfoType;
    uint8_t isBit10Set = ((slemInfoTypeValue >> 10) & 0x1);
    iqInfo->isMultiTone = isBit10Set;
    // 3. 计算 chnlNum
    iqInfo->iqChnlNum = HadmGetChnlNum(&(iqInfo->slemChmap), isBit10Set);
    // 4. 解析 tofResult, rssi, iqBitLen
    PARSER_DATA_BY_LEN(&iqInfo->tofResult, iqInfoFromDli, sizeof(uint32_t));
    PARSER_DATA_BY_LEN(&iqInfo->rssi, iqInfoFromDli, sizeof(uint8_t));
    PARSER_DATA_BY_LEN(&iqInfo->iqBitLen, iqInfoFromDli, sizeof(uint8_t));
    // 5. 比特 10=1 时有 1 字节多音信息
    if (isBit10Set) {
        iqInfoFromDli += sizeof(uint8_t);
    }
    // 6. 解析 IQ 数据
    iqInfo->iqData = HadmGetIqData(iqInfoFromDli, iqInfo->iqChnlNum);
    if (iqInfo->iqData == NULL) {
        SDF_MemFree(iqInfo);
        return NULL;
    }
    iqInfoFromDli += iqInfo->iqChnlNum * HADM_IQ_LEN;
    // 7. 解析尾部 (localId/remoteId/venderLen)
    if (isBit10Set) {
        PARSER_DATA_BY_LEN(&iqInfo->localId, iqInfoFromDli, sizeof(uint8_t));
        PARSER_DATA_BY_LEN(&iqInfo->remoteId, iqInfoFromDli, sizeof(uint8_t));
        PARSER_DATA_BY_LEN(&iqInfo->venderLen, iqInfoFromDli, sizeof(uint8_t));
    } else {
        PARSER_DATA_BY_LEN(&iqInfo->venderLen, iqInfoFromDli, sizeof(uint8_t));
        PARSER_DATA_BY_LEN(&iqInfo->localId, iqInfoFromDli, sizeof(uint8_t));
        PARSER_DATA_BY_LEN(&iqInfo->remoteId, iqInfoFromDli, sizeof(uint8_t));
    }
    NLSTK_LOG_DEBUG("Parser result: isMultiTone=%u, iqBitLen=%u", iqInfo->isMultiTone, iqInfo->iqBitLen);
    return iqInfo;
}