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
#ifndef STACK_DTAP_STUB_H
#define STACK_DTAP_STUB_H

#include <stdbool.h>
#include "dtap.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t TEST_DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb);
uint32_t TEST_DTAP_UnregisterDataRecvCb(uint8_t tcid);
uint32_t TEST_DTAP_DataSend(DTAP_Data_S *data);

/**
 * @brief 模拟NLSTK链路收到消息的情况。
 *
 * @param tcid 传输管道类型
 * @param info 接收消息的链路信息
 * @param buff 接收到的消息信息
 * @return None
 * eg：
 * NLSTK代码中调用：
 *      DTAP_RegisterDataRecvCb(TCID_SLE_SMTC, SSAP_Recv);
 * 在DT测试用例中，可以通过以下方式来模拟：
 *      STUB_DTAP_Init();
 *      DTAP_Data_Info_S info = {xxx};
 *      SDF_Buff_S buff = {xxx};
 *      TEST_DTAP_RecData(TCID_SLE_SMTC, &info, &buff);
 * 当DT代码中调用了 TEST_DTAP_RecData 函数之后，就会触发 SSAP_Recv 函数的调用
 * 这里需要注意 DTAP_Data_Info_S 中的内容；
 */
void TEST_DTAP_RecData(uint8_t tcid, DTAP_Data_Info_S *info, SDF_Buff_S *buff);

void TEST_DTAP_RecDataWithPkt(uint8_t lcid, uint8_t tcid, uint8_t *buf, uint32_t size);

uint32_t TEST_DTAP_DataSend(DTAP_Data_S *data);

bool TEST_DTAP_CompareLastPkt(uint8_t *buf, uint32_t size);

void TEST_DTAP_SSAP_RevcInitPkt(uint16_t lcid);

#ifdef __cplusplus
}
#endif
#endif