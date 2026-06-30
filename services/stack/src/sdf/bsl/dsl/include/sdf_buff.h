/**
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

/****************************************************************************
 *
 * this file contains data buffer definition and relatd APIs.
 *
 ***************************************************************************/

#ifndef SDF_BUFF_H
#define SDF_BUFF_H

#include <stdint.h>
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_BUFF_MAX_HEADROOM_SIZE 14U  // DTAP最大帧头9字节（暂不考虑TLV）+ DLI头5字节
#define SDF_BUFF_MAX_TAILROOM_SIZE 2U   // DTAP crc16需要2个字节
#define SDF_BUFF_MAX_RESERVED_SIZE (SDF_BUFF_MAX_HEADROOM_SIZE + SDF_BUFF_MAX_TAILROOM_SIZE)

#define SDF_DataLenGet(buff) ((buff)->dataLen)
#define SDF_BuffLenGet(buff) ((buff)->buffLen)
#define SDF_BuffHeadRoom(buff) ((buff)->dataOff)
#define SDF_DataOffset(_buff) ((uint8_t *)(_buff)->buff + (_buff)->dataOff)
#define SDF_BuffTailRoom(_buff) \
    (SDF_BuffLenGet(_buff) - SDF_BuffHeadRoom(_buff) - SDF_DataLenGet(_buff))

/**
 * @brief 创建指定长度数据缓冲区
 * @param[IN] size 缓冲区的长度
 * @return 创建的SDF_Buff_S结构体指针，如果创建失败则返回NULL
 */
SDF_Buff_S *SDF_BuffNew(uint32_t size);

/**
 * @brief 创建指定长度数据缓冲区，并预留16字节的空间，缓冲区起始14字节(headroom)预留给DTAP/DLI header，最后2字节(tailroom)预留给crc16
 * @param[IN] size 缓冲区的长度
 * @return 创建的SDF_Buff_S结构体指针，如果创建失败则返回NULL
 */
SDF_Buff_S *SDF_BuffNewWithReserve(uint32_t size);

/**
 * @brief 创建一个新的SDF_Buff_S对象，并预留额外的空间
 * @param[IN] size 需要创建的SDF_Buff_S对象的大小
 * @param[IN] extraSize 需要额外预留的空间大小
 * @return 返回创建的SDF_Buff_S对象指针，如果创建失败则返回NULL
 */
SDF_Buff_S *SDF_BuffNewWithExtraReserve(uint32_t size, uint16_t extraSize);

/**
 * @brief 释放数据缓冲区
 * @param[IN] buff 待释放的缓冲区指针
 */
void SDF_BuffFree(SDF_Buff_S *buff);

/**
 * @brief 在数据缓冲区data后面追加指定长度空间，不会新申请内存空间
 * @param[IN] buff 缓冲区指针
 * @param[IN] size 要追加的数据长度
 * @return 追加成功返回尾部追加数据的起始地址，否则，返回NULL
 */
uint8_t *SDF_BuffAppend(SDF_Buff_S *buff, uint32_t size);

/**
 * @brief 在数据缓冲区data前面追加指定长度空间，不会新申请内存空间
 * @param[IN] buff 缓冲区指针
 * @param[IN] size 要追加的数据长度
 * @return 追加成功返回头部追加数据的起始地址，否则，返回NULL
 */
uint8_t *SDF_BuffPrepend(SDF_Buff_S *buff, uint32_t size);

/**
 * @brief 移除数据缓冲区data中最前面指定长度空间
 * @param[IN] buff 缓冲区指针
 * @param[IN] size 需要移除的空间长度
 * @return 移除成功返回调整后的缓冲区指针，失败返回NULL
 */
uint8_t *SDF_BuffTrimPrefix(SDF_Buff_S *buff, uint32_t size);

/**
 * @brief 移除数据缓冲区data中最后面指定长度空间
 * @param[IN] buff 缓冲区指针
 * @param[IN] size 需要移除的空间长度
 * @return 移除成功返回0，失败返回-1
 */
int32_t SDF_BuffTrimSuffix(SDF_Buff_S *buff, uint32_t size);

/**
 * @brief 复制数据缓冲区
 * @param[IN] buff 缓冲区指针
 * @return 复制的新缓冲区指针，如果复制失败则返回NULL
 */
SDF_Buff_S *SDF_BuffCopy(SDF_Buff_S *buff);

#ifdef __cplusplus
}
#endif
#endif