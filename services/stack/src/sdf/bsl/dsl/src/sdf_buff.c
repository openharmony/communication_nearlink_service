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

#include "sdf_buff.h"

#include <stddef.h>

#include "sdf_mem.h"
#include "securec.h"

SDF_Buff_S *SDF_BuffNew(uint32_t size)
{
    SDF_Buff_S *buff = NULL;

    buff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + size);
    if (buff == NULL) {
        return NULL;
    }

    buff->buffLen = size;
    return buff;
}

SDF_Buff_S *SDF_BuffNewWithReserve(uint32_t size)
{
    SDF_Buff_S *buff = NULL;

    buff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + SDF_BUFF_MAX_RESERVED_SIZE + size);
    if (buff == NULL) {
        return NULL;
    }

    buff->dataOff = SDF_BUFF_MAX_HEADROOM_SIZE;
    buff->buffLen = SDF_BUFF_MAX_RESERVED_SIZE + size;
    return buff;
}

SDF_Buff_S *SDF_BuffNewWithExtraReserve(uint32_t size, uint16_t extraSize)
{
    SDF_Buff_S *buff = NULL;

    buff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + SDF_BUFF_MAX_RESERVED_SIZE + size + extraSize);
    if (buff == NULL) {
        return NULL;
    }

    buff->dataOff = SDF_BUFF_MAX_HEADROOM_SIZE + extraSize;
    buff->buffLen = SDF_BUFF_MAX_RESERVED_SIZE + size + extraSize;
    return buff;
}

void SDF_BuffFree(SDF_Buff_S *buff)
{
    if (buff == NULL) {
        return;
    }
    SDF_MemFree(buff);
}

uint8_t *SDF_BuffAppend(SDF_Buff_S *buff, uint32_t size)
{
    uint8_t *tail = NULL;

    if (buff == NULL || size > SDF_BuffTailRoom(buff)) {
        return NULL;
    }

    tail = (uint8_t *)buff->buff + buff->dataOff + buff->dataLen;
    buff->dataLen += size;
    return tail;
}

uint8_t *SDF_BuffPrepend(SDF_Buff_S *buff, uint32_t size)
{
    if (buff == NULL || size > SDF_BuffHeadRoom(buff)) {
        return NULL;
    }

    buff->dataOff -= size;
    buff->dataLen += size;

    return (uint8_t *)buff->buff + buff->dataOff;
}

uint8_t *SDF_BuffTrimPrefix(SDF_Buff_S *buff, uint32_t size)
{
    if (buff == NULL || size > buff->dataLen) {
        return NULL;
    }

    buff->dataLen -= size;
    buff->dataOff += size;
    return (uint8_t *)buff->buff + buff->dataOff;
}

int32_t SDF_BuffTrimSuffix(SDF_Buff_S *buff, uint32_t size)
{
    if (buff == NULL || size > buff->dataLen) {
        return -1;
    }

    buff->dataLen -= size;
    return 0;
}

SDF_Buff_S *SDF_BuffCopy(SDF_Buff_S *buff)
{
    if (buff == NULL || SDF_BuffLenGet(buff) > UINT64_MAX - sizeof(SDF_Buff_S)) {
        return NULL;
    }
    SDF_Buff_S *newBuff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + SDF_BuffLenGet(buff));
    if (newBuff == NULL) {
        return NULL;
    }
    (void)memcpy_s(newBuff, sizeof(SDF_Buff_S) + SDF_BuffLenGet(buff),
        buff, sizeof(SDF_Buff_S) + SDF_BuffLenGet(buff));
    return newBuff;
}