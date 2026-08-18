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

#include <string.h>
#include <stdlib.h>
#include "securec.h"

#include "nlstk_init_api.h"
#include "sm_stub_common.h"
#include "nlstk_log.h"
#include "nlstk_cfgdb.h"
#include "sm_dft.h"
#include "sm_errcode.h"
#include "sm_img.h"

DLI_ExecuteCmdCbk g_sendFunc;
DLI_ExecuteCmdCbk g_recvFunc;
DLI_ExecuteCmdCbk g_encChangeFunc;
DLI_ExecuteCmdCbk g_encReqFunc;
DLI_ExecuteCmdCbk g_encReqReplyFunc;

static void SlinkSecMemFree(SmSLink_S *slink);

SLE_Addr_S g_gAddr = {.type = PUBLIC_ADDRESS, .addr = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
SLE_Addr_S g_tAddr = {.type = PUBLIC_ADDRESS, .addr = { 0x99, 0x88, 0x77, 0x66, 0x55, 0x44 } };


uint32_t NLSTK_CfgdbGetPublicAddressForGStub(SLE_Addr_S *addr)
{
    (void)memcpy_s(addr, sizeof(SLE_Addr_S), &g_gAddr, sizeof(SLE_Addr_S));
    return NLSTK_OK;
}

uint32_t NLSTK_CfgdbGetPublicAddressForTStub(SLE_Addr_S *addr)
{
    (void)memcpy_s(addr, sizeof(SLE_Addr_S), &g_tAddr, sizeof(SLE_Addr_S));
    return NLSTK_OK;
}

uint32_t CM_GetLogicLinkByLcidForGStub(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    (void)memcpy_s(&logicLink->addr, sizeof(SLE_Addr_S), &g_tAddr, sizeof(SLE_Addr_S));
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkByLcidForTStub(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    (void)memcpy_s(&logicLink->addr, sizeof(SLE_Addr_S), &g_gAddr, sizeof(SLE_Addr_S));
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkByAddrForGStub(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    logicLink->role = SM_G_NODE;
    logicLink->lcid = 0;
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkByAddrForTStub(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    logicLink->role = SM_T_NODE;
    logicLink->lcid = 0;
    return CM_SUCCESS;
}

uint32_t DLI_CmdCbkRegStub(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    g_sendFunc = (DLI_ExecuteCmdCbk)table[0].func;
    g_recvFunc = (DLI_ExecuteCmdCbk)table[1].func;
    g_encChangeFunc = (DLI_ExecuteCmdCbk)table[2].func;
    g_encReqFunc = (DLI_ExecuteCmdCbk)table[3].func;
    g_encReqReplyFunc = (DLI_ExecuteCmdCbk)table[4].func;
    return DLI_SUCCESS;
}

void SendBuffer(DLI_ControllerDataEvt *buff)
{
    DLI_ExecuteCmdRetParam param = {0};
    param.eventParameter = (void *)buff;
    NLSTK_LOG_INFO("[SM] data: %s.", SDF_GET_UINT8_STR(buff->data, buff->len));
    g_recvFunc(NULL, 0, &param);
}

void Crypto_RandNumGenerateStub(uint8_t *out, uint8_t len)
{
    for (int i = 0; i < len; i++) {
        out[i] = i;
    }
}

void Crypto_PubPriKeyPairGenerateStub(NLSTK_SmKeyPair_S *keyPair)
{
    uint8_t priKey[SM_PRIVATE_KEY_LEN] = {
        0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    uint8_t pubKey[SM_PUBLIC_KEY_LEN] = {
        0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
        0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
        0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
        0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
    };

    (void)memcpy_s(keyPair->priKey, SM_PRIVATE_KEY_LEN, priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair->localPubKey, SM_PUBLIC_KEY_LEN, pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair->remotePubKey, SM_PUBLIC_KEY_LEN, pubKey, SM_PUBLIC_KEY_LEN);
}

void Crypto_SecKeyGenerateStub(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen)
{
    SDF_UNUSED(keyPair);
    uint8_t dhkey[SM_DHKEY_LEN] = {
        0x06, 0x93, 0x76, 0xCD, 0x64, 0xE5, 0xEF, 0x2D, 0x25, 0xB5, 0x0F, 0xEE, 0xE3, 0xD5, 0xDC, 0xA0,
        0xDC, 0x46, 0x39, 0x71, 0x71, 0xB7, 0xB3, 0x6C, 0x0E, 0xC9, 0x60, 0x11, 0x39, 0x4B, 0x9E, 0x7A,
    };
    (void)memcpy_s(secKey, secKeyLen, dhkey, SM_DHKEY_LEN);
}

void Crypto_DerivedKeyGenerateStub(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen)
{
    uint8_t key[SM_OCTETS_16] = {0};
    (void)memcpy_s(output, outputLen, key, SM_OCTETS_16);
}

uint32_t CfgdbReadAlgoCaps(CfgdbAlgoCaps_S *caps)
{
    caps->encryptAlgo = 2;
    caps->intgProtectAlgo = 2;
    caps->keyDerivAlgo = 2;
    caps->keyNegoAlgo = 2;
    return NLSTK_OK;
}

/*****************************************************************************************
                                    Stub SmSlink
*****************************************************************************************/

static void SlinkSecMemFree(SmSLink_S *slink)
{
    NLSTK_LOG_INFO("[SM] Start to free security memory.");
    (void)memset_s(slink->dhKey, SM_DHKEY_LEN, 0, SM_DHKEY_LEN);
    (void)memset_s(slink->priKey, SM_PRIVATE_KEY_LEN, 0, SM_PRIVATE_KEY_LEN);
    (void)memset_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, 0, SM_PUBLIC_KEY_LEN);
    (void)memset_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, 0, SM_PUBLIC_KEY_LEN);
    slink->gNode.passCode = 0;
    slink->tNode.passCode = 0;
    (void)memset_s(slink->psk, SM_PSK_SEC_KEY_LEN, 0, SM_PSK_SEC_KEY_LEN);
    (void)memset_s(slink->linkKey, SM_LINK_KEY_LEN, 0, SM_LINK_KEY_LEN);
    SDF_MemFree(slink);
}

void SmSLinkDtor(void *slinkIn)
{
    SmSLink_S *slink = (SmSLink_S *)slinkIn;
    if (slink != NULL) {
        SmSLinkWaitDelTimer(slink);
        SmSLinkWaitDelUapiTimer(slink);
        SmStateMachineDtor(slink->stm);
        SlinkSecMemFree(slink);
    }
}

SmSLink_S *SmSLinkCtor(const SLE_Addr_S *rmtAddr)
{
    SmSLink_S *slink = SDF_MemZalloc(sizeof(SmSLink_S)); // 在slink dtor里释放
    if (slink == NULL) {
        goto FAIL_LABEL;
    }

    SLE_Addr_S localAddr = {0};
    uint32_t ret = NLSTK_CfgdbGetPublicAddress(&localAddr);
    if (ret != NLSTK_OK) {
        goto FAIL_LABEL;
    }
    NLSTK_LOG_INFO("[SM] Local addr is %s", GET_ENC_ADDR(&localAddr));

    *slink = (SmSLink_S) {
        .rmtAddr = *rmtAddr,
        .localAddr = localAddr,
        .lcid = CM_INVALID_LCID,
        .role = SM_G_NODE,
        .stm = NULL,
        .dispatcher = NULL,
        .expectOpCode = 0,
        .gNode.recvFlag = false,
        .tNode.recvFlag = false,
        .timerHandle = TIMER_NO_USED_VALUE,
        .uapiTimerHandle = TIMER_NO_USED_VALUE,
    };

    SmStateMachine_S *stm = SmStateMachineCtor(slink);
    if (stm == NULL) {
        goto FAIL_LABEL;
    }

    slink->stm = stm;
    return slink;

FAIL_LABEL:
    SmSLinkDtor((void *)slink);
    return NULL;
}

bool SmSLinkBindLogicLink(SmSLink_S *slink)
{
    CM_LogicLink_S logicLink = {0};
    if (CM_GetLogicLinkByAddr(&slink->rmtAddr, &logicLink) != CM_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Get logic link by address fail.");
        return false;
    }
    slink->lcid = logicLink.lcid;
    slink->role = logicLink.role;
    NLSTK_LOG_INFO("[SM] Bind slink %s with logic link lcid: %u", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    return true;
}

void SmSLinkWaitDelTimer(SmSLink_S *slink)
{
    CP_TimerDel(slink->timerHandle);
    slink->timerHandle = TIMER_NO_USED_VALUE;
}

void SmSLinkWaitDelUapiTimer(SmSLink_S *slink)
{
    CP_TimerDel(slink->uapiTimerHandle);
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
}

void WaitTimeoutCbk(void *slinkIn)
{
    NLSTK_LOG_ERROR("[SM] Timeout when wait expect opcode or input.");
    SmSLink_S *slink = (SmSLink_S *)slinkIn;
    SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_TIMEOUT_ERR);
    STM_MFUNC(slink->stm, ProcessMessage, (Message) {
        .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
}

void SmSLinkAuthCbk(SmSLink_S *slink, uint8_t status)
{
    NLSTK_SmAuthComplete_S param = {0};
    param.addr = slink->rmtAddr;
    param.lcid = slink->lcid;
    param.authStatus = status;
    if (status == SM_PAIR_OK) {
        (void)memcpy_s(param.linkKey, SM_LINK_KEY_LEN, slink->linkKey, SM_LINK_KEY_LEN);
        param.cryptoAlgo = slink->negoParams.codeAlgoCap[SM_ENC_ALGO_ABILITY];
        param.keyDerivAlgo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY];
        param.intgChkInd = slink->encIntgCheck;
        SmGenerateImgSecuConfig(param.keyDerivAlgo, param.groupKey, &param.giv);
    }
    SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &param);
    (void)memset_s(&param, sizeof(NLSTK_SmAuthComplete_S), 0x00, sizeof(NLSTK_SmAuthComplete_S));
}

void SmSLinkEncpCbk(SmSLink_S *slink, uint8_t status)
{
    NLSTK_SmEncComplete_S param = {0};
    param.lcid = slink->lcid;
    param.addr = slink->rmtAddr;
    param.encStatus = status;
    SmExternalCbks(SM_CBK_EVENT_ENCRYPT_COMPLETE, &param);
}

void SmSLinkRequestCbk(SmSLink_S *slink, uint8_t type, SmAuthUserCode_S code)
{
    NLSTK_SmPairingRequest_S param = {0};
    param.lcid = slink->lcid;
    param.addr = slink->rmtAddr;
    param.requestType = type;
    (void)memcpy_s(param.sixDigits, SM_OCTETS_7, code.code, SM_OCTETS_7);
    SmExternalCbks(SM_CBK_EVENT_PAIRING_REQUEST, &param);
}