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
#include "sm_oob.h"

static void OobSendGNodeCfmWithRa(SmSLink_S *slink);
static void OobRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void OobSendTNodeCfmWithRb(SmSLink_S *slink);
static void OobRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size);

static bool CheckOobConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen);

void SmOobAuthStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {
        if (!SmGenRandNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][OOB] G random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmConfirmNum_S confirm = {0};
        if (!SmGenConfirmNum(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink, &confirm)) {
            NLSTK_LOG_ERROR("[SM][OOB] G Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        /* 发送携带Ra的G节点确认码 */
        OobSendGNodeCfmWithRa(slink);
    } else {
        if (!SmGenRandNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN)) {
            NLSTK_LOG_ERROR("[SM][OOB] T random number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        SmConfirmNum_S confirm = {0};
        if (!SmGenConfirmNum(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink, &confirm)) {
            NLSTK_LOG_ERROR("[SM][OOB] T Confirm number generate failed.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
            return;
        }
        (void)memcpy_s(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN, confirm.confirm, SM_CONFIRM_NUMBER_LEN);
        SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_CFM_WITH_RA, SM_RECV_TIMEOUT_TIME);
    }
}

void SmOobAuthPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    switch (opcode) {
        case SM_AUTH_G_NODE_CFM_WITH_RA: {     /* T节点，收到G发送的携带Ra的确认码消息 */
            OobRecvGNodeCfmWithRa(slink, pkg, size);
            break;
        }
        case SM_AUTH_T_NODE_CFM_WITH_RB: {     /* G节点，收到T发送的携带Rb的确认码消息 */
            OobRecvTNodeCfmWithRb(slink, pkg, size);
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
            NLSTK_LOG_ERROR("[SM][OOB] Auth pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

static void OobSendGNodeCfmWithRa(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][OOB] Send auth G node confirm with Ra.");
    NLSTK_LOG_DEBUG("[SM][OOB] Send G confirm: %s", SDF_GET_ENC_STR(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthGCfmWithRaMsg_S msg = {0};
    (void)memcpy_s(msg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_G_NODE_CFM_WITH_RA, (const uint8_t *)&msg, sizeof(SmAuthGCfmWithRaMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_T_NODE_CFM_WITH_RB, SM_RECV_TIMEOUT_TIME);
}

static void OobRecvGNodeCfmWithRa(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][OOB] Recv auth G node confirm with Ra.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthGCfmWithRaMsg_S),
                              "[SM][OOB] Recv G node confirm with ra pkg size failure.");
    SmAuthGCfmWithRaMsg_S *msg = (SmAuthGCfmWithRaMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][OOB] Recv G confirm: %s", SDF_GET_ENC_STR(msg->gNodeCfm, SM_CONFIRM_NUMBER_LEN));
    /* T节点验证G节点确认码 */
    if (!CheckOobConfirm(slink, msg->gNodeCfm, msg->randomA, SM_RANDOM_NUMBER_R_LEN)) {
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][OOB] G Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_OUT_OF_BAND });
        return;
    }
    /* 验证一致则保存 */
    (void)memcpy_s(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, msg->gNodeCfm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, msg->randomA, SM_RANDOM_NUMBER_R_LEN);
    /* 计算DH Key */
    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    (void)memcpy_s(keyPair.priKey, SM_PRIVATE_KEY_LEN, slink->priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(keyPair.localPubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(keyPair.remotePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    if (!SmGenDhKey(&keyPair, slink->dhKey, SM_DHKEY_LEN)) {
        NLSTK_LOG_ERROR("[SM][OOB] T node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][OOB] T node: Link key generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 发送携带Rb的T节点确认码 */
    OobSendTNodeCfmWithRb(slink);
}

static void OobSendTNodeCfmWithRb(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM][OOB] Send auth T node confirm with Rb.");
    NLSTK_LOG_DEBUG("[SM][OOB] Send T confirm: %s", SDF_GET_ENC_STR(slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN));
    SmAuthTCfmWithRbMsg_S msg = {0};
    (void)memcpy_s(msg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, slink->tNode.confirm, SM_CONFIRM_NUMBER_LEN);
    (void)memcpy_s(msg.randomB, SM_RANDOM_NUMBER_R_LEN, slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    bool ret = SmSendMessage(slink, SM_AUTH_T_NODE_CFM_WITH_RB, (const uint8_t *)&msg, sizeof(SmAuthTCfmWithRbMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_AUTH_G_NODE_DHKEY, SM_RECV_TIMEOUT_TIME);
}

static void OobRecvTNodeCfmWithRb(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM][OOB] Recv auth T node confirm with Rb.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmAuthTCfmWithRbMsg_S),
                              "[SM][OOB] Recv T node confirm with rb pkg size failure.");
    SmAuthTCfmWithRbMsg_S *msg = (SmAuthTCfmWithRbMsg_S *)pkg;
    NLSTK_LOG_DEBUG("[SM][OOB] Recv T confirm: %s", SDF_GET_ENC_STR(msg->tNodeCfm, SM_CONFIRM_NUMBER_LEN));
    /* G节点验证T节点确认码 */
    if (!CheckOobConfirm(slink, msg->tNodeCfm, msg->randomB, SM_RANDOM_NUMBER_R_LEN)) {
        /* 验证不一致则中止配对 */
        NLSTK_LOG_ERROR("[SM][OOB] T Confirm code check error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_OUT_OF_BAND });
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
        NLSTK_LOG_ERROR("[SM][OOB] G node: dhkey generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    /* 计算link Key */
    if (!SmGenLinkKey(slink)) {
        NLSTK_LOG_ERROR("[SM][OOB] G node: Link key generation failure.");
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

static bool CheckOobConfirm(SmSLink_S *slink, uint8_t *confirmRecv, uint8_t *randomNum, uint8_t randLen)
{
    NLSTK_LOG_DEBUG("[SM][OOB] Start to check confirm number.");
    SmConfirmNum_S confirm = {0};
    if (!SmGenConfirmNum(randomNum, randLen, slink, &confirm)) {
        NLSTK_LOG_ERROR("[SM][OOB] Confirm number generate failed.");
        return false;
    }
    if (memcmp(confirmRecv, confirm.confirm, SM_CONFIRM_NUMBER_LEN) != 0) {
        NLSTK_LOG_ERROR("[SM][OOB] Confirm code is inconsistent.");
        return false;
    }
    return true;
}