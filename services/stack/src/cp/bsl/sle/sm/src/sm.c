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
/*
 * SM: security management，安全管理.
 * 此文件提供了
 *      + 初始化和去初始化接口.
 *      + 星闪配对流程相关的接口.
 * 参考：
 * 配对流程包含以下三个阶段
 *      + 协商 - sm_nego.c
 *      + 鉴权 - sm_auth.c
 *      + 加密 - sm_encp.c
 * slink 承载配对中的数据，而有限状态机用于控制配对阶段的切换.
 *      + sm_slink.c
 *      + sm_stm.c
 */

#include "securec.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sdf_addr.h"
#include "sdf_string.h"
#include "cp_worker.h"
#include "nlstk_log.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_cfgdb.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "dli_event_struct.h"

#include "nlstk_sm.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sm_nego.h"
#include "sm_encp.h"
#include "sm_numcmp.h"
#include "sm_noentry.h"
#include "sm_passcode.h"
#include "sm_password.h"
#include "sm_errcode.h"
#include "sm_dft.h"
#include "sm.h"

#define SM_AUTH_DEFAULT_MASK    0xFF
#define SM_MAX_KEY_DEFAULT_LEN  16

SDF_Map *g_slinkMap = NULL;
NLSTK_SmLocalParams_S *g_localParams = NULL;

static NLSTK_SmCallbacks_S g_smCbkTbl = {0};
static NLSTK_SmImgCallbacks_S g_smImgCbk = {0};

static bool RegDliCbk(void);
static void UnRegDliCbk(void);
static bool SLinkLocalParamsInit(void);
static uint32_t RegLogicLinkListener(void);

void SmDeInit(void)
{
    if (g_slinkMap) {
        SDF_MapDtor(g_slinkMap);
        g_slinkMap = NULL;
    }
    if (g_localParams) {
        SDF_MemFree(g_localParams);
        g_localParams = NULL;
    }
    (void)CM_UnRegLogicLinkListener(CM_MODULE_SM);
    UnRegDliCbk();

    memset_s(&g_smCbkTbl, sizeof(g_smCbkTbl), 0, sizeof(g_smCbkTbl));
    memset_s(&g_smImgCbk, sizeof(g_smImgCbk), 0, sizeof(g_smImgCbk));
}

uint32_t SmInit(void)
{
    uint32_t ret = SM_OK;
    /* Step 1: 注册收发包的函数 */
    if (!RegDliCbk()) {
        NLSTK_LOG_ERROR("[SM] Register dli callbacks fail.");
        ret = SM_REGISTER_DLI_CBK_FAIL;
        goto FAIL_LABEL;
    }

    /* Step 2: SLINK容器初始化 */
    g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
    if (g_slinkMap == NULL) {
        NLSTK_LOG_ERROR("[SM] Construct slink map fail.");
        ret = SM_SLINK_MAP_INIT_FAIL;
        goto FAIL_LABEL;
    }

    /* Step 3: 本端参数初始化 */
    if (!SLinkLocalParamsInit()) {
        NLSTK_LOG_ERROR("[SM] Slink local params init fail.");
        ret = SM_LOCAL_PARAMS_INIT_FAIL;
        goto FAIL_LABEL;
    }

    /* Step 4: 注册监听接口 */
    ret = RegLogicLinkListener();
    if (ret != SM_OK) {
        NLSTK_LOG_ERROR("[SM] Register local link listener fail.");
        ret = SM_REGISTER_CM_LISTENER_FAIL;
        goto FAIL_LABEL;
    }

    return ret;

FAIL_LABEL:
    SmDeInit();
    return ret;
}

void SmEnable(void)
{
    NLSTK_CHECK_RETURN_VOID(g_localParams != NULL, "[SM] Global local param is null, SM not init.");
    CfgdbAlgoCaps_S param = {0};
    uint32_t ret = CfgdbReadAlgoCaps(&param);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "Read crypto algo callback error.");
    g_localParams->codeAlgoCap[SM_ENC_ALGO_ABILITY] = param.encryptAlgo & param.intgProtectAlgo;
    g_localParams->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = param.encryptAlgo & param.intgProtectAlgo;
    g_localParams->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = param.keyDerivAlgo;
    /* 若芯片指示不支持任何密钥交互算法，默认使用ECDH-P256 */
    g_localParams->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] =
        param.keyNegoAlgo | SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    NLSTK_LOG_INFO("[SM] CryptoAlgo: 0x%x 0x%x 0x%x 0x%x.",
                 g_localParams->codeAlgoCap[SM_ENC_ALGO_ABILITY],
                 g_localParams->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY],
                 g_localParams->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY],
                 g_localParams->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY]);
}

void SmPkgDispatcher(uint16_t opcode, SLE_Addr_S *addr, const uint8_t *pkg, size_t size)
{
    NLSTK_CHECK_RETURN_VOID(pkg != NULL, "[SM] Package null pointer failure.");

    /* Step 1: 根据地址在表中查询，不存在则创建 */
    SmSLink_S *slink = SmFindOrCreateSLink(addr);
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Find or create slink failed on recv pkg.");

    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);

    /* Step 2: 通知状态机收到报文，以被动方式启动配对流程
     *         若已处于配对流程，则忽略该事件 */
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_PASSIVE_START });
    slink = SmFindSLink(addr); // 由于状态机可能会释放slink，因此需要重新查询
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Find or create slink failed on recv pkg.");

    /* Step 3: 将报文输入分发器 */
    if (size > 0 && opcode == slink->expectOpCode) {
        /* 由于Step 2状态机处理SM_PASSIVE_START事件时, 可能会失败, 所以这里需要释放slink。 */
        if (slink->lcid == CM_INVALID_LCID) {
            NLSTK_LOG_ERROR("[SM] Bind slink %s with logic link failed.", GET_ENC_ADDR(&slink->rmtAddr));
            STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_INTERNAL_ERROR });
            return;
        }
        /* 假如绑定成功, 则不释放slink, 执行正常流程 */
        SmSLinkWaitDelTimer(slink);
        slink->dispatcher(opcode, slink, pkg, size);    // if opCode matched, dispatcher wouldn't be NULL
        return;
    }

    NLSTK_LOG_ERROR("[SM] Unexpected size or opcode in pkg dispatcher, size = %d, opcode = 0x%X.", size, opcode);
    if (opcode == SM_PAIR_FAIL_MESSAGE_OPCODE) {
        uint8_t errCode = *pkg;
        NLSTK_LOG_ERROR("[SM] Recv remote device pairing fail message, opcode: 0x%X.", errCode);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_EXTERNAL_ERROR, .extData = (void *)(uintptr_t)errCode });
    } else if (opcode == SM_NEGO_PAIRING_REQUEST) {
        NLSTK_LOG_ERROR("[SM] Find remote device key missing.");
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_MISS]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_MISS_HANDLE });
        NLSTK_CHECK_RETURN_VOID(slink->dispatcher != NULL, "[SM] slink dispatcher is null.");
        slink->dispatcher(opcode, slink, pkg, size);
    } else {
        if (opcode != SM_NEGO_PAIRING_START) {
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_EXTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        }
    }
}

/*****************************************************************************************
                            uapi interface functions
*****************************************************************************************/

void SmStartPairing(void *addrIn)
{
    NLSTK_LOG_INFO("[SM] Start pairing.");
    SLE_Addr_S *addr = (SLE_Addr_S *)addrIn;
    SmSLink_S *slink = SmFindOrCreateSLink(addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Find or create slink failed on start pairing.");
        SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &(NLSTK_SmAuthComplete_S) {
            .addr = *addr, .authStatus = SM_PAIR_ERROR });
        return;
    }
    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ACTIVE_START });
}

void SmRemovePairing(void *addrIn)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)addrIn;
    SmSLink_S *slink = SmFindSLink(addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] No need to remove pairing, since slink is not found, addr: %s.", GET_ENC_ADDR(addr));
        SmExternalCbks(SM_CBK_EVENT_PAIRING_REMOVE, &(NLSTK_SmPairingRemove_S) {
            .lcid = CM_INVALID_LCID, .addr = *addr, .removeStatus = SM_PAIR_OK });
    } else {
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
    }
}

void SmSetConfirm(void *addrIn)
{
    NLSTK_LOG_INFO("[SM] Start to set confirm.");
    SLE_Addr_S *addr = (SLE_Addr_S *)addrIn;
    SmSLink_S *slink = SmFindSLink(addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Find slink failed on confirm number comparison.");
        SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &(NLSTK_SmAuthComplete_S) {
            .addr = *addr, .authStatus = SM_PAIR_ERROR });
        return;
    }
    SmSLinkWaitDelUapiTimer(slink);
    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    if (slink->negoParams.authMethod == SM_AUTH_NUMBER_COMPARE) {
        SmNumCmpContinueNumComparison(slink);
    } else if (slink->negoParams.authMethod == SM_AUTH_NO_ENTRY) {
        SmNoEntryContinueNoentry(slink);
    } else {
        NLSTK_LOG_ERROR("[SM] Auth method doesn't match confirm set.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
}

void SmSetPassCode(void *passCodeIn)
{
    NLSTK_CHECK_RETURN_VOID(passCodeIn != NULL, "[SM] Passcode null pointer error.");
    NLSTK_LOG_INFO("[SM] Set passcode.");
    NLSTK_SmPassCode_S *passCode = (NLSTK_SmPassCode_S *)passCodeIn;
    SLE_Addr_S addr = passCode->addr;
    SmSLink_S *slink = SmFindSLink(&addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Find slink failed on passcode setting.");
        SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &(NLSTK_SmAuthComplete_S) {
            .addr = addr, .authStatus = SM_PAIR_ERROR });
        return;
    }
    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    SmSLinkWaitDelUapiTimer(slink);
    if (slink->role == SM_G_NODE) {
        slink->gNode.passCode = passCode->passCode;
        SmPassCodeSetGPassCode(slink);
    } else {
        slink->tNode.passCode = passCode->passCode;
        SmPassCodeSetTPassCode(slink);
    }
}

void SmSetPassWord(void *passWordIn)
{
    NLSTK_CHECK_RETURN_VOID(passWordIn != NULL, "[SM] Password null pointer error.");
    NLSTK_LOG_INFO("[SM] Set password.");
    NLSTK_SmPassWord_S *passWord = (NLSTK_SmPassWord_S *)passWordIn;
    SLE_Addr_S addr = passWord->addr;
    SmSLink_S *slink = SmFindSLink(&addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Find slink failed on password setting.");
        SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &(NLSTK_SmAuthComplete_S) {
            .addr = addr, .authStatus = SM_PAIR_ERROR });
        return;
    }
    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    SmSLinkWaitDelUapiTimer(slink);
    if (slink->role == SM_G_NODE) {
        SmPassWordSetGPassWord(slink, passWord);
    } else {
        SmPassWordSetTPassWord(slink, passWord);
    }
    (void)memset_s(passWord->passWord, passWord->passWordLen, 0, passWord->passWordLen);
}

void SmSetLocalPsk(void *pskIn)
{
    NLSTK_CHECK_RETURN_VOID(pskIn != NULL, "[SM] Psk null pointer error.");
    NLSTK_LOG_INFO("[SM] Set local psk.");
    NLSTK_SmPsk_S *psk = (NLSTK_SmPsk_S *)pskIn;
    SLE_Addr_S addr = psk->addr;
    SmSLink_S *slink = SmFindOrCreateSLink(&addr);
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Find or create slink failed on psk setting.");
        SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &(NLSTK_SmAuthComplete_S) {
            .addr = addr, .authStatus = SM_PAIR_ERROR });
        return;
    }
    NLSTK_LOG_INFO("[SM] Find slink with addr = %s, lcid = %d.", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    (void)memcpy_s(slink->psk, SM_PSK_SEC_KEY_LEN, psk->psk, SM_PSK_SEC_KEY_LEN);
}

void SmRecoverKey(void *recoverKeyListIn)
{
    NLSTK_CHECK_RETURN_VOID(recoverKeyListIn != NULL, "[SM] Recover key list is null.");
    SmSLink_S *slink = NULL;
    NLSTK_SmRecoverKeyParamVector_S *recoverKeyList = (NLSTK_SmRecoverKeyParamVector_S *)recoverKeyListIn;
    NLSTK_SmRecoverKeyParam_S *recoverKey = recoverKeyList->smKey;
    uint8_t keyNum = recoverKeyList->num;

    for (int i = 0; i < keyNum; i++) {
        NLSTK_LOG_INFO("[SM] Recover key, idx: %d, addr: %s.", i, GET_ENC_ADDR(&recoverKey[i].addr));
        slink = SmFindOrCreateSLink(&recoverKey[i].addr);
        if (slink == NULL) {
            NLSTK_LOG_ERROR("[SM] Recover key failed since find or create slink error.");
            continue;
        }
        slink->rmtAddr.type = recoverKey[i].addr.type;
        (void)memcpy_s(slink->rmtAddr.addr, SLE_ADDR_LEN, recoverKey[i].addr.addr, SLE_ADDR_LEN);
        (void)memcpy_s(slink->linkKey, SM_LINK_KEY_LEN, recoverKey[i].linkKey, SM_LINK_KEY_LEN);
        slink->negoParams.codeAlgoCap[SM_ENC_ALGO_ABILITY] = recoverKey[i].cryptoAlgo;
        slink->negoParams.codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = recoverKey[i].cryptoAlgo;
        slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = recoverKey[i].keyDerivAlgo;
        slink->encIntgCheck = recoverKey[i].intgChkInd;
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_RECOVER_START });
    }
}

void SmSetSecurityParams(void *paramsIn)
{
    NLSTK_CHECK_RETURN_VOID(paramsIn != NULL, "[SM] Set security params param is null.");
    NLSTK_SmLocalParams_S *params = (NLSTK_SmLocalParams_S *)paramsIn;
    NLSTK_CHECK_RETURN_VOID(g_localParams != NULL, "[SM] Global local param is null, SM not init.");
    g_localParams->ioAbility = params->ioAbility;
    g_localParams->authMethodMask = params->authMethodMask;
    g_localParams->oobDataFlag = params->oobDataFlag;
    g_localParams->secKeyMaxLen = params->secKeyMaxLen;
    g_localParams->distIrkFlag = params->distIrkFlag;
    g_localParams->distAddrFlag = params->distAddrFlag;
    g_localParams->codeAlgoCap[SM_ENC_ALGO_ABILITY] = params->codeAlgoCap[SM_ENC_ALGO_ABILITY];
    g_localParams->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = params->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY];
    g_localParams->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = params->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY];
    g_localParams->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = params->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    g_localParams->authReq.secAttribute = params->authReq.secAttribute;
    g_localParams->authReq.mitmDefend = params->authReq.mitmDefend;
    g_localParams->authReq.kpressNotif = params->authReq.kpressNotif;
    g_localParams->pskFlag = params->pskFlag;
    NLSTK_LOG_INFO("[SM] Set security params succeed.");
}

/*****************************************************************************************
                            Callbacks and Message Process
*****************************************************************************************/

void SmRegExternalCbks(NLSTK_SmCallbacks_S *cbksIn)
{
    g_smCbkTbl.startCbk = cbksIn->startCbk;
    g_smCbkTbl.removeCbk = cbksIn->removeCbk;
    g_smCbkTbl.authCbk = cbksIn->authCbk;
    g_smCbkTbl.requestCbk = cbksIn->requestCbk;
    g_smCbkTbl.encCbk = cbksIn->encCbk;
    g_smCbkTbl.imgMsgCbk = cbksIn->imgMsgCbk;
}

void SmRegImgCbks(NLSTK_SmImgCallbacks_S *cbksIn)
{
    g_smImgCbk.imgEncpCbk = cbksIn->imgEncpCbk;
}

void SmExternalCbks(uint8_t event, void *params)
{
    NLSTK_LOG_INFO("[SM] External callbacks event = 0x%u", event);
    if (event >= SM_INVALID_EVENT) {
        NLSTK_LOG_WARN("[SM] Event out of range: 0x%u", event);
        return;
    }
    switch (event) {
        case SM_CBK_EVENT_PAIRING_START:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.startCbk != NULL, "[SM] Pairing start callback is null.");
            g_smCbkTbl.startCbk((NLSTK_SmPairingStart_S *)params);
            break;
        case SM_CBK_EVENT_PAIRING_REMOVE:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.removeCbk != NULL, "[SM] Pairing remove callback is null.");
            g_smCbkTbl.removeCbk((NLSTK_SmPairingRemove_S *)params);
            break;
        case SM_CBK_EVENT_PAIRING_REQUEST:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.requestCbk != NULL, "[SM] Pairing request callback is null.");
            g_smCbkTbl.requestCbk((NLSTK_SmPairingRequest_S *)params);
            break;
        case SM_CBK_EVENT_AUTH_COMPLETE:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.authCbk != NULL, "[SM] Auth complete callback is null.");
            g_smCbkTbl.authCbk((NLSTK_SmAuthComplete_S *)params);
            break;
        case SM_CBK_EVENT_ENCRYPT_COMPLETE:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.encCbk != NULL, "[SM] Encrypt complete callback is null.");
            g_smCbkTbl.encCbk((NLSTK_SmEncComplete_S *)params);
            break;
        case SM_CBK_EVENT_IMG_SEND_MESSAGE:
            NLSTK_CHECK_RETURN_VOID(g_smCbkTbl.imgMsgCbk != NULL, "[SM] Image send message callback is null.");
            g_smCbkTbl.imgMsgCbk((NLSTK_SmSendImgMsgCmpl_S *)params);
            break;
        case SM_CBK_EVENT_IMG_ENCRYPT:
            NLSTK_CHECK_RETURN_VOID(g_smImgCbk.imgEncpCbk != NULL, "[SM] Image encrypt callback is null.");
            g_smImgCbk.imgEncpCbk((NLSTK_SmImgEncpCmpl_S *)params);
            break;
        default:
            NLSTK_LOG_ERROR("[SM] Invalid event in external callbacks.");
    }
}

void SmRemoveSLink(void *addrIn)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)addrIn;
    NLSTK_LOG_INFO("[SM] Remove slink with addr = %s.", GET_ENC_ADDR(addr));
    if (!SDF_MapErase(g_slinkMap, addr)) {
        NLSTK_LOG_ERROR("[SM] SDF_MapErase slink map by addr failed");
        return;
    }
}

bool SmSendMessage(SmSLink_S *slink, uint16_t opcode, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_INFO("[SM] Send message: targetLcid: %u, opcode, 0x%x, size: %u.", slink->lcid, opcode, size);
    DLI_ControllerData *data = SDF_MemZalloc(sizeof(DLI_ControllerData) + size); // 在dli设置完controller data后释放
    if (data == NULL) {
        NLSTK_LOG_ERROR("[SM] Send message data malloc error.");
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_SEND_MSG_ERR);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return false;
    }
    data->connHandle = slink->lcid;
    data->opcode = opcode;
    data->dataLength = (uint8_t)size;
    (void)memcpy_s(data->dataBuffer, data->dataLength, pkg, size);
    uint32_t ret = DLI_SetControllerData(data);
    if (ret != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send Controller data failure.");
        SDF_MemFree(data);
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_SEND_MSG_ERR);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return false;
    }
    SDF_MemFree(data);
    return true;
}

static void SM_SendControllerDataCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *param)
{
    SDF_UNUSED(context);
    NLSTK_CHECK_RETURN_VOID(param != NULL && param->eventParameter != NULL, "[SM] Send controller data error.");
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)param->eventParameter;
    uint16_t lcid = buff->connHandle;
    uint16_t dataIdx = buff->ctrlDataIndex;
    if (status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Send controller data error, status: %u.", status);
        CM_LogicLink_S logicLink = {0};
        if (CM_GetLogicLinkByLcid(lcid, &logicLink) != CM_SUCCESS) {
            NLSTK_LOG_ERROR("[SM] State machine process error, get logic link by lcid fail.");
            return;
        }
        SmSLink_S *slink = SmFindSLink(&logicLink.addr);
        NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Find slink by logic link failed.");
        SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, status);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    NLSTK_LOG_INFO("[SM] Send controller data succeed, lcid: %u, dataIdx: 0x%x.", lcid, dataIdx);
}

static void SM_RecvControllerDataCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *param)
{
    SDF_UNUSED(context);
    SDF_UNUSED(status);
    NLSTK_CHECK_RETURN_VOID(param != NULL && param->eventParameter != NULL, "[SM] Recv controller data error.");
    NLSTK_CHECK_RETURN_VOID(param->size >= sizeof(DLI_ControllerDataEvt), "[SM] controller data size error.");
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)param->eventParameter;
    NLSTK_CHECK_RETURN_VOID(buff->len <= param->size - sizeof(DLI_ControllerDataEvt), "[SM] buff len error.");
    uint16_t opcode = buff->ctrlDataIndex;
    NLSTK_LOG_INFO("[SM] Recv data, lcid: %u, len: %u, opcode: 0x%x.", buff->connHandle, buff->len, opcode);

    CM_LogicLink_S logicLink = {0};
    NLSTK_CHECK_RETURN_VOID(CM_GetLogicLinkByLcid(buff->connHandle, &logicLink) == CM_SUCCESS,
        "[SM] Pkg dispatch error, get logic link by lcid fail.");

    NLSTK_LOG_DEBUG("[SM] Recv data, addr: %s, data: %s.",
        GET_ENC_ADDR(&logicLink.addr), SDF_GET_UINT8_STR(buff->data, buff->len));
    SmPkgDispatcher(opcode, &logicLink.addr, buff->data, buff->len);
}

static void ImgEncryptChange(DLI_EncryptChangeEvt *param)
{
    NLSTK_LOG_INFO("[SM] Img encrypt change, img handle: 0x%04x, status: 0x%x, encryptChange: 0x%x.",
        param->connHandle, param->status, param->encryptChange);
    NLSTK_SmImgEncpCmpl_S cmpl = {0};
    cmpl.imgHandle = param->connHandle;
    cmpl.encpStatus = param->status == DLI_SUCCESS ? SM_IMG_OK : SM_IMG_ERROR;
    SmExternalCbks(SM_CBK_EVENT_IMG_ENCRYPT, &cmpl);
}

static void SM_EncryptChangeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL,
                              "[SM] Encrypt change callback param error.");
    DLI_EncryptChangeEvt *evtParam = (DLI_EncryptChangeEvt *)cmdRes->eventParameter;
    NLSTK_CHECK_RETURN_VOID(evtParam != NULL, "[SM] Encrypt change param is null.");
    NLSTK_LOG_INFO("[SM] Encrypt change, lcid: 0x%04x, status: 0x%x, encryptChange: 0x%x.",
                 evtParam->connHandle, evtParam->status, evtParam->encryptChange);
    CM_LogicLink_S logicLink = {0};
    uint32_t ret = CM_GetLogicLinkByLcid(evtParam->connHandle, &logicLink);
    if (ret != CM_SUCCESS) {
        NLSTK_LOG_INFO("[SM] Not find logic link, maybe img handle.");
        ImgEncryptChange(evtParam);
        return;
    }
    SmSLink_S *slink = SmFindSLink(&logicLink.addr);
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Find slink failed on encrypt change, addr: %s.",
                              GET_ENC_ADDR(&logicLink.addr));
    if (status != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Error 0x%X occurred in encryption.", status);
        if (status == DLI_PIN_OR_KEY_MISSING) {
            SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, evtParam->status);
            NLSTK_LOG_ERROR("[SM] Encryption fail by key missing.");
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_ENCP_FAIL, .extData = (void *)(uintptr_t)(evtParam->status) });
            return;
        } else {
            STM_MFUNC(slink->stm, ProcessMessage, (Message) {
                .what = SM_EXTERNAL_ERROR, .extData = (void *)(uintptr_t)(evtParam->status) });
            return;
        }
    }
    if (evtParam->status == SM_PAIR_OK) {
        NLSTK_LOG_INFO("[SM] Encryption succeed.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_SUCCESS });
    }
}

static void SM_EncryptParamReqCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    SDF_UNUSED(status);
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL, "Encrypt param req callback error.");
    DLI_EncryptParamReqEvt *evtParam = (DLI_EncryptParamReqEvt *)cmdRes->eventParameter;
    NLSTK_CHECK_RETURN_VOID(evtParam != NULL, "[SM] Encrypt request param is null.");
    NLSTK_LOG_INFO("[SM] Encryption parameter request, lcid: 0x%04x", evtParam->connHandle);
    CM_LogicLink_S logicLink = {0};
    CM_GetLogicLinkByLcid(evtParam->connHandle, &logicLink);
    SmSLink_S *slink = SmFindOrCreateSLink(&logicLink.addr);
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Find slink failed on encrypt param request, addr: %s.",
                              GET_ENC_ADDR(&logicLink.addr));
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
}

static void SM_EncryptParamReqReplyCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    SDF_UNUSED(context);
    NLSTK_CHECK_RETURN_VOID(cmdRes != NULL && cmdRes->eventParameter != NULL, "Encrypt param reply callback error.");
    NLSTK_LOG_INFO("[SM] Encryption parameter request reply : %s, opcode = %u.",
                 status == DLI_SUCCESS ? "success" : "false", cmdRes->cmdOpcode);
}

static const DLI_CbkLineStru g_smCbkTable[] = {
    {DLI_CBK_SEND_CONTROLLER_DATA, (void *)SM_SendControllerDataCbk},
    {DLI_CBK_RECV_CONTROLLER_DATA, (void *)SM_RecvControllerDataCbk},
    {DLI_CBK_ENCRYPT_CHANGE, (void *)SM_EncryptChangeCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ, (void *)SM_EncryptParamReqCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ_REPLY, (void *)SM_EncryptParamReqReplyCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ_NEG_REPLY, (void *)SM_EncryptParamReqReplyCbk},
};

static bool RegDliCbk(void)
{
    uint32_t ret = DLI_CmdCbkReg(SM, NULL, 0,
        g_smCbkTable, sizeof(g_smCbkTable) / sizeof(DLI_CbkLineStru));
    NLSTK_CHECK_RETURN(ret == DLI_SUCCESS, false, "[SM] Dli register callbacks failure with ret = %d.", ret);
    return true;
}

static void UnRegDliCbk(void)
{
    DLI_CmdCbkUnReg(SM, NULL, 0, g_smCbkTable, sizeof(g_smCbkTable) / sizeof(DLI_CbkLineStru));
}

static void SM_LogicLinkListener(CM_LogicLinkState_S *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SM] Logic link listener param nullptr.");
    if (param->result == CM_LINK_STATE_DISCONNECTED) {
        NLSTK_LOG_ERROR("[SM] Link Disconnected, lcid: %u, addr: %s.", param->lcid, GET_ENC_ADDR(&param->addr));
        SmSLink_S *slink = SmFindSLink(&param->addr);
        NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Logic link listener param invalid.");
        // clear expired lcid
        slink->lcid = CM_INVALID_LCID;
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_LCHANNEL_DISCONN, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
    }
}

static uint32_t RegLogicLinkListener(void)
{
    uint32_t ret = SM_OK;
    CM_LogicLinkCbks_S cbks = {0};
    cbks.moduleId = CM_MODULE_SM;
    cbks.logicLinkCbk = SM_LogicLinkListener;
    if (CM_RegLogicLinkListener(&cbks) != CM_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] CM Register local link listener fail.");
        ret = SM_REGISTER_CM_LISTENER_FAIL;
    }
    return ret;
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

static bool SLinkLocalParamsInit(void)
{
    g_localParams = (NLSTK_SmLocalParams_S *)SDF_MemZalloc(sizeof(NLSTK_SmLocalParams_S)); // SmDeInit 会释放该内存
    NLSTK_CHECK_RETURN((g_localParams != NULL), false, "[SM] Local Params initiation failed.");

    /* 设置本端安全管理模块参数 */
    g_localParams->ioAbility = SM_IO_KEYBOARD_DISPLAY;
    g_localParams->oobDataFlag = SM_WITHOUT_OOB_DATA;
    g_localParams->secKeyMaxLen = SM_MAX_KEY_DEFAULT_LEN;
    g_localParams->distIrkFlag = 0;
    g_localParams->distAddrFlag = 0;
    g_localParams->pskFlag = SM_PSK_OFF;
    g_localParams->authReq.secAttribute = SM_PAIRING_MODE_BINDING;
    g_localParams->authReq.mitmDefend = SM_MITM_DEFEND_SUPPORT;
    g_localParams->authReq.kpressNotif = SM_KEYPRESS_NOTIF_NOT_USED;
    g_localParams->authMethodMask = SM_AUTH_DEFAULT_MASK;
    return true;
}

SmSLink_S *SmFindSLink(SLE_Addr_S *addr)
{
    SDF_MapIter *iter = SDF_MapFind(g_slinkMap, addr);
    if (iter != NULL) {
        return iter->val;
    }
    return NULL;
}

SmSLink_S *SmFindOrCreateSLink(SLE_Addr_S *addr)
{
    SDF_MapIter *iter = SDF_MapFind(g_slinkMap, addr);
    if (iter != NULL) {
        return iter->val;
    }

    /* 在下方调用SDF_MapMoveInsert时将所有权转移给slink map里的node节点,
       而后在SDF_MapErase移除node节点时释放 */
    SLE_Addr_S *newAddr = SDF_MemZalloc(sizeof(SLE_Addr_S));
    if (newAddr == NULL) {
        NLSTK_LOG_ERROR("[SM] Construct newAddr failed");
        return NULL;
    }
    *newAddr = *addr; // newAddr for map emplace.

    SmSLink_S *slink = SmSLinkCtor(addr); // copy addr inside.
    if (slink == NULL) {
        NLSTK_LOG_ERROR("[SM] Construct slink failed");
        SDF_MemFree(newAddr);
        return NULL;
    }

    if (!SDF_MapMoveInsert(g_slinkMap, newAddr, slink)) {
        SDF_MemFree(newAddr);
        SmSLinkDtor((void *)slink);
        return NULL;
    }
    return slink;
}

SmSLink_S *SmFindSLinkByLcid(uint16_t lcid)
{
    CM_LogicLink_S logicLink;
    uint32_t ret = CM_GetLogicLinkByLcid(lcid, &logicLink);
    NLSTK_CHECK_RETURN(ret == CM_SUCCESS, NULL, "[SM] Cannot find logic link by lcid, lcid: %u.", lcid);
    return SmFindSLink(&logicLink.addr);
}

NLSTK_SmLocalParams_S *SmGetLocalParams(void)
{
    return g_localParams;
}