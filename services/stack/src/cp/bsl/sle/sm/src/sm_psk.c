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
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"

#include "nlstk_sm_api.h"
#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_slink.h"
#include "sm_algos.h"
#include "sm_dhkey.h"
#include "sm_errcode.h"
#include "sm_psk.h"

static void PskSendRandNumRa(SmSLink_S *slink);
static void PskRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void PskSendRandNumRb(SmSLink_S *slink);
static void PskRecvRandNumRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void PskSendGNodeCfm(SmSLink_S *slink);
static void PskRecvGNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void PskSendTNodeCfm(SmSLink_S *slink);
static void PskRecvTNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size);

static bool GenPskConfirmNum(uint8_t *psk, SmSLink_S *slink, SmConfirmNum_S *confirm,
                             uint8_t *randomNum, uint8_t randLen);
static bool CheckPskConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *psk, uint8_t *randomNum, uint8_t randLen);

void SmPskAuthStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PSK] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        PskSendRandNumRa(slink);
    } else {
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PSK] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_RAND_NUM_RA, SM_RECV_TIMEOUT_TIME);
    }
}

void SmPskAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_RAND_NUM_RA: {            /* T节点，收到G发送的Ra消息 */
            PskRecvRandNumRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_RAND_NUM_RB: {            /* G节点，收到T发送的Rb消息 */
            PskRecvRandNumRb(slink, pkg, size);
            break;
        }
        case SM_AUTH_G_NODE_CFM: {             /* T节点，收到G发送的确认码消息 */
            PskRecvGNodeCfm(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_CFM: {             /* G节点，收到T发送的确认码消息 */
            PskRecvTNodeCfm(slink, pkg, size);
            break;
        }
        case SM_AUTH_G_NODE_DHKEY: {           /* T节点，收到G发送的DHKEY验证码消息 */
            SmRecvGNodeDhKey(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_DHKEY: {           /* G节点，收到T发送的DHKEY验证码消息 */
            SmRecvTNodeDhKey(slink, pkg, size);
            break;
        }
        default:
            NLSTK_LOG_ERROR("[SM][PSK] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

static void PskSendRandNumRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Send auth random num ra.");
    SmAuthRandomAMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_RAND_NUM_RA, (const uint8_t *)&msg, sizeof(SmAuthRandomAMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_RAND_NUM_RB, SM_RECV_TIMEOUT_TIME);
}

static void PskRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Recv auth random num ra.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthRandomAMsg_S), "[SM][PSK] Auth random ra recv pkg size failure.");
    SmAuthRandomAMsg_S *msg = (SmAuthRandomAMsg_S *)pkg;
    /* T节点保存Ra */
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->authData, SM_RANDOM_NUMBER_R_LEN);
    /* 基于PSK计算T节点确认码 */
    SmConfirmNum_S confirm = {0};
    if (!GenPskConfirmNum(slink->psk, slink, &confirm, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        NLSTK_LOG_ERROR("[SM][PSK] T Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    /* 发送Rb */
    PskSendRandNumRb(slink);
}

static void PskSendRandNumRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Send auth random num rb.");
    SmAuthRandomBMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_RAND_NUM_RB, (const uint8_t *)&msg, sizeof(SmAuthRandomBMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_CFM, SM_RECV_TIMEOUT_TIME);
}

static void PskRecvRandNumRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Recv auth random num rb.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthRandomBMsg_S), "[SM][PSK] Auth random rb recv pkg size failure.");
    SmAuthRandomBMsg_S *msg = (SmAuthRandomBMsg_S *)pkg;
    /* G节点保存Rb */
    (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->authData, SM_RANDOM_NUMBER_R_LEN);
    /* 基于PSK计算G节点确认码 */
    SmConfirmNum_S confirm = {0};
    if (!GenPskConfirmNum(slink->psk, slink, &confirm, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        NLSTK_LOG_ERROR("[SM][PSK] G Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    /* 发送G节点确认码 */
    PskSendGNodeCfm(slink);
}

static void PskSendGNodeCfm(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Send auth G node confirm.");
    NLSTK_LOG_DEBUG("[SM][PSK] Send G confirm: %s", SDF_GET_ENC_STR(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthGCfmMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_CONFIRM_NUMBER_LEN, slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_G_NODE_CFM, (const uint8_t *)&msg, sizeof(SmAuthGCfmMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM, SM_RECV_TIMEOUT_TIME);
}

static void PskRecvGNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Recv auth G node confirm.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthGCfmMsg_S), "[SM][PSK] Recv G node confirm pkg size failure.");
    SmAuthGCfmMsg_S *msg = (SmAuthGCfmMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][PSK] Recv G confirm: %s", SDF_GET_ENC_STR(msg->authData, SM_CONFIRM_NUMBER_LEN));
    /* T节点基于PSK验证G节点确认码 */
    uint8_t psk[SM_OCTETS_16] = {0};
    (void)memcpy_s(psk, SM_OCTETS_16, slink->psk, SM_OCTETS_16);
    if (!CheckPskConfirm(slink, msg->authData, psk, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][PSK] T node: G Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PSK });
        return;
    }
    /* 验证一致则保存 */
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->authData, SM_CONFIRM_NUMBER_LEN);
    /* 计算DH Key */
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][PSK] T node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][PSK] T node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 发送T节点确认码 */
    PskSendTNodeCfm(slink);
}

static void PskSendTNodeCfm(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Send auth T node confirm.");
    NLSTK_LOG_DEBUG("[SM][PSK] Send T confirm: %s", SDF_GET_ENC_STR(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthTCfmMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_CONFIRM_NUMBER_LEN, slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_T_NODE_CFM, (const uint8_t *)&msg, sizeof(SmAuthTCfmMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

static void PskRecvTNodeCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Recv auth T node confirm.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthTCfmMsg_S), "[SM][PSK] Recv T node confirm pkg size failure.");
    SmAuthTCfmMsg_S *msg = (SmAuthTCfmMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][PSK] Recv T confirm: %s", SDF_GET_ENC_STR(msg->authData, SM_CONFIRM_NUMBER_LEN));
    /* G节点基于PSK验证T节点确认码 */
    uint8_t psk[SM_OCTETS_16] = {0};
    (void)memcpy_s(psk, SM_OCTETS_16, slink->psk, SM_OCTETS_16);
    if (!CheckPskConfirm(slink, msg->authData, psk, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][PSK] G node: T Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PSK });
        return;
    }
    /* 验证一致则保存 */
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->authData, SM_CONFIRM_NUMBER_LEN);
    /* 计算DH Key */
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][PSK] G node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][PSK] G node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmSendGNodeDhKey(slink);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

static bool GenPskConfirmNum(uint8_t *psk, SmSLink_S *slink, SmConfirmNum_S *confirm,
                             uint8_t *randomNum, uint8_t randLen)
{
    NLSTK_CHECK_RETURN(randLen == SM_RANDOM_NUMBER_R_LEN, false, "[SM][PSK] Rand num length error.");
    uint8_t buff[SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + SM_RANDOM_NUMBER_R_LEN];
    /* G节点公钥 || T节点公钥 || G/T随机数 */
    if ((memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN, randLen, randomNum, randLen) != EOK)) {
        return false;
    }
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, psk, SM_OCTETS_16);
    input.buff = buff;
    input.buffSize = SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + randLen;
    if (!SmCmacGenerate(&input, confirm->confirm, SM_CONFIRM_NUMBER_LEN)) {
        (void)memset_s(input.key, SM_OCTETS_16, 0, SM_OCTETS_16);
        NLSTK_LOG_ERROR("[SM][PSK] Generate psk confirm num error.");
        return false;
    }
    (void)memset_s(input.key, SM_OCTETS_16, 0, SM_OCTETS_16);
    return true;
}

static bool CheckPskConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *psk, uint8_t *randomNum, uint8_t randLen)
{
    NLSTK_LOG_DEBUG("[SM][PSK] Start to check confirm number.");
    SmConfirmNum_S confirm = {0};
    if (!GenPskConfirmNum(psk, slink, &confirm, randomNum, randLen)) {
        NLSTK_LOG_ERROR("[SM][PSK] Confirm number generate failed.");
        return false;
    }
    if (memcmp(confirmRecv, confirm.confirm, SM_CONFIRM_NUMBER_LEN) != 0) {
        NLSTK_LOG_ERROR("[SM][PSK] Confirm code is inconsistent.");
        return false;
    }
    return true;
}