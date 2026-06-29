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
#include <string.h>
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"

#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "sm_algos.h"
#include "sm_dft.h"
#include "sm_dhkey.h"
#include "sm_noentry.h"

static SmAuthDhkeyMsg_S g_noEntryTNodeRecvDHKey = {0};
static void NoEntrySendTNodeCfm(SmSLink_S *slink);
static void NoEntryRecvTNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void NoEntrySendRandNumRa(SmSLink_S *slink);
static void NoEntryRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void NoEntrySendRandNumRb(SmSLink_S *slink);
static void NoEntryRecvRandNumRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);

void SmNoEntryAuthStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_METHOD, SM_DFT_NOENTRY);
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][NOENTRY] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM, SM_RECV_TIMEOUT_TIME);
    } else {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_METHOD, SM_DFT_NOENTRY);
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][NOENTRY] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmConfirmNum_S confirm = {0};
        if (!SmGenConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink, &confirm)) {
            SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_CFM_ERR);
            NLSTK_LOG_ERROR("[SM][NOENTRY] Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_CFM_TIME);
        NoEntrySendTNodeCfm(slink);
    }
}

void SmNoEntryAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_T_NODE_CFM: {                /* G节点，收到T发送的确认码消息 */
            NoEntryRecvTNodeCfm(slink, pkg, size);
            break;
        }
        case SM_AUTH_RAND_NUM_RA: {               /* T节点，收到G发送的Ra消息 */
            NoEntryRecvRandNumRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_RAND_NUM_RB: {               /* G节点，收到T发送的Rb消息 */
            NoEntryRecvRandNumRb(slink, pkg, size);
            break;
        }
        case SM_AUTH_G_NODE_DHKEY: {              /* T节点，收到G发送的DHKEY验证码消息 */
            slink->tNode.recvFlag = true;
            if (slink->uapiTimerHandle == TIMER_NO_USED_VALUE) {
                slink->tNode.recvFlag = false;
                SmRecvGNodeDhKey(slink, pkg, size);
            } else {
                NLSTK_CHECK_RETURN_VOID(size == sizeof(SmAuthDhkeyMsg_S), "[SM] T node DhKey recv pkg size failure.");
                (void)memcpy_s(g_noEntryTNodeRecvDHKey.authData, SM_DHKEY_AUTHCODE_LEN, pkg, SM_DHKEY_AUTHCODE_LEN);
            }
            break;
        }
        case SM_AUTH_T_NODE_DHKEY: {              /* G节点，收到T发送的DHKEY验证码消息 */
            SmRecvTNodeDhKey(slink, pkg, size);
            break;
        }
        default:
            NLSTK_LOG_ERROR("[SM][NOENTRY] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

/*****************************************************************************************
                            Authentication Message Procedure
*****************************************************************************************/

static void NoEntrySendTNodeCfm(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Send auth T node confirm.");
    SmAuthTCfmMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_CONFIRM_NUMBER_LEN, slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_SEND_CFM_TIME);
    bool ret = SmSendMessage(slink, SM_AUTH_T_NODE_CFM, (const uint8_t *)&msg, sizeof(SmAuthTCfmMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_RAND_NUM_RA, SM_RECV_TIMEOUT_TIME);
}

static void NoEntryRecvTNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    (void)size;
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_RECV_CFM_TIME);
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Recv auth T node confirm.");
    (void)pkg;
    /* 无操作，直接发送Ra */
    NoEntrySendRandNumRa(slink);
}

static void NoEntrySendRandNumRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Send auth random num ra.");
    SmAuthRandomAMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_SEND_RA_TIME);
    bool ret = SmSendMessage(slink, SM_AUTH_RAND_NUM_RA, (const uint8_t *)&msg, sizeof(SmAuthRandomAMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_RAND_NUM_RB, SM_RECV_TIMEOUT_TIME);
}

static void NoEntryRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_RECV_RA_TIME);
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Recv auth random num ra.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthRandomAMsg_S), "[SM][NOENTRY] Recv random ra pkg size failure.");
    SmAuthRandomAMsg_S *msg = (SmAuthRandomAMsg_S *)pkg;
    /* T节点保存Ra */
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->authData, SM_RANDOM_NUMBER_R_LEN);
    /* 发送Rb */
    NoEntrySendRandNumRb(slink);
    SmAuthUserCode_S code = {0};
    SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
    SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
}

static void NoEntrySendRandNumRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Send auth random num rb.");
    SmAuthRandomBMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_SEND_RB_TIME);
    bool ret = SmSendMessage(slink, SM_AUTH_RAND_NUM_RB, (const uint8_t *)&msg, sizeof(SmAuthRandomBMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

static void NoEntryRecvRandNumRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_RECV_RB_TIME);
    NLSTK_LOG_DEBUG("[SM][NOENTRY] Recv auth random num rb.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthRandomBMsg_S), "[SM][NOENTRY] Recv random rb pkg size failure.");
    SmAuthRandomBMsg_S *msg = (SmAuthRandomBMsg_S *)pkg;
    /* G节点保存Rb */
    (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->authData, SM_RANDOM_NUMBER_R_LEN);
    SmAuthUserCode_S code = {0};
    SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
    SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
}

void SmNoEntryContinueNoentry(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        /* 计算DH Key */
        NLSTK_SmKeyPair_S keyPair;
        keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
            NLSTK_LOG_ERROR("[SM] G node: dhkey generation failure.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        /* 计算link Key */
        if (!SmGenLinkKey(slink)) {
            SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
            NLSTK_LOG_ERROR("[SM] G node: Link key generation failure.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_GEN_KEY_TIME);
        /* 发送DHKey */
        SmSendGNodeDhKey(slink);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
    } else {
        /* 计算DH Key */
        NLSTK_SmKeyPair_S keyPair;
        keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
        (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
        (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
        (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
        if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
            NLSTK_LOG_ERROR("[SM] T node: dhkey generation failure.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        /* 计算link Key */
        if (!SmGenLinkKey(slink)) {
            SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
            NLSTK_LOG_ERROR("[SM] T node: Link key generation failure.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_KEY_TIME);
        if (slink->tNode.recvFlag) {
            slink->tNode.recvFlag = false;
            SmRecvGNodeDhKey(slink, g_noEntryTNodeRecvDHKey.authData, sizeof(SmAuthDhkeyMsg_S));
            (void)memset_s(g_noEntryTNodeRecvDHKey.authData, SM_OCTETS_16, 0, SM_OCTETS_16);
        }
    }
}