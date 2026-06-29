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

#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_slink.h"
#include "sm_algos.h"
#include "sm_dhkey.h"
#include "sm_errcode.h"
#include "sm_password.h"

static void PassWordSendGNodeCfmWithRa(SmSLink_S *slink);
static void PassWordRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void PassWordSendTNodeCfmWithRb(SmSLink_S *slink);
static void PassWordRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);

static void GenMixPassWord(SmSLink_S *slink, NLSTK_SmPassWord_S *passWord, uint8_t *mixWord, uint8_t wordLen);
static bool GenPassWordConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink,
                                  uint8_t *mixWord, uint8_t wordLen, SmConfirmNum_S *confirm);
static bool CheckPassWordConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen,
                                 uint8_t *mixWord, uint8_t wordLen);

void SmPassWordAuthStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PASSWORD] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    } else {
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PASSWORD] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_CFM_WITH_RA, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    }
}

void SmPassWordAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_G_NODE_CFM_WITH_RA: {     /* T节点，收到G发送的携带Ra的确认码消息 */
            PassWordRecvGNodeCfmWithRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_CFM_WITH_RB: {     /* G节点，收到T发送的携带Rb的确认码消息 */
            PassWordRecvTNodeCfmWithRb(slink, pkg, size);
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
            NLSTK_LOG_ERROR("[SM][PASSWORD] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

/*****************************************************************************************
                            Authentication Message Procedure
*****************************************************************************************/

void SmPassWordSetGPassWord(SmSLink_S *slink, NLSTK_SmPassWord_S *passWord)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Set G Password.");
    /* 生成混淆后的口令码 */
    GenMixPassWord(slink, passWord, slink->gNode.mixWord, SM_MIX_PASSWORD_LEN);
    /* 生成G节点确认码 */
    SmConfirmNum_S confirm = {0};
    if (!GenPassWordConfirmNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                               slink->gNode.mixWord, SM_MIX_PASSWORD_LEN, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] G Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    PassWordSendGNodeCfmWithRa(slink);
}

void SmPassWordSetTPassWord(SmSLink_S *slink, NLSTK_SmPassWord_S *passWord)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Set T Password.");
    /* 生成混淆后的口令码 */
    GenMixPassWord(slink, passWord, slink->tNode.mixWord, SM_MIX_PASSWORD_LEN);
    /* 生成T节点确认码 */
    SmConfirmNum_S confirm = {0};
    if (!GenPassWordConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                               slink->tNode.mixWord, SM_MIX_PASSWORD_LEN, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] T Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    if (slink->tNode.recvFlag) {
        /* 假如已经收到来自G的确认码消息，将flag置回false，验证G确认码后再发送携带Rb的确认码消息 */
        slink->tNode.recvFlag = false;
        if (!CheckPassWordConfirm(slink, slink->gNode.confirm, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN,
                                  slink->tNode.mixWord, SM_MIX_PASSWORD_LEN)) {
            /* 验证不一致则中止配对 */
            NLSTK_LOG_ERROR("[SM][PASSWORD] G Confirm code check error.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PASSWORD_ENTRY });
            return;
        }
        PassWordSendTNodeCfmWithRb(slink);
    }
}

static void PassWordSendGNodeCfmWithRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Send auth G node confirm with Ra.");
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Send G confirm: %s", SDF_GET_ENC_STR(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthGCfmWithRaMsg_S msg = {0};
    (void)memcpy_s(msg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_G_NODE_CFM_WITH_RA, (const uint8_t *)&msg, sizeof(SmAuthGCfmWithRaMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM_WITH_RB, SM_RECV_TIMEOUT_TIME);
}

static void PassWordRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Recv auth G node confirm with Ra.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthGCfmWithRaMsg_S),
                              "[SM][PASSWORD] Recv G node confirm with ra pkg size failure.");
    SmAuthGCfmWithRaMsg_S *msg = (SmAuthGCfmWithRaMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Recv G confirm: %s", SDF_GET_ENC_STR(msg->gNodeCfm, SM_CONFIRM_NUMBER_LEN));
    /* 先保存确认码和Ra */
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->gNodeCfm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->randomA, SM_RANDOM_NUMBER_R_LEN);
    if (slink->uapiTimerHandle == TIMER_NO_USED_VALUE) {
        /* 如果T的北向handle已经被释放，此时T一定已经生成了Rb与确认码
         * 将flag置回false，验证G确认码后再发送携带Rb的确认码消息 */
        slink->tNode.recvFlag = false;
        if (!CheckPassWordConfirm(slink, slink->gNode.confirm, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN,
                                  slink->tNode.mixWord, SM_MIX_PASSWORD_LEN)) {
            /* 验证不正确则中止配对 */
            NLSTK_LOG_ERROR("[SM][PASSWORD] G Confirm code check error.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PASSWORD_ENTRY });
            return;
        }
        /* 发送携带Rb的T节点确认码 */
        PassWordSendTNodeCfmWithRb(slink);
    } else {
        slink->tNode.recvFlag = true;
    }
}

static void PassWordSendTNodeCfmWithRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Send auth T node confirm with Rb.");
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Send T confirm: %s", SDF_GET_ENC_STR(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthTCfmWithRbMsg_S msg = {0};
    (void)memcpy_s(msg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomB, SM_RANDOM_NUMBER_R_LEN, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_T_NODE_CFM_WITH_RB, (const uint8_t *)&msg, sizeof(SmAuthTCfmWithRbMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    /* 计算DH Key */
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] T node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] T node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

static void PassWordRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Recv auth T node confirm with Rb.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthTCfmWithRbMsg_S),
                              "[SM][PASSWORD] Recv T node confirm with rb pkg size failure.");
    SmAuthTCfmWithRbMsg_S *msg = (SmAuthTCfmWithRbMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Recv T confirm: %s", SDF_GET_ENC_STR(msg->tNodeCfm, SM_CONFIRM_NUMBER_LEN));
    /* G节点验证T节点确认码 */
    if (!CheckPassWordConfirm(slink, msg->tNodeCfm, msg->randomB, SM_RANDOM_NUMBER_R_LEN,
                              slink->gNode.mixWord, SM_MIX_PASSWORD_LEN)) {
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][PASSWORD] G node: T Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_INTERNAL_ERROR });
        return;
    }
    /* 验证一致则保存 */
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->tNodeCfm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->randomB, SM_RANDOM_NUMBER_R_LEN);
    /* 计算DH Key */
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] G node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        (void)memset_s(&keyPair, sizeof(keyPair), 0, sizeof(keyPair));
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] G node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        (void)memset_s(&keyPair, sizeof(keyPair), 0, sizeof(keyPair));
        return;
    }
    SmSendGNodeDhKey(slink);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
    // 清理 keyPair 中的敏感数据
    (void)memset_s(&keyPair, sizeof(keyPair), 0, sizeof(keyPair));
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

static void GenMixPassWord(SmSLink_S *slink, NLSTK_SmPassWord_S *passWord, uint8_t *mixWord, uint8_t wordLen)
{
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    /* G节点公钥低128比特 */
    (void)memcpy_s(input.key, SM_OCTETS_16, slink->gNode.pubKey, SM_OCTETS_16);
    uint8_t size = passWord->passWordLen;
    uint8_t *buffM = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t) * size);
    NLSTK_CHECK_RETURN_VOID(buffM != NULL, "[SM][PASSWORD] Generate mix password malloc error.");
    for (int i = 0; i < size; i++) {
        buffM[i] = (uint8_t)(passWord->passWord[i]);
    }
    input.buff = buffM;
    input.buffSize = size;
    SmCmacGenerate(&input, mixWord, wordLen);
    SDF_MemFree(buffM);
}

static bool GenPassWordConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink,
                                  uint8_t *mixWord, uint8_t wordLen, SmConfirmNum_S *confirm)
{
    NLSTK_CHECK_RETURN(wordLen == SM_MIX_PASSWORD_LEN, false, "[SM][PASSWORD] Password length error.");
    uint8_t buff[SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + SM_MIX_PASSWORD_LEN];
    /* G节点公钥 || T节点公钥 || G/T节点生成的混淆口令码 */
    if ((memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN, wordLen,
                  mixWord, wordLen) != EOK)) {
        return false;
    }
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    if (memcpy_s(input.key, SM_OCTETS_16, key, keyLen) != EOK) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] Generate password memcpy failed keyLen: %u.", keyLen);
        return false;
    }
    input.buff = buff;
    input.buffSize = SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + wordLen;
    if (!SmCmacGenerate(&input, confirm->confirm, SM_CONFIRM_NUMBER_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] Generate password confirm num error.");
        return false;
    }
    return true;
}

static bool CheckPassWordConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen,
                                 uint8_t *mixWord, uint8_t wordLen)
{
    NLSTK_LOG_DEBUG("[SM][PASSWORD] Start to check confirm number.");
    SmConfirmNum_S confirm = {0};
    if (!GenPassWordConfirmNum(randomNum, randLen, slink, mixWord, wordLen, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] Confirm number generate failed.");
        return false;
    }
    if (memcmp(confirmRecv, confirm.confirm, SM_CONFIRM_NUMBER_LEN) != 0) {
        NLSTK_LOG_ERROR("[SM][PASSWORD] Confirm code is inconsistent.");
        return false;
    }
    return true;
}