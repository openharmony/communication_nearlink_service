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
#include <stdint.h>
#include <stdbool.h>
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

typedef struct {
    uint8_t authCode[SM_DHKEY_AUTHCODE_LEN];
} DhKeyAuthCode_S;

typedef struct {
    uint8_t salt[SM_SALT_LEN];
} KeySalt_S;

static bool GenDhKeyCfmCodeKey(uint8_t *randomR, uint8_t randLen, SmSLink_S *slink,
                               SLE_Addr_S gAddr, SLE_Addr_S tAddr, DhKeyAuthCode_S *dhkeyCode);

void SmSendGNodeDhKey(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] G node send dhkey.");
    /* 生成G节点DH Key验证码密钥 */
    DhKeyAuthCode_S dhkeyCode = {0};
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_GEN_CFMKEY_TIME);
    bool ret = GenDhKeyCfmCodeKey(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                  slink->localAddr, slink->rmtAddr, &dhkeyCode);
    if (!ret) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_CFMKEY_ERR);
        NLSTK_LOG_ERROR("[SM] G Node: dhkey confirm code generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmAuthDhkeyMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode.authCode, SM_DHKEY_AUTHCODE_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_SEND_CFMKEY_TIME);
    SmSendMessage(slink, SM_AUTH_G_NODE_DHKEY, (const uint8_t *)&msg, sizeof(SmAuthDhkeyMsg_S));
}

void SmRecvGNodeDhKey(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] T node recv dhkey.");
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_RECV_CFMKEY_TIME);
    NLSTK_CHECK_RETURN_VOID(size == sizeof(SmAuthDhkeyMsg_S), "[SM] T node dhkey recv pkg size failure.");
    SmAuthDhkeyMsg_S *msg = (SmAuthDhkeyMsg_S *)pkg;

    DhKeyAuthCode_S code = {0};
    bool ret = GenDhKeyCfmCodeKey(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                  slink->rmtAddr, slink->localAddr, &code);
    if (!ret) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_CFMKEY_ERR);
        NLSTK_LOG_ERROR("[SM] T Node: dhkey confirm code generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }

    if (memcmp(msg->authData, code.authCode, SM_DHKEY_AUTHCODE_LEN) != 0) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_CFM_KEY_ERR);
        NLSTK_LOG_ERROR("[SM] T Node: DHKey authenticode from G inconsistent, lcid = %u.", slink->lcid);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_DHKEY_AUTHENTICODE_CHECK_FAILED });
        return;
    }

    SmSendTNodeDhKey(slink);
}

void SmSendTNodeDhKey(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] T node send dhkey.");
    DhKeyAuthCode_S dhkeyCode = {0};
    /* 生成T节点DH Key验证码密钥 */
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_GEN_CFMKEY_TIME);
    bool ret = GenDhKeyCfmCodeKey(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                  slink->rmtAddr, slink->localAddr, &dhkeyCode);
    if (!ret) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_CFMKEY_ERR);
        NLSTK_LOG_ERROR("[SM] T Node: dhkey confirm code generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    SmAuthDhkeyMsg_S msg = {0};
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_AUTH_EXCEP, SM_DFT_T_AUTH_SEND_CFMKEY_TIME);
    (void)memcpy_s(msg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode.authCode, SM_DHKEY_AUTHCODE_LEN);
    ret = SmSendMessage(slink, SM_AUTH_T_NODE_DHKEY, (const uint8_t *)&msg, sizeof(SmAuthDhkeyMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_AUTH_SUCCESS });
}

void SmRecvTNodeDhKey(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] G node recv dhkey.");
    NLSTK_CHECK_RETURN_VOID(size == sizeof(SmAuthDhkeyMsg_S), "[SM] G node dhkey recv pkg size failure.");
    SmAuthDhkeyMsg_S *msg = (SmAuthDhkeyMsg_S *)pkg;

    DhKeyAuthCode_S code = {0};
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_AUTH_EXCEP, SM_DFT_G_AUTH_RECV_CFMKEY_TIME);
    bool ret = GenDhKeyCfmCodeKey(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, slink,
                                  slink->localAddr, slink->rmtAddr, &code);
    if (!ret) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_GEN_CFMKEY_ERR);
        NLSTK_LOG_ERROR("[SM] G Node: dhkey confirm code generation failure.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }

    if (memcmp(msg->authData, code.authCode, SM_DHKEY_AUTHCODE_LEN) != 0) {
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_CFM_KEY_ERR);
        NLSTK_LOG_ERROR("[SM] G Node: DHKey authenticode from T inconsistent, lcid = %u.", slink->lcid);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_DHKEY_AUTHENTICODE_CHECK_FAILED });
        return;
    }
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_AUTH_SUCCESS });
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

bool SmGenLinkKey(SmSLink_S *slink)
{
    /* keyID 为字符串 "lk" 转化成扩展 ASCII 码的值 */
    uint16_t lk = (EXTEND_ASCII_CHAR_LITTLE_L << SM_SHIFT_BITS_8) | EXTEND_ASCII_CHAR_LITTLE_K;
    size_t size = sizeof(lk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN + SLE_ADDR_LEN + SLE_ADDR_LEN;
    uint8_t *buff = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t) * size); // 生成link key后释放
    NLSTK_CHECK_RETURN(buff != NULL, false, "[SM] Generate link key malloc fail.");
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    uint8_t *gAddr = (slink->role == SM_G_NODE) ? slink->localAddr.addr : slink->rmtAddr.addr;
    uint8_t *tAddr = (slink->role == SM_G_NODE) ? slink->rmtAddr.addr : slink->localAddr.addr;
    /* lk || 随机数Ra || 随机数Rb || G节点地址 || T节点地址 */
    if ((memcpy_s(buff, sizeof(lk), &lk, sizeof(lk)) != EOK) ||
        (memcpy_s(buff + sizeof(lk), SM_RANDOM_NUMBER_R_LEN,
            slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN) != EOK) ||
        (memcpy_s(buff + sizeof(lk) + SM_RANDOM_NUMBER_R_LEN, SM_RANDOM_NUMBER_R_LEN,
            slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN) != EOK) ||
        (memcpy_s(buff + sizeof(lk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN, SLE_ADDR_LEN,
            gAddr, SLE_ADDR_LEN) != EOK) ||
        (memcpy_s(buff + sizeof(lk) + SM_RANDOM_NUMBER_R_LEN + SM_RANDOM_NUMBER_R_LEN + SLE_ADDR_LEN, SLE_ADDR_LEN,
            tAddr, SLE_ADDR_LEN) != EOK) ||
        (memcpy_s(input.key, SM_OCTETS_16, slink->dhKey, SM_OCTETS_16) != EOK)) {
        SDF_MemFree(buff);
        return false;
    }
    input.buff = buff;
    input.buffSize = size;
    if (!SmCmacGenerate(&input, slink->linkKey, SM_LINK_KEY_LEN)) {
        NLSTK_LOG_ERROR("[SM] Generate link key error.");
        (void)memset_s(buff, sizeof(uint8_t) * size, 0, sizeof(uint8_t) * size);
        SDF_MemFree(buff);
        return false;
    }
    (void)memset_s(buff, sizeof(uint8_t) * size, 0, sizeof(uint8_t) * size);
    (void)memset_s(&input, sizeof(NLSTK_SmDerivedMac_S), 0x00, sizeof(NLSTK_SmDerivedMac_S));
    SDF_MemFree(buff);
    return true;
}

static void GetDhKeySalt(SmSLink_S *slink, KeySalt_S *salt)
{
    uint32_t passCodeNumber;
    switch (slink->negoParams.authMethod) {
        case SM_AUTH_NUMBER_COMPARE:
        case SM_AUTH_NO_ENTRY:
        case SM_AUTH_OUT_OF_BAND:
        case SM_AUTH_PSK:
            (void)memset_s(salt->salt, SM_SALT_LEN, 0, SM_SALT_LEN);
            break;
        case SM_AUTH_PASSCODE:
            passCodeNumber = (slink->role == SM_G_NODE) ? slink->gNode.passCode : slink->tNode.passCode;
            for (uint8_t idx = 0; idx < sizeof(passCodeNumber); idx++) {
                uint8_t byte = (passCodeNumber >> (idx * SM_OCTETS_8)) & SM_BYTE_MAX_VAL;
                salt->salt[idx] = byte;
            }
            break;
        default:
            break;
    }
}

static bool GetInputKey(uint8_t randLen, SmSLink_S *slink, SLE_Addr_S gAddr, SLE_Addr_S tAddr,
                        uint8_t *key, uint8_t keyLen)
{
    uint16_t dk = EXTEND_ASCII_CHAR_LITTLE_D << SM_SHIFT_BITS_8 | EXTEND_ASCII_CHAR_LITTLE_K;
    size_t size = sizeof(dk) + randLen + randLen + SLE_ADDR_LEN + SLE_ADDR_LEN;
    uint8_t *buff = SDF_MemZalloc(sizeof(uint8_t) * size); // 生成key后释放
    NLSTK_CHECK_RETURN(buff != NULL, false, "[SM] Generate dhkey confirm code key malloc fail.");
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
    };
    /* dk || 随机数Ra || 随机数Rb || G节点地址 || T节点地址 */
    if ((memcpy_s(buff, sizeof(dk), &dk, sizeof(dk)) != EOK) ||
        (memcpy_s(buff + sizeof(dk), randLen, slink->gNode.randomR, randLen) != EOK) ||
        (memcpy_s(buff + sizeof(dk) + randLen, randLen, slink->tNode.randomR, randLen) != EOK) ||
        (memcpy_s(buff + sizeof(dk) + randLen + randLen, SLE_ADDR_LEN, gAddr.addr, SLE_ADDR_LEN) != EOK) ||
        (memcpy_s(buff + sizeof(dk) + randLen + randLen + SLE_ADDR_LEN, SLE_ADDR_LEN,
            tAddr.addr, SLE_ADDR_LEN) != EOK) ||
        /* DH Key截断 */
        (memcpy_s(input.key, SM_OCTETS_16, slink->dhKey, SM_OCTETS_16) != EOK)) {
        SDF_MemFree(buff);
        return false;
    }
    input.buff = buff;
    input.buffSize = size;
    if (!SmCmacGenerate(&input, key, keyLen)) {
        NLSTK_LOG_ERROR("[SM] Generate input key error.");
        SDF_MemFree(buff);
        return false;
    }
    NLSTK_LOG_DEBUG("[SM] Generate Input key succeed.");
    SDF_MemFree(buff);
    return true;
}

static bool GenDhKeyCfmCodeKey(uint8_t *randomR, uint8_t randLen, SmSLink_S *slink,
                               SLE_Addr_S gAddr, SLE_Addr_S tAddr, DhKeyAuthCode_S *dhkeyCode)
{
    NLSTK_LOG_DEBUG("[SM] gAddr: %s, tAddr: %s", GET_ENC_ADDR(&gAddr), GET_ENC_ADDR(&tAddr));
    uint8_t key[SM_OCTETS_16];
    if (!GetInputKey(randLen, slink, gAddr, tAddr, key, SM_OCTETS_16)) {
        NLSTK_LOG_ERROR("[SM] Generate input key fail.");
        return false;
    }
    /*
     * G节点： 随机数Ra || salt(加盐) || G节点输入输出能力 || T节点输入输出能力 || 鉴权方式 || 密码算法 ||
     *        G节点PSK指示 || T节点PSK指示 || G节点地址 || T节点地址
     * T节点： 随机数Rb || salt(加盐) || G节点输入输出能力 || T节点输入输出能力 || 鉴权方式 || 密码算法 ||
     *        G节点PSK指示 || T节点PSK指示 || G节点地址 || T节点地址
     */
    size_t size = randLen + SM_SALT_LEN + SM_OCTETS_1 + SM_OCTETS_1 + SM_OCTETS_1 + SM_OCTETS_4 +
                  SM_OCTETS_1 + SM_OCTETS_1 + SLE_ADDR_LEN + SLE_ADDR_LEN;
    uint8_t *buff = SDF_MemZalloc(sizeof(uint8_t) * size); // 生成dhkey确认码后释放
    NLSTK_CHECK_RETURN(buff != NULL, false, "[SM] Generate dhkey confirm code key malloc fail.");

    KeySalt_S salt = {0};
    GetDhKeySalt(slink, &salt);
    buff[randLen + SM_SALT_LEN] = slink->gNode.ioAbility;
    buff[randLen + SM_SALT_LEN + SM_OCTETS_1] = slink->tNode.ioAbility;
    buff[randLen + SM_SALT_LEN + SM_OCTETS_2] = slink->negoParams.authMethod;
    buff[randLen + SM_SALT_LEN + SM_OCTETS_7] = slink->gNode.pskFlag;
    buff[randLen + SM_SALT_LEN + SM_OCTETS_8] = slink->tNode.pskFlag;
    if ((memcpy_s(buff, randLen, randomR, randLen) != EOK) ||
        (memcpy_s(buff + randLen, SM_SALT_LEN, salt.salt, SM_SALT_LEN) != EOK) ||
        (memcpy_s(buff + randLen + SM_SALT_LEN + SM_OCTETS_3, SM_OCTETS_4,
            slink->negoParams.codeAlgoCap, SM_OCTETS_4) != EOK) ||
        (memcpy_s(buff + randLen + SM_SALT_LEN + SM_OCTETS_9, SLE_ADDR_LEN, gAddr.addr, SLE_ADDR_LEN) != EOK) ||
        (memcpy_s(buff + randLen + SM_SALT_LEN + SM_OCTETS_9 + SLE_ADDR_LEN, SLE_ADDR_LEN,
            tAddr.addr, SLE_ADDR_LEN) != EOK)) {
        SDF_MemFree(buff);
        return false;
    }
    NLSTK_SmDerivedMac_S input = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
        .buff = buff,
        .buffSize = size,
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
    if (!SmCmacGenerate(&input, dhkeyCode->authCode, SM_DHKEY_AUTHCODE_LEN)) {
        NLSTK_LOG_ERROR("[SM] Generate dhkey auth code error.");
        SDF_MemFree(buff);
        return false;
    }
    NLSTK_LOG_DEBUG("[SM] Generate dhkey auth code succeed.");
    SDF_MemFree(buff);
    return true;
}