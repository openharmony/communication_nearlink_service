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
 * slink: secure link, 使能加密能力后的逻辑链路.
 * 每个 slink 记录一个外围设备的配对信息，该设备可能
 * 尚未配对，正在配对或已经配对.
 * 每个 slink 包含一个状态机用于控制配对阶段的转换和异常的处理.
 * 参考 sm_stm.c 获得更多信息.
 */

#include <stdlib.h> // for mocked lcid bind
#include "securec.h"

#include "sdf_mem.h"
#include "cp_worker.h"
#include "nlstk_log.h"
#include "dli.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "nlstk_cfgdb_api.h"

#include "sm.h"
#include "sm_errcode.h"
#include "sm_dft.h"
#include "sm_slink.h"
#include "sm_img.h"

static void WaitTimeoutCbk(void *slinkIn);

static void SlinkSecMemFree(SmSLink_S *slink)
{
    NLSTK_LOG_INFO("[SM] Start to free security memory.");
    (void)memset_s(slink->dhKey, SM_DHKEY_LEN, 0, SM_DHKEY_LEN);
    (void)memset_s(slink->priKey, SM_PRIVATE_KEY_LEN, 0, SM_PRIVATE_KEY_LEN);
    (void)memset_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, 0, SM_PUBLIC_KEY_LEN);
    (void)memset_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, 0, SM_PUBLIC_KEY_LEN);
    slink->gNode.passCode = 0;
    slink->tNode.passCode = 0;
    (void)memset_s(slink->psk, SM_PSK_SEC_KEY_LEN, 0, SM_PSK_SEC_KEY_LEN);
    (void)memset_s(slink->linkKey, SM_LINK_KEY_LEN, 0, SM_LINK_KEY_LEN);
    SDF_MemFree(slink);
}

void SmSLinkDtor(void *slinkIn)
{
    SmSLink_S *slink = (SmSLink_S *)slinkIn;
    if (slink != NULL) {
        SmSLinkWaitDelTimer(slink);
        SmSLinkWaitDelUapiTimer(slink);
        SmStateMachineDtor(slink->stm);
        SlinkSecMemFree(slink);
    }
}

SmSLink_S *SmSLinkCtor(const SLE_Addr_S *rmtAddr)
{
    SmSLink_S *slink = SDF_MemZalloc(sizeof(SmSLink_S)); // 在slink dtor里释放
    if (slink == NULL) {
        goto FAIL_LABEL;
    }

    SLE_Addr_S localAddr = {0};
    uint32_t ret = NLSTK_CfgdbGetPublicAddress(&localAddr);
    if (ret != NLSTK_OK) {
        goto FAIL_LABEL;
    }
    NLSTK_LOG_INFO("[SM] Local addr is %s", GET_ENC_ADDR(&localAddr));

    *slink = (SmSLink_S) {
        .rmtAddr = *rmtAddr,
        .localAddr = localAddr,
        .lcid = CM_INVALID_LCID,
        .role = SM_G_NODE,
        .stm = NULL,
        .dispatcher = NULL,
        .expectOpCode = 0,
        .gNode.recvFlag = false,
        .tNode.recvFlag = false,
        .timerHandle = TIMER_NO_USED_VALUE,
        .uapiTimerHandle = TIMER_NO_USED_VALUE,
    };

    SmStateMachine_S *stm = SmStateMachineCtor(slink);
    if (stm == NULL) {
        goto FAIL_LABEL;
    }

    slink->stm = stm;
    return slink;

FAIL_LABEL:
    SmSLinkDtor((void *)slink);
    return NULL;
}

bool SmSLinkBindLogicLink(SmSLink_S *slink)
{
    CM_LogicLink_S logicLink = {0};
    if (CM_GetLogicLinkByAddr(&slink->rmtAddr, &logicLink) != CM_SUCCESS) {
        NLSTK_LOG_ERROR("[SM] Get logic link by address fail.");
        return false;
    }
    slink->lcid = logicLink.lcid;
    slink->role = logicLink.role;
    NLSTK_LOG_INFO("[SM] Bind slink %s with logic link lcid: %u", GET_ENC_ADDR(&slink->rmtAddr), slink->lcid);
    return true;
}

/*
 *  在收到控制报文后，slink 会执行以下操作.
 *      1. 停止之前的定时器.
 *      2. 回复控制报文.
 *      3. 调用 SmSLinkWaitExpectOpCode() 以改变 expectOpCode,
 *         并重启定时器.
 *
 * 由于定时器机制实现的限制, 每次启动定时器均会创建新的定时器句柄，
 * 因此，定时器资源应该被恰当的释放，通过手动删除或依赖 slink 析构流程.
 */
bool SmSLinkWaitExpectOpCode(SmSLink_S *slink, uint16_t expectOpCode, time_t timeout)
{
    NLSTK_CHECK_RETURN(slink->timerHandle == TIMER_NO_USED_VALUE, false, "[SM] The requested timerHandle is in use.");
    slink->expectOpCode = expectOpCode;

    SDF_TimerParam param = {
        .expires = timeout,
        .period = false,
        .callback = WaitTimeoutCbk,
        .args = slink,
    };
    return CP_TimerAdd(&slink->timerHandle, &param) == CP_OK;
}

/*
 *  等待北向用户的输入。
 *  其机制与SM_SLinkWaitExpectOpCode相同。
 *  但需要注意，不能同时使用，否则会导致资源不能被恰当释放。
 *  使用该函数时需确保前一个timerHandle已经被释放。
 */
bool SmSLinkWaitUapiInput(SmSLink_S *slink, time_t timeout)
{
    NLSTK_CHECK_RETURN(slink->uapiTimerHandle == TIMER_NO_USED_VALUE,
                         false, "[SM] The requested timerHandle is in use.");
    SDF_TimerParam param = {
        .expires = timeout,
        .period = false,
        .callback = WaitTimeoutCbk,
        .args = slink,
    };
    return CP_TimerAdd(&slink->uapiTimerHandle, &param) == CP_OK;
}

void SmSLinkWaitDelTimer(SmSLink_S *slink)
{
    CP_TimerDel(slink->timerHandle);
    slink->timerHandle = TIMER_NO_USED_VALUE;
}

void SmSLinkWaitDelUapiTimer(SmSLink_S *slink)
{
    CP_TimerDel(slink->uapiTimerHandle);
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
}

static void WaitTimeoutCbk(void *slinkIn)
{
    NLSTK_LOG_ERROR("[SM] Timeout when wait expect opcode or input.");
    SmSLink_S *slink = (SmSLink_S *)slinkIn;
    SmDftReport(&slink->rmtAddr, slink->curStateIndex, slink->role, SM_DFT_TIMEOUT_ERR);
    STM_MFUNC(slink->stm, ProcessMessage, (Message) {
        .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
}

void SmSLinkAuthCbk(SmSLink_S *slink, uint8_t status)
{
    NLSTK_SmAuthComplete_S param = {0};
    param.addr = slink->rmtAddr;
    param.lcid = slink->lcid;
    param.authStatus = status;
    if (status == SM_PAIR_OK) {
        (void)memcpy_s(param.linkKey, SM_LINK_KEY_LEN, slink->linkKey, SM_LINK_KEY_LEN);
        param.cryptoAlgo = slink->negoParams.codeAlgoCap[SM_ENC_ALGO_ABILITY];
        param.keyDerivAlgo = slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY];
        param.intgChkInd = slink->encIntgCheck;
        SmGenerateImgSecuConfig(param.keyDerivAlgo, param.groupKey, &param.giv);
    }
    SmExternalCbks(SM_CBK_EVENT_AUTH_COMPLETE, &param);
    (void)memset_s(&param, sizeof(NLSTK_SmAuthComplete_S), 0x00, sizeof(NLSTK_SmAuthComplete_S));
}

void SmSLinkEncpCbk(SmSLink_S *slink, uint8_t status)
{
    NLSTK_SmEncComplete_S param = {0};
    param.lcid = slink->lcid;
    param.addr = slink->rmtAddr;
    param.encStatus = status;
    SmExternalCbks(SM_CBK_EVENT_ENCRYPT_COMPLETE, &param);
}

void SmSLinkRequestCbk(SmSLink_S *slink, uint8_t type, SmAuthUserCode_S code)
{
    NLSTK_SmPairingRequest_S param = {0};
    param.lcid = slink->lcid;
    param.addr = slink->rmtAddr;
    param.requestType = type;
    (void)memcpy_s(param.sixDigits, SM_OCTETS_7, code.code, SM_OCTETS_7);
    SmExternalCbks(SM_CBK_EVENT_PAIRING_REQUEST, &param);
}