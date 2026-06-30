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

#include "sdf_addr.h"
#include "nlstk_sm_api.h"

#ifndef SM_STUB_TEST_H
#define SM_STUB_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

extern SLE_Addr_S g_rmtAddr;
extern SLE_Addr_S g_localAddr;
extern int g_timerHandle;
extern bool g_isRecvCbk;

uint32_t TEST_ScheduleTimerAddWithHandleStub(int *handle, SDF_TimerParam *param);
void TEST_ScheduleTimerDelWithHandleStub(int handle);

void AddSlinkToMap(SLE_Addr_S *addr);
void AddNegoSlinkToMap(SLE_Addr_S *addr);
void AddAuthSlinkToMap(SLE_Addr_S *addr);
void AddNoEntrySlinkToMap(SLE_Addr_S *addr);
void AddGNodeNumCmpSlinkToMap(SLE_Addr_S *addr);
void AddTNodeNumCmpSlinkToMap(SLE_Addr_S *addr);
void AddTNodeSlinkToMap(SLE_Addr_S *addr);
void AddSteteFullSlinkToMap(SLE_Addr_S *addr);

void Test_SmPairStartCbk(NLSTK_SmPairingStart_S *params);
void Test_SmPairRemoveCbk(NLSTK_SmPairingRemove_S *params);
void Test_SmPairRequestCbk(NLSTK_SmPairingRequest_S *params);
void Test_SmAuthCmpCbk(NLSTK_SmAuthComplete_S *params);
void Test_SmEncCmpCbk(NLSTK_SmEncComplete_S *params);
void Test_SmImgMsgCbk(NLSTK_SmSendImgMsgCmpl_S *params);

void Test_ResetData();

void Test_CryptoRandNumGenerate(uint8_t *out, uint8_t len);
void Test_CryptoPubPriKeyPairGenerate(NLSTK_SmKeyPair_S *keyPair);
void Test_CryptoSecKeyGenerate(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen);
void Test_CryptoDerivedKeyGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen);

#ifdef __cplusplus
}
#endif

#endif