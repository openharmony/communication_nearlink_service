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
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sm_slink.h"
#include "sm_stub_test.h"
#include "cpfwk_log.h"

extern SDF_Map *g_slinkMap;

bool g_isRecvCbk = false;

SLE_Addr_S g_localAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };

SLE_Addr_S g_rmtAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };
int g_timerHandle = -1;

/*****************************************************************************************
                                    Help Functions
*****************************************************************************************/

uint32_t TEST_ScheduleTimerAddWithHandleStub(int *handle, SDF_TimerParam *param)
{
    g_timerHandle = *handle + 1;
    CP_LOG_INFO("[TEST] add stub timer succ, handle:%d", g_timerHandle);
    *handle += 1;
    (void)param;

    return 0;
}
void TEST_ScheduleTimerDelWithHandleStub(int handle)
{
    if (g_timerHandle == handle) {
        CP_LOG_INFO("[TEST] del stub timer succ, handle:%d", g_timerHandle);
        g_timerHandle -= 1;
    } else {
        CP_LOG_ERROR("[TEST] del stub timer error, can not find timer with handle:%d", handle);
    }
}


void AddSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddNegoSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddAuthSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddNoEntrySlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    slink->negoParams.authMethod = SM_AUTH_NO_ENTRY;
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddGNodeNumCmpSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    slink->negoParams.authMethod = SM_AUTH_NUMBER_COMPARE;
    slink->role = SM_G_NODE;
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddTNodeNumCmpSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    slink->negoParams.authMethod = SM_AUTH_NUMBER_COMPARE;
    slink->role = SM_T_NODE;
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddTNodeSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    slink->role = SM_T_NODE;
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void AddSteteFullSlinkToMap(SLE_Addr_S *addr)
{
    SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmSLinkCtor(addr);
    slink->curStateIndex = SM_STATE_FULL;
    SDF_MapMoveInsert(g_slinkMap, key, slink);
}

void Test_SmPairStartCbk(NLSTK_SmPairingStart_S *params)
{
    CP_LOG_INFO("[TEST] recv Test_SmPairStartCbk");
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_SmPairRemoveCbk(NLSTK_SmPairingRemove_S *params)
{
    CP_LOG_INFO("[TEST] recv Test_SmPairRemoveCbk");
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_SmPairRequestCbk(NLSTK_SmPairingRequest_S *params)
{
    CP_LOG_INFO("[TEST] recv Test_SmPairRequestCbk");
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_SmAuthCmpCbk(NLSTK_SmAuthComplete_S *params)
{
    CP_LOG_INFO("[TEST] recv Test_SmAuthCmpCbk");
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_SmEncCmpCbk(NLSTK_SmEncComplete_S *params)
{
    CP_LOG_INFO("[TEST] recv Test_SmEncCmpCbk");
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_SmImgMsgCbk(NLSTK_SmSendImgMsgCmpl_S *params)
{
    g_isRecvCbk = true;
    SDF_UNUSED(params);
}

void Test_ResetData()
{
    g_isRecvCbk = false;
}

void Test_CryptoRandNumGenerate(uint8_t *out, uint8_t len)
{
    for (int i = 0; i < len; i++) {
        out[i] = i;
    }
}

void Test_CryptoPubPriKeyPairGenerate(NLSTK_SmKeyPair_S *keyPair)
{
    uint8_t priKey[SM_PRIVATE_KEY_LEN] = {
        0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    uint8_t localPubKey[SM_PUBLIC_KEY_LEN] = {
        0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
        0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
        0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
        0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
    };

    uint8_t remotePubKey[SM_PUBLIC_KEY_LEN] = {
        0xc6, 0x0a, 0xe7, 0x16, 0xbd, 0x4a, 0x16, 0xa8, 0xdd, 0x7c, 0x8b, 0x22, 0x4d, 0x62, 0x82, 0x20,
        0xc1, 0xc6, 0x53, 0xec, 0x4e, 0x33, 0x87, 0xcb, 0x7c, 0xc8, 0x5f, 0xfb, 0x4f, 0x59, 0x05, 0x70,
        0x7a, 0x3c, 0xfc, 0x3c, 0xfc, 0xfc, 0x8d, 0xba, 0xc3, 0x4e, 0x8f, 0x8a, 0x0d, 0xcf, 0x40, 0xe9,
        0x8a, 0x2d, 0x40, 0xfb, 0xc1, 0xf0, 0x7b, 0x97, 0x1d, 0xc1, 0x27, 0xb1, 0xa1, 0xa6, 0x23, 0x11,
    };

    (void)memcpy_s(keyPair->priKey, SM_PRIVATE_KEY_LEN, priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair->localPubKey, SM_PUBLIC_KEY_LEN, localPubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair->remotePubKey, SM_PUBLIC_KEY_LEN, remotePubKey, SM_PUBLIC_KEY_LEN);
}

void Test_CryptoSecKeyGenerate(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen)
{
    SDF_UNUSED(keyPair);
    uint8_t dhkey[SM_DHKEY_LEN] = {
        0xdf, 0x21, 0x93, 0x16, 0x77, 0x5c, 0xa0, 0x96, 0x64, 0x4f, 0x40, 0x1d, 0x77, 0x39, 0x0a, 0x43,
        0xdb, 0xeb, 0xfa, 0x26, 0x27, 0xd8, 0xe7, 0x99, 0xc2, 0x06, 0x09, 0x5b, 0x4f, 0x81, 0xb9, 0xd7,
    };
    (void)memcpy_s(secKey, secKeyLen, dhkey, SM_DHKEY_LEN);
}

void Test_CryptoDerivedKeyGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen)
{
    uint8_t key[SM_OCTETS_16] = {0};
    (void)memcpy_s(output, outputLen, key, SM_OCTETS_16);
}
