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
#include "sm_img.h"
#include "sm.h"
#include "sm_slink.h"
#include "sm_struct.h"
#include "nlstk_log.h"
#include "nlstk_sm_algos.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "securec.h"

static bool GenerateGroupKey(uint8_t algo, uint8_t *groupKey, uint8_t keyLen)
{
    NLSTK_CHECK_RETURN(groupKey != NULL && keyLen == SM_OCTETS_16, false, "[SM] invalid param.");
    uint8_t rand1[SM_OCTETS_16] = {0};
    uint8_t rand2[SM_OCTETS_16] = {0};
    uint8_t out[SM_OCTETS_16] = {0};
    bool ret = SmGenRandNum(rand1, SM_OCTETS_16);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate rand1 failed.");
    ret = SmGenRandNum(rand2, SM_OCTETS_16);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate rand2 failed.");
    NLSTK_SmDerivedMac_S input = {0};
    input.algo = algo;
    input.buff = rand2;
    input.buffSize = SM_OCTETS_16;
    (void)memcpy_s(input.key, SM_OCTETS_16, rand1, SM_OCTETS_16);
    ret = SmCmacGenerate(&input, out, SM_OCTETS_16);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate GK failed.");
    (void)memcpy_s(groupKey, SM_OCTETS_16, out, SM_OCTETS_16);
    return true;
}

static bool GenerateGiv(uint64_t *giv)
{
    NLSTK_CHECK_RETURN(giv != NULL, false, "[SM] invalid param.");
    uint64_t out = 0;
    bool ret = SmGenRandNum((uint8_t *)&out, sizeof(uint64_t));
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate giv failed.");
    *giv = out;
    return true;
}

void SmGenerateImgSecuConfig(uint8_t algo, uint8_t *groupKey, uint64_t *giv)
{
    NLSTK_LOG_DEBUG("[SM] Start to generate img security config.");
    NLSTK_CHECK_RETURN_VOID(groupKey != NULL && giv != NULL, "[SM] arg is null.");
    bool ret = GenerateGroupKey(algo, groupKey, SM_OCTETS_16);
    NLSTK_CHECK_RETURN_VOID(ret, "[SM] Generate groupKey failed.");
    ret = GenerateGiv(giv);
    NLSTK_CHECK_RETURN_VOID(ret, "[SM] Generate giv failed.");
}

static bool GenerateKg(uint8_t algo, uint8_t *linkKey, uint8_t *rand, uint8_t *output)
{
    uint8_t out[SM_OCTETS_16] = {0};
    bool ret = SmGenRandNum(out, SM_OCTETS_16);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate rand failed.");
    (void)memcpy_s(rand, SM_OCTETS_16, out, SM_OCTETS_16);
    NLSTK_SmDerivedMac_S input = {0};
    input.algo = algo;
    input.buff = rand;
    input.buffSize = SM_OCTETS_16;
    (void)memcpy_s(input.key, SM_OCTETS_16, linkKey, SM_OCTETS_16);
    ret = SmCmacGenerate(&input, out, SM_OCTETS_16);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate Kg failed.");
    (void)memcpy_s(output, SM_OCTETS_16, out, SM_OCTETS_16);
    return true;
}

static bool SendImgSecuConfig(SmSLink_S *slink, uint8_t imgId, uint8_t *groupKey, uint8_t *codeAlgoCap, uint64_t giv)
{
    NLSTK_LOG_DEBUG("[SM] Start to send img security config.");
    SmImgSecuConfigMsg_S msg = {0};
    msg.imgId = imgId;
    msg.giv = giv;
    (void)memcpy_s(msg.codeAlgoCap, SM_OCTETS_3, codeAlgoCap, SM_OCTETS_3);
    uint8_t kg[SM_OCTETS_16] = {0};
    uint8_t rand[SM_OCTETS_16] = {0};
    bool ret = GenerateKg(codeAlgoCap[SM_OCTETS_0], slink->linkKey, rand, kg);
    NLSTK_CHECK_RETURN(ret, false, "[SM] Generate Kg failed.");
    (void)memcpy_s(msg.rand, SM_OCTETS_16, rand, SM_OCTETS_16);
    for (uint8_t i = 0; i < SM_OCTETS_16; i++) {
        msg.c[i] = groupKey[i] ^ kg[i];
    }
    ret = SmSendMessage(slink, SM_IMG_SECU_CONFIG, (const uint8_t *)&msg, sizeof(SmImgSecuConfigMsg_S));
    NLSTK_CHECK_RETURN(ret, false, "[SM] Send Message failed.");
    return true;
}

void SmSendImgMsg(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] arg is null.");
    NLSTK_SmImgSecuConfig_S *config = (NLSTK_SmImgSecuConfig_S *)arg;
    NLSTK_SmSendImgMsgCmpl_S cmpl = {0};
    (void)memcpy_s(&cmpl.addr, sizeof(SLE_Addr_S), &config->addr, sizeof(SLE_Addr_S));
    SmSLink_S *slink = SmFindSLink(&config->addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] not find slink.");
        cmpl.sendStatus = SM_IMG_ERROR;
        SmExternalCbks(SM_CBK_EVENT_IMG_SEND_MESSAGE, &cmpl);
        return;
    }
    uint8_t codeAlgoCap[SM_OCTETS_3] = {0};
    codeAlgoCap[SM_OCTETS_0] = config->keyDerivAlgo;
    codeAlgoCap[SM_OCTETS_1] = config->cryptoAlgo;
    codeAlgoCap[SM_OCTETS_2] = config->intgChkInd;
    bool ret = SendImgSecuConfig(slink, config->imgId, config->groupKey, codeAlgoCap, config->giv);
    cmpl.sendStatus = ret ? SM_IMG_OK : SM_IMG_ERROR;
    SmExternalCbks(SM_CBK_EVENT_IMG_SEND_MESSAGE, &cmpl);
}

void SmEnableImgEncryption(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] arg is null.");
    NLSTK_SmImgEncpParam_S *enable = (NLSTK_SmImgEncpParam_S *)arg;
    DLI_IMGEncryptParam param = {0};
    param.handler = enable->imgHandle;
    if (enable->cryptoAlgo > 0) {
        param.algo = enable->cryptoAlgo - 1;
    } else {
        param.algo = 0;
    }
    (void)memcpy_s(&param.iv, sizeof(uint64_t), &enable->giv, sizeof(uint64_t));
    (void)memcpy_s(&param.key, SM_OCTETS_16, &enable->groupKey, SM_OCTETS_16);
    NLSTK_CHECK_RETURN_VOID(DLI_EnableIMGEncryption(&param) == DLI_SUCCESS, "[SM] dli enable img encryption failed.");
}