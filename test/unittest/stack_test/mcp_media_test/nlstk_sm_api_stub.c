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
#include "dli_cmd.h"
#include "dli_errno.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "sm.h"
#include "sm_algos.h"
#include "sm_img.h"
#include "nlstk_sm.h"
#include "nlstk_sm_api.h"

static void FreePassCode(void *arg);
static void FreePassWord(void *arg);
static void FreeLocalPsk(void *arg);
static void FreeRecoverKey(void *arg);

NLSTK_Errcode_E NLSTK_SmStartPairing(SLE_Addr_S *addrIn)
{
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Start pairing null pointer.");
    SLE_Addr_S *addr = SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Start pairing addr malloc error.");
    addr->type = addrIn->type;
    if (memcpy_s(addr->addr, sizeof(addr->addr), addrIn->addr, sizeof(addrIn->addr)) != EOK) {
        NLSTK_LOG_ERROR("[SM] Start pairing addr memcpy fail");
        SDF_MemFree(addr);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmStartPairing, addr, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Start pairing post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmRemovePairing(SLE_Addr_S *addrIn)
{
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Remove pairing null pointer.");
    SLE_Addr_S *addr = SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Remove pairing addr malloc error.");
    addr->type = addrIn->type;
    if (memcpy_s(addr->addr, sizeof(addr->addr), addrIn->addr, sizeof(addrIn->addr)) != EOK) {
        NLSTK_LOG_ERROR("[SM] Remove pairing addr memcpy fail");
        SDF_MemFree(addr);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmRemovePairing, addr, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Remove pairing post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetConfirm(SLE_Addr_S *addrIn)
{
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Set confirm null pointer.");
    SLE_Addr_S *addr = SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Set confirm addr malloc error.");
    addr->type = addrIn->type;
    if (memcpy_s(addr->addr, sizeof(addr->addr), addrIn->addr, sizeof(addrIn->addr)) != EOK) {
        NLSTK_LOG_ERROR("[SM] Set confirm addr memcpy fail");
        SDF_MemFree(addr);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmSetConfirm, addr, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Set confirm post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetPassCode(NLSTK_SmPassCode_S *passCodeIn)
{
    NLSTK_CHECK_RETURN(passCodeIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Set passcode null pointer.");
    NLSTK_SmPassCode_S *passCode = SDF_MemZalloc(sizeof(NLSTK_SmPassCode_S));
    NLSTK_CHECK_RETURN(passCode != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Set passcode malloc error.");
    passCode->passCode = passCodeIn->passCode;
    passCode->addr.type = passCodeIn->addr.type;
    if (memcpy_s(passCode->addr.addr, sizeof(passCode->addr.addr),
                 passCodeIn->addr.addr, sizeof(passCodeIn->addr.addr)) != EOK) {
        NLSTK_LOG_ERROR("[SM] Set passcode addr memcpy fail");
        SDF_MemFree(passCode);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmSetPassCode, passCode, FreePassCode) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Set passcode post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetPassWord(NLSTK_SmPassWord_S *passWordIn)
{
    NLSTK_CHECK_RETURN(passWordIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Set password null pointer.");
    size_t totalSize = sizeof(NLSTK_SmPassWord_S) + passWordIn->passWordLen;
    NLSTK_SmPassWord_S *passWord = SDF_MemZalloc(totalSize);
    NLSTK_CHECK_RETURN(passWord != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Set password malloc error.");
    passWord->passWordLen = passWordIn->passWordLen;
    if (memcpy_s(&passWord->addr, sizeof(SLE_Addr_S), &passWordIn->addr, sizeof(SLE_Addr_S)) != EOK ||
        memcpy_s(passWord->passWord, passWord->passWordLen, passWordIn->passWord, passWordIn->passWordLen) != EOK) {
        NLSTK_LOG_ERROR("[SM] Set password memcpy fail");
        SDF_MemFree(passWord);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmSetPassWord, passWord, FreePassWord) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Set password post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetLocalPsk(NLSTK_SmPsk_S *pskIn)
{
    NLSTK_CHECK_RETURN(pskIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Set psk null pointer.");
    NLSTK_SmPsk_S *localPsk = SDF_MemZalloc(sizeof(NLSTK_SmPsk_S));
    NLSTK_CHECK_RETURN(localPsk != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Set psk malloc error.");
    if (memcpy_s(&localPsk->addr, sizeof(SLE_Addr_S), &pskIn->addr, sizeof(SLE_Addr_S)) != EOK ||
        memcpy_s(localPsk->psk, SM_PSK_SEC_KEY_LEN, pskIn->psk, SM_PSK_SEC_KEY_LEN) != EOK) {
        NLSTK_LOG_ERROR("[SM] Set psk memcpy fail");
        SDF_MemFree(localPsk);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmSetLocalPsk, localPsk, FreeLocalPsk) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Set psk post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmRecoverKey(NLSTK_SmRecoverKeyParam_S *recoverKey, uint8_t keyNum)
{
    NLSTK_CHECK_RETURN((recoverKey != NULL) && (keyNum != 0), NLSTK_ERRCODE_POINTER_NULL,
                         "[SM] Recover key null pointer");
    NLSTK_SmRecoverKeyParamVector_S *recoverKeyList = SDF_MemZalloc(sizeof(NLSTK_SmRecoverKeyParamVector_S));
    NLSTK_CHECK_RETURN(recoverKeyList != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Recover key malloc fail");
    recoverKeyList->smKey = SDF_MemZalloc(sizeof(NLSTK_SmRecoverKeyParam_S) * keyNum);
    if (recoverKeyList->smKey == NULL) {
        NLSTK_LOG_ERROR("[SM] Recover key malloc fail");
        SDF_MemFree(recoverKeyList);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    recoverKeyList->num = keyNum;
    if (memcpy_s(recoverKeyList->smKey, sizeof(NLSTK_SmRecoverKeyParam_S) * keyNum,
        recoverKey, sizeof(NLSTK_SmRecoverKeyParam_S) * keyNum) != EOK) {
        NLSTK_LOG_ERROR("[SM] Recover key memcpy fail");
        FreeRecoverKey(recoverKeyList);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmRecoverKey, recoverKeyList, (SDF_FreeWorkArg)FreeRecoverKey) !=
        NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Recover key post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSetSecurityParams(NLSTK_SmLocalParams_S *paramsIn)
{
    NLSTK_CHECK_RETURN(paramsIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Set security params null pointer.");
    NLSTK_SmLocalParams_S *params = SDF_MemZalloc(sizeof(NLSTK_SmLocalParams_S));
    NLSTK_CHECK_RETURN(params != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] Set security params malloc error.");
    if (memcpy_s(params, sizeof(NLSTK_SmLocalParams_S), paramsIn, sizeof(NLSTK_SmLocalParams_S)) != EOK) {
        NLSTK_LOG_ERROR("[SM] Set security params memcpy fail.");
        SDF_MemFree(params);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    if (SchedulePostTask((SDF_WorkCb)SmSetSecurityParams, params, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Set security params post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmRegExternalCbks(NLSTK_SmCallbacks_S *cbksIn)
{
    NLSTK_CHECK_RETURN(cbksIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Register cbks null pointer.");
    NLSTK_SmCallbacks_S cbks = {
        .startCbk = cbksIn->startCbk,
        .removeCbk = cbksIn->removeCbk,
        .requestCbk = cbksIn->requestCbk,
        .authCbk = cbksIn->authCbk,
        .encCbk = cbksIn->encCbk,
        .imgMsgCbk = cbksIn->imgMsgCbk,
    };
    SmRegExternalCbks(&cbks);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmRegAlgoFuncs(NLSTK_SmCryptoAlgoFuncs_S *funsIn)
{
    NLSTK_SmCryptoAlgoFuncs_S *algos = SmGetAlgoFuncs();
    NLSTK_CHECK_RETURN(algos != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Get algo funcs failed.");
    algos->randNumFunc = funsIn->randNumFunc;
    algos->pubPriKeyFunc = funsIn->pubPriKeyFunc;
    algos->secKeyFunc = funsIn->secKeyFunc;
    algos->derivedKeyFunc = funsIn->derivedKeyFunc;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmSendImgSecuConfig(NLSTK_SmImgSecuConfig_S *config)
{
    NLSTK_CHECK_RETURN(config != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Send img secu config null pointer.");
    NLSTK_SmImgSecuConfig_S *configIn = SDF_MemZalloc(sizeof(NLSTK_SmImgSecuConfig_S));
    NLSTK_CHECK_RETURN(configIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] configIn malloc failed.");
    (void)memcpy_s(configIn, sizeof(NLSTK_SmImgSecuConfig_S), config, sizeof(NLSTK_SmImgSecuConfig_S));
    if (SchedulePostTask((SDF_WorkCb)SmSendImgMsg, configIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send img secu config post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmEnableImgEncp(NLSTK_SmImgEncpParam_S *param)
{
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Enable img encp null pointer.");
    NLSTK_SmImgEncpParam_S *paramIn = SDF_MemZalloc(sizeof(NLSTK_SmImgEncpParam_S));
    NLSTK_CHECK_RETURN(paramIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[SM] paramIn malloc failed.");
    (void)memcpy_s(paramIn, sizeof(NLSTK_SmImgEncpParam_S), param, sizeof(NLSTK_SmImgEncpParam_S));
    if (SchedulePostTask((SDF_WorkCb)SmEnableImgEncryption, paramIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Enable img encp post task failed.");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SmRegImgCbks(NLSTK_SmImgCallbacks_S *cbksIn)
{
    NLSTK_CHECK_RETURN(cbksIn != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SM] Register img cbks null pointer.");
    SmRegImgCbks(cbksIn);
    return NLSTK_ERRCODE_SUCCESS;
}

bool SmIsSLinkAuthComplete(uint16_t lcid)
{
    return true;
}

bool SmIsSLinkEncryptComplete(uint16_t lcid)
{
    return true;
}

static void FreePassCode(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] Free pass code null pointer.");
    NLSTK_SmPassCode_S *passCode = (NLSTK_SmPassCode_S *)arg;
    passCode->passCode = 0;
    SDF_MemFree(passCode);
}

static void FreePassWord(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] Free pass word null pointer.");
    NLSTK_SmPassWord_S *passWord = (NLSTK_SmPassWord_S *)arg;
    (void)memset_s(passWord->passWord, passWord->passWordLen, 0, passWord->passWordLen);
    SDF_MemFree(passWord);
}

static void FreeLocalPsk(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] Free psk null pointer.");
    NLSTK_SmPsk_S *psk = (NLSTK_SmPsk_S *)arg;
    (void)memset_s(psk->psk, SM_PSK_SEC_KEY_LEN, 0, SM_PSK_SEC_KEY_LEN);
    SDF_MemFree(psk);
}

static void FreeRecoverKey(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[SM] Free recover key null pointer.");
    NLSTK_SmRecoverKeyParamVector_S *recoverKeyParamVector = (NLSTK_SmRecoverKeyParamVector_S *)arg;
    (void)memset_s(recoverKeyParamVector->smKey->linkKey, SM_LINK_KEY_LEN, 0, SM_LINK_KEY_LEN);
    SDF_MemFree(recoverKeyParamVector->smKey);
    SDF_MemFree(recoverKeyParamVector);
}