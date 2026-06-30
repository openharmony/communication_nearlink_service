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
#include "sdf_string.h"
#include "nlstk_log.h"

#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "sm_algos.h"
#include "sm_dhkey.h"
#include "sm_dft.h"
#include "sm_numcmp.h"

static SmAuthDhkeyMsg_S g_numCmpTNodeRecvDHKey;
static size_t g_numCmpTNodeRecvDHKeySize;
static void NumCmpSendTNodeCfmWithRb(SmSLink_S *slink);
static void NumCmpRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void NumCmpSendRandNumRa(SmSLink_S *slink);
static void NumCmpRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);

static bool GenSixCompareCode(SmSLink_S *slink, SmAuthUserCode_S *code);

void SmNumCmpAuthStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_METHOD, SM_DFT_NUMCMP);
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][NUMCMP] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM_WITH_RB, SM_RECV_TIMEOUT_TIME);
    } else {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_METHOD, SM_DFT_NUMCMP);
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][NUMCMP] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmConfirmNum_S confirm = {0};
        if (!SmGenConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink, &confirm)) {
            NLSTK_LOG_ERROR("[SM][NUMCMP] T Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_CFM_TIME);
        (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        NumCmpSendTNodeCfmWithRb(slink);
    }
}

void SmNumCmpAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_T_NODE_CFM_WITH_RB: {          /* G节点，收到T发送的携带Rb的确认码消息 */
            NumCmpRecvTNodeCfmWithRb(slink, pkg, size);
            break;
        }
        case SM_AUTH_RAND_NUM_RA: {                 /* T节点，收到G发送的Ra消息 */
            NumCmpRecvRandNumRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_G_NODE_DHKEY: {                /* T节点，收到G发送的DHKEY验证码消息 */
            slink->tNode.recvFlag = true;
            if (slink->uapiTimerHandle == TIMER_NO_USED_VALUE) {
                slink->tNode.recvFlag = false;
                SmRecvGNodeDhKey(slink, pkg, size);
            } else {
                NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthDhkeyMsg_S), "[SM] T node DhKey recv pkg size failure.");
                (void)memcpy_s(g_numCmpTNodeRecvDHKey.authData, SM_DHKEY_AUTHCODE_LEN, pkg, SM_DHKEY_AUTHCODE_LEN);
                g_numCmpTNodeRecvDHKeySize = size;
            }
            break;
        }
        case SM_AUTH_T_NODE_DHKEY: {                /* G节点，收到T发送的DHKEY验证码消息 */
            SmRecvTNodeDhKey(slink, pkg, size);
            break;
        }
        default:
            NLSTK_LOG_ERROR("[SM][NUMCMP] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

static void NumCmpSendTNodeCfmWithRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][NUMCMP] Send auth T node confirm with rb.");
    SmAuthTCfmWithRbMsg_S msg = {0};
    (void)memcpy_s(msg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomB, SM_RANDOM_NUMBER_R_LEN, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_SEND_CFM_TIME);
    bool ret = SmSendMessage(slink, SM_AUTH_T_NODE_CFM_WITH_RB, (const uint8_t *)&msg, sizeof(SmAuthTCfmWithRbMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_RAND_NUM_RA, SM_RECV_TIMEOUT_TIME);
}

static void NumCmpRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][NUMCMP] Recv auth T node confirm with rb.");
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_RECV_CFM_TIME);
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthTCfmWithRbMsg_S),
                              "[SM][NUMCMP] Recv T node confirm with rb pkg size failure.");
    SmAuthTCfmWithRbMsg_S *msg = (SmAuthTCfmWithRbMsg_S *)pkg;
    /* G节点接收到T节点确认码和Rb后先保存 */
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->tNodeCfm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->randomB, SM_RANDOM_NUMBER_R_LEN);
    NumCmpSendRandNumRa(slink);
}

static void NumCmpSendRandNumRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][NUMCMP] Send auth random num ra.");
    SmAuthRandomAMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_SEND_RA_TIME);
    bool ret = SmSendMessage(slink, SM_AUTH_RAND_NUM_RA, (const uint8_t *)&msg, sizeof(SmAuthRandomAMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    /* 计算T节点确认码 */
    SmConfirmNum_S confirm = {0};
    if (!SmGenConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink, &confirm)) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] G Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    if (memcmp(slink->tNode.confirm, confirm.confirm, SM_RANDOM_NUMBER_R_LEN) != 0) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] Confirm code from T is inconsistent.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_NUMBER_COMPARE });
        return;
    }
    /* 生成六位数字 */
    SmAuthUserCode_S code = {0};
    if (!GenSixCompareCode(slink, &code)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_SIX_DIGIT_ERR);
        NLSTK_LOG_ERROR("[SM][NUMCMP] Generate six compare code error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
    SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
}

static void NumCmpRecvRandNumRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][NUMCMP] Recv auth random num ra.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthRandomAMsg_S), "[SM][NUMCMP] Recv random ra pkg size failure.");
    SmAuthRandomAMsg_S *msg = (SmAuthRandomAMsg_S *)pkg;
    /* T节点保存Ra */
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->authData, SM_RANDOM_NUMBER_R_LEN);
    /* 生成六位数字 */
    SmAuthUserCode_S code = {0};
    if (!GenSixCompareCode(slink, &code)) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] Generate six compare code error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
    SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
}

static void SmNumCmpHandleGNode(SmSLink_S *slink)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_USER_CFM_TIME);
    // 计算DH Key
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] G node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    // 计算link Key
    if (!SmGenLinkKey(slink)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
        NLSTK_LOG_ERROR("[SM][NUMCMP] G node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_GEN_KEY_TIME);
    // 发送DHKey
    SmSendGNodeDhKey(slink);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_DHKEY, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
}

static void SmNumCmpHandleTNode(SmSLink_S *slink)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_USER_CFM_TIME);
    // 计算DH Key
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] T node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    // 计算link Key
    if (!SmGenLinkKey(slink)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
        NLSTK_LOG_ERROR("[SM][NUMCMP] T node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_KEY_TIME);
    if (slink->tNode.recvFlag) {
        slink->tNode.recvFlag = false;
        SmRecvGNodeDhKey(slink, g_numCmpTNodeRecvDHKey.authData, g_numCmpTNodeRecvDHKeySize);
    }
}

void SmNumCmpContinueNumComparison(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        SmNumCmpHandleGNode(slink);
    } else {
        SmNumCmpHandleTNode(slink);
    }
}

static bool GenSixCompareCode(SmSLink_S *slink, SmAuthUserCode_S *code)
{
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    /* G节点公钥低128比特，由于按小端序存储，所以从前开始取 */
    (void)memcpy_s(input.key, SM_OCTETS_16, slink->gNode.pubKey, SM_OCTETS_16);
    uint8_t buffM[SM_OCTETS_16 + SM_PUBLIC_KEY_LEN + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN];
    /* G节点公钥高128比特 || T节点公钥 || 随机数Ra || 随机数Rb */
    /* G节点公钥高128比特，由于按小端序存储，所以从后开始取 */
    if ((memcpy_s(buffM, SM_OCTETS_16, slink->gNode.pubKey + SM_OCTETS_16, SM_OCTETS_16) != EOK) ||
        (memcpy_s(buffM + SM_OCTETS_16, SM_PUBLIC_KEY_LEN,
            slink->tNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buffM + SM_OCTETS_16 + SM_PUBLIC_KEY_LEN, SM_RANDOM_NUMBER_R_LEN,
            slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN) != EOK) ||
        (memcpy_s(buffM + SM_OCTETS_16 + SM_PUBLIC_KEY_LEN + SM_RANDOM_NUMBER_R_LEN, SM_RANDOM_NUMBER_R_LEN,
            slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN) != EOK)) {
        return false;
    }
    uint8_t output[SM_OCTETS_16];
    size_t size = SM_OCTETS_16 + SM_PUBLIC_KEY_LEN + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN;
    input.buff = buffM;
    input.buffSize = size;
    if (!SmCmacGenerate(&input, output, SM_OCTETS_16)) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] Generate six compare code error.");
        return false;
    }
    /* 将 uint8_t 数组转换为十进制 */
    uint32_t result = 0;
    uint32_t base = 1;
    for (int i = SM_OCTETS_15; i >= 0; i--) {
        uint8_t byte = output[i];
        // 提取高 4 位 (第一个十六进制位)
        uint32_t digit1 = (byte >> 4) & 0xF;
        // 提取低 4 位 (第二个十六进制位)
        uint32_t digit2 = byte & 0xF;
        result = (result + (digit2 * base) % LOW_SIX_DIGIT) % LOW_SIX_DIGIT;
        base = (base * SM_OCTETS_16) % LOW_SIX_DIGIT;
        result = (result + (digit1 * base) % LOW_SIX_DIGIT) % LOW_SIX_DIGIT;
        base = (base * SM_OCTETS_16) % LOW_SIX_DIGIT;
    }
    // 获取低六位数字
    if (snprintf_s(code->code, SM_OCTETS_7, SM_OCTETS_6, "%06lu", result) < 0) {
        NLSTK_LOG_ERROR("[SM][NUMCMP] snprintf_s error.");
        return false;
    }
    return true;
}