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
#include "sm_errcode.h"
#include "sm_algos.h"
#include "sm_dft.h"
#include "sm_dhkey.h"
#include "sm_passcode.h"

typedef enum {
    PASSCODE_G_DISPLAY_T_INPUT,     /* G显示 T输入 */
    PASSCODE_T_DISPLAY_G_INPUT,     /* T显示 G输入 */
    PASSCODE_PATTERN_INVALID,
} PassCodePattern_E;

static void PassCodeStartPatternInputG(SmSLink_S *slink);
static void PassCodeStartPatternInputT(SmSLink_S *slink);

static void PassCodeSendGNodeCfmWithRa(SmSLink_S *slink);
static void PassCodeRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void PassCodeSendTNodeCfmWithRb(SmSLink_S *slink);
static void PassCodeRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);

static PassCodePattern_E CheckPassCodePattern(SmSLink_S *slink);
static uint32_t GenerateSixDigits(SmAuthUserCode_S *code);
static void GenMixPassCode(SmSLink_S *slink, uint32_t passCodeNum, uint8_t *mixCode, uint8_t codeLen);
static bool GenPassCodeConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink,
                                  uint8_t *mixCode, uint8_t codeLen, SmConfirmNum_S *confirm);
static bool CheckPassCodeConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen,
                                 uint8_t *mixCode, uint8_t codeLen);


void SmPassCodeAuthStart(SmSLink_S *slink)
{
    PassCodePattern_E pattern = CheckPassCodePattern(slink);
    if (pattern == PASSCODE_G_DISPLAY_T_INPUT) {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_METHOD, SM_DFT_PASSCODE);
        PassCodeStartPatternInputT(slink);
    } else if (pattern == PASSCODE_T_DISPLAY_G_INPUT) {
        SmDftCacheU16(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_METHOD, SM_DFT_PASSCODE);
        PassCodeStartPatternInputG(slink);
    } else {
        NLSTK_LOG_ERROR("[SM][PASSCODE] Pattern check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_AUTHENTICATION_REQUIREMENTS });
        return;
    }
}

void SmPassCodeAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_G_NODE_CFM_WITH_RA: {          /* T节点，收到G发送的携带Ra的确认码消息 */
            PassCodeRecvGNodeCfmWithRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_CFM_WITH_RB: {          /* G节点，收到T发送的携带Rb的确认码消息 */
            PassCodeRecvTNodeCfmWithRb(slink, pkg, size);
            break;
        }
        case SM_AUTH_G_NODE_DHKEY: {                /* T节点，收到G发送的DHKEY验证码消息 */
            SmRecvGNodeDhKey(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_DHKEY: {                /* G节点，收到T发送的DHKEY验证码消息 */
            SmRecvTNodeDhKey(slink, pkg, size);
            break;
        }
        default:
            NLSTK_LOG_ERROR("[SM][PASSCODE] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

static void PassCodeStartPatternInputG(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        /* 通知G端用户可以输入，G端上层不会使用code值，因此无需赋值 */
        SmAuthUserCode_S code = {0};
        SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
        SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    } else {
        /* 生成六位数字后上报 */
        SmAuthUserCode_S code = {0};
        slink->tNode.passCode = GenerateSixDigits(&code);
        if (slink->tNode.passCode == 0) {
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
        /* 生成随机数Rb 混淆码 以及T节点确认码 */
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PASSCODE] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        GenMixPassCode(slink, slink->tNode.passCode, slink->tNode.mixCode, SM_MIX_PASSCODE_LEN);
        SmConfirmNum_S confirm = {0};
        if (!GenPassCodeConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                   slink->tNode.mixCode, SM_MIX_PASSCODE_LEN, &confirm)) {
            NLSTK_LOG_ERROR("[SM][PASSCODE] T Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_CFM_WITH_RA, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    }
}

static void PassCodeStartPatternInputT(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        /* 生成六位数字后上报 */
        SmAuthUserCode_S code = {0};
        slink->gNode.passCode = GenerateSixDigits(&code);
        if (slink->gNode.passCode == 0) {
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
        /* 生成随机数Ra 混淆码 以及G节点确认码 */
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][PASSCODE] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        GenMixPassCode(slink, slink->gNode.passCode, slink->gNode.mixCode, SM_MIX_PASSCODE_LEN);
        SmConfirmNum_S confirm = {0};
        if (!GenPassCodeConfirmNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                   slink->gNode.mixCode, SM_MIX_PASSCODE_LEN, &confirm)) {
            NLSTK_LOG_ERROR("[SM][PASSCODE] G Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        PassCodeSendGNodeCfmWithRa(slink);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM_WITH_RB, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
    } else {
        /* 通知T端用户可以输入，T端上层不会使用code值，因此无需赋值 */
        SmAuthUserCode_S code = {0};
        SmSLinkRequestCbk(slink, slink->negoParams.authMethod, code);
        SmSLinkWaitUapiInput(slink, SM_RECV_USER_CONFIRM_TIMEOUT_TIME);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_CFM_WITH_RA, SM_RECV_TIMEOUT_TIME);
    }
}

/*****************************************************************************************
                            Authentication Message Procedure
*****************************************************************************************/

void SmPassCodeSetGPassCode(SmSLink_S *slink)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_GEN_PASSCODE_TIME);
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Set G PassCode.");
    /* 生成随机数Ra 混淆码 以及G节点确认码 */
    if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] G random number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    GenMixPassCode(slink, slink->gNode.passCode, slink->gNode.mixCode, SM_MIX_PASSCODE_LEN);
    SmConfirmNum_S confirm = {0};
    if (!GenPassCodeConfirmNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                               slink->gNode.mixCode, SM_MIX_PASSCODE_LEN, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] G Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    PassCodeSendGNodeCfmWithRa(slink);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM_WITH_RB, SM_RECV_TIMEOUT_TIME);
}

void SmPassCodeSetTPassCode(SmSLink_S *slink)
{
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_CFM_TIME);
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Set T PassCode.");
    /* 生成随机数Rb 混淆码 以及T节点确认码 */
    if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] T random number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    GenMixPassCode(slink, slink->tNode.passCode, slink->tNode.mixCode, SM_MIX_PASSCODE_LEN);
    SmConfirmNum_S confirm = {0};
    if (!GenPassCodeConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                               slink->tNode.mixCode, SM_MIX_PASSCODE_LEN, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] T Confirm number generate failed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
    if (slink->tNode.recvFlag) {
        /* 假如已经收到来自G的确认码消息，将flag置回false，验证G确认码后再发送携带Rb的确认码消息 */
        slink->tNode.recvFlag = false;
        if (!CheckPassCodeConfirm(slink, slink->gNode.confirm, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN,
                                  slink->tNode.mixCode, SM_MIX_PASSCODE_LEN)) {
            /* 验证不一致则中止配对 */
            NLSTK_LOG_ERROR("[SM][PASSCODE] G Confirm code check error.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PASSCODE });
            return;
        }
        PassCodeSendTNodeCfmWithRb(slink);
    }
}

static void PassCodeSendGNodeCfmWithRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Send auth G node confirm with Ra.");
    SmAuthGCfmWithRaMsg_S msg = {0};
    (void)memcpy_s(msg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmSendMessage(slink, SM_AUTH_G_NODE_CFM_WITH_RA, (const uint8_t *)&msg, sizeof(SmAuthGCfmWithRaMsg_S));
}

static void PassCodeRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PASSCODE] T node recv G cfm.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthGCfmWithRaMsg_S),
                              "[SM][PASSCODE] Recv G node confirm with ra pkg size failure.");
    SmAuthGCfmWithRaMsg_S *msg = (SmAuthGCfmWithRaMsg_S *)pkg;
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_RECV_CFM_TIME);
    /* 先保存确认码和Ra */
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->gNodeCfm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->randomA, SM_RANDOM_NUMBER_R_LEN);
    slink->tNode.recvFlag = true;
    if (slink->uapiTimerHandle == TIMER_NO_USED_VALUE) {
        /* 如果T的北向handle已经被释放或未被使用，那么无论场景是G输入还是T输入，此时T一定已经生成了Rb与确认码
         * 将flag置回false，验证G确认码后再发送携带Rb的确认码消息 */
        slink->tNode.recvFlag = false;
        if (!CheckPassCodeConfirm(slink, msg->gNodeCfm, msg->randomA, SM_RANDOM_NUMBER_R_LEN,
                                  slink->tNode.mixCode, SM_MIX_PASSCODE_LEN)) {
            /* 验证不正确则中止配对 */
            SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_PASS_CODE_ERR);
            NLSTK_LOG_ERROR("[SM][PASSCODE] G Confirm code check error.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PASSCODE });
            return;
        }
        PassCodeSendTNodeCfmWithRb(slink);
    }
}

static void PassCodeSendTNodeCfmWithRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Send auth T node confirm with Rb.");
    SmAuthTCfmWithRbMsg_S msg = {0};
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_SEND_CFM_TIME);
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
        NLSTK_LOG_ERROR("[SM][PASSCODE] T node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
        NLSTK_LOG_ERROR("[SM][PASSCODE] T node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_KEY_TIME);
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

static void PassCodeRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Recv auth T node confirm with Rb.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthTCfmWithRbMsg_S),
                              "[SM][PASSCODE] Recv T node confirm with rb pkg size failure.");
    SmAuthTCfmWithRbMsg_S *msg = (SmAuthTCfmWithRbMsg_S *)pkg;
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_RECV_CFM_TIME);
    /* G节点验证T节点确认码 */
    if (!CheckPassCodeConfirm(slink, msg->tNodeCfm, msg->randomB, SM_RANDOM_NUMBER_R_LEN,
                              slink->gNode.mixCode, SM_MIX_PASSCODE_LEN)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_CFM_CODE_ERR);
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][PASSCODE] G node: T Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_PASSCODE });
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
        NLSTK_LOG_ERROR("[SM][PASSCODE] G node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_GEN_KEY_TIME);
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_LINKKEY_ERR);
        NLSTK_LOG_ERROR("[SM][PASSCODE] G node: Link key generation failure.");
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

static PassCodePattern_E CheckPassCodePattern(SmSLink_S *slink)
{
    if (slink->gNode.ioAbility == SM_IO_KEYBOARD_ONLY || slink->tNode.ioAbility == SM_IO_DISPLAY_ONLY) {
        return PASSCODE_T_DISPLAY_G_INPUT;
    } else if (slink->gNode.ioAbility == SM_IO_DISPLAY_ONLY || slink->tNode.ioAbility == SM_IO_KEYBOARD_ONLY) {
        return PASSCODE_G_DISPLAY_T_INPUT;
    } else {
        return PASSCODE_PATTERN_INVALID;
    }
}

static uint32_t GenerateSixDigits(SmAuthUserCode_S *code)
{
    uint8_t out[SM_OCTETS_3];
    if (!SmGenRandNum(out, SM_OCTETS_3)) {
        NLSTK_LOG_ERROR("[SM] SmGenRandNum Error");
        return 0;
    }
    uint32_t num = 0;
    // 将 3 个字节合并为 20 位的数字
    num |= (uint32_t)out[SM_OCTETS_0] << SM_OCTETS_12;  // 高 8 位
    num |= (uint32_t)out[SM_OCTETS_1] << SM_OCTETS_4;   // 中间 8 位
    num |= (uint32_t)out[SM_OCTETS_2] >> SM_OCTETS_4;   // 低 4 位 (截掉后 4 位)
    num = num % MAX_SIX_DIGIT + MIN_SIX_DIGIT;  // 将值限定为六位数 (100000 to 999999)
    (void)sprintf_s(code->code, SM_OCTETS_7, "%06u", num);
    return num;
}

static void GenMixPassCode(SmSLink_S *slink, uint32_t passCodeNum, uint8_t *mixCode, uint8_t codeLen)
{
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    /* G节点公钥低128比特 */
    (void)memcpy_s(input.key, SM_OCTETS_16, slink->gNode.pubKey, SM_OCTETS_16);
    uint8_t buffM[SM_MIX_PASSCODE_LEN];
    for (uint8_t idx = 0; idx < sizeof(passCodeNum); idx++) {
        uint8_t byte = (passCodeNum >> (idx * SM_OCTETS_8)) & SM_BYTE_MAX_VAL;
        buffM[idx] = byte;
    }
    input.buff = buffM;
    input.buffSize = SM_MIX_PASSCODE_LEN;
    SmCmacGenerate(&input, mixCode, codeLen);
}

static bool GenPassCodeConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink,
                                  uint8_t *mixCode, uint8_t codeLen, SmConfirmNum_S *confirm)
{
    NLSTK_CHECK_RETURN(codeLen == SM_MIX_PASSCODE_LEN, false, "[SM][PASSCODE] Passcode length error.");
    uint8_t buff[SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + SM_MIX_PASSCODE_LEN];
    /* G节点公钥 || T节点公钥 || G/T节点生成的混淆通行码 */
    if ((memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN) != EOK) ||
        (memcpy_s(buff + SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN, codeLen,
                  mixCode, codeLen) != EOK)) {
        return false;
    }
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, key, keyLen);
    input.buff = buff;
    input.buffSize = SM_PUBLIC_KEY_LEN + SM_PUBLIC_KEY_LEN + codeLen;
    if (!SmCmacGenerate(&input, confirm->confirm, SM_CONFIRM_NUMBER_LEN)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] Generate passcode confirm num error.");
        return false;
    }
    return true;
}

static bool CheckPassCodeConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen,
                                 uint8_t *mixCode, uint8_t codeLen)
{
    NLSTK_LOG_DEBUG("[SM][PASSCODE] Start to check confirm number.");
    SmConfirmNum_S confirm = {0};
    if (!GenPassCodeConfirmNum(randomNum, randLen, slink, mixCode, codeLen, &confirm)) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] Confirm number generate failed.");
        return false;
    }
    if (memcmp(confirmRecv, confirm.confirm, SM_CONFIRM_NUMBER_LEN) != 0) {
        NLSTK_LOG_ERROR("[SM][PASSCODE] Confirm code is inconsistent.");
        return false;
    }
    return true;
}