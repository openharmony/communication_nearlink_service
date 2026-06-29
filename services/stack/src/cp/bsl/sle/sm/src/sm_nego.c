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
/*
 * 本文件处理协商阶段的报文，opcode 为 0x1330 - 0x137.
 * 星闪协议规定的鉴权方式选择策略和密码学算法选择策略同样在此实现.
 */

#include "securec.h"
#include "sdf_addr.h"
#include "nlstk_log.h"
#include "cm_logic_link_api.h"
#include "cm_errno.h"

#include "sm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "sm_algos.h"
#include "sm_dft.h"
#include "sm_nego.h"

static void SetPairingReqRspMsg(SmPairReqRspMsg_S *msg);
static uint32_t PairingNego(SmPairReqRspMsg_S *msg, uint8_t gMitmDefend, uint8_t tMitmDefend, SmSLink_S *slink);

static void SendPairingStart(SmSLink_S *slink);
static void RecvPairingStart(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void SendPairingReq(SmSLink_S *slink);
static void RecvPairingReq(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void SendPairingRsp(SmSLink_S *slink);
static void RecvPairingRsp(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void SendPairingCfm(SmSLink_S *slink);
static void RecvPairingCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size);
static void SendPairingInitInfo(SmSLink_S *slink);
static void RecvPairingInitInfo(SmSLink_S *slink, const uint8_t *pkg, size_t size);

void SmNegoPkgDispatcher(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    /* 检查opcode是否属于Nego处理的范畴, 如果不是则直接退出 */
    switch (opcode) {
        case SM_NEGO_PAIRING_START: {
            RecvPairingStart(slink, pkg, size);        /* G节点，收到T发送的pairing start命令 */
            break;
        }
        case SM_NEGO_PAIRING_REQUEST: {
            RecvPairingReq(slink, pkg, size);          /* T节点，收到G发送的pairing request命令 */
            break;
        }
        case SM_NEGO_PAIRING_RESPONSE: {
            RecvPairingRsp(slink, pkg, size);          /* G节点，收到T发送的pairing response命令 */
            break;
        }
        case SM_NEGO_PAIRING_CONFIRM: {
            RecvPairingCfm(slink, pkg, size);          /* T节点，收到G发送的pairing confirm命令 */
            break;
        }
        case SM_NEGO_PAIRING_INIT_INFO: {
            RecvPairingInitInfo(slink, pkg, size);     /* G节点，收到T发送的pairing init info命令 */
            break;
        }
        default:
            NLSTK_LOG_ERROR("[SM] Nego pkg dispatcher get unexpected opcode: %u.", opcode);
            break;
    }
}

/*****************************************************************************************
                                Pairing Message Procedure
*****************************************************************************************/

void SmNegoStart(SmSLink_S *slink)
{
    if (slink->role == SM_G_NODE) {        /* G节点，先发送pairing request命令 */
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_START_PAIR_TIME);
        SendPairingReq(slink);
    } else {                               /* T节点，先发送pairing start命令 */
        SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP, SM_DFT_T_NEGO_START_PAIR_TIME);
        SendPairingStart(slink);
    }
}

static void SendPairingStart(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to send pairing start.");
    SmPairStartMsg_S msg = {0};
    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN_VOID(localParams != NULL, "[SM] Local params null pointer");
    (void)memcpy_s(&(msg.authReq), sizeof(NLSTK_SmAuthReq_S), &(localParams->authReq), sizeof(NLSTK_SmAuthReq_S));
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP, SM_DFT_T_NEGO_SEND_PAIR_START_TIME);
    bool ret = SmSendMessage(slink, SM_NEGO_PAIRING_START, (const uint8_t *)&msg, sizeof(SmPairStartMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_NEGO_PAIRING_REQUEST, SM_RECV_TIMEOUT_TIME);
}

static void RecvPairingStart(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] Start to recv pairing start.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmPairStartMsg_S), "[SM] Pairing start recv pkg size(%d) error.", size);
    SmPairStartMsg_S *msg = (SmPairStartMsg_S *)pkg;
    (void)msg;
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_RECV_PAIR_START_TIME);
    SendPairingReq(slink);
}

static void SendPairingReq(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to send pairing request.");
    SmPairReqRspMsg_S msg = {0};
    SetPairingReqRspMsg(&msg);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_SEND_PAIR_REQ_TIME);
    bool ret = SmSendMessage(slink, SM_NEGO_PAIRING_REQUEST, (const uint8_t *)&msg, sizeof(SmPairReqRspMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_NEGO_PAIRING_RESPONSE, SM_RECV_TIMEOUT_TIME);
}

static void RecvPairingReq(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] Start to recv pairing request.");
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Pairing req slink is null.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmPairReqRspMsg_S), "[SM] Pairing req recv pkg size(%d) error.", size);
    SmPairReqRspMsg_S *msg = (SmPairReqRspMsg_S *)pkg;

    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN_VOID(localParams != NULL, "[SM] Local params null pointer");

    uint8_t gMitmDefend = msg->authReq.mitmDefend;
    uint8_t tMitmDefend = localParams->authReq.mitmDefend;

    slink->gNode.ioAbility = msg->ioAbility;
    slink->gNode.pskFlag = msg->pskFlag;
    slink->tNode.ioAbility = localParams->ioAbility;
    slink->tNode.pskFlag = localParams->pskFlag;
    uint32_t ret = PairingNego(msg, gMitmDefend, tMitmDefend, slink);
    if (ret != SM_OK) {
        NLSTK_LOG_ERROR("[SM] Pairing negotiation fail.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_AUTHENTICATION_REQUIREMENTS });
        return;
    }
    NLSTK_SmPairingStart_S params = {0};
    params.bondStatus = slink->negoParams.secAttribute;
    params.lcid = slink->lcid;
    params.addr.type = slink->rmtAddr.type;
    (void)memcpy_s(params.addr.addr, SLE_ADDR_LEN, slink->rmtAddr.addr, SLE_ADDR_LEN);
    SmExternalCbks(SM_CBK_EVENT_PAIRING_START, &params);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP, SM_DFT_T_NEGO_RECV_PAIR_REQ_TIME);
    SendPairingRsp(slink);
}

static void SendPairingRsp(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to send pairing response.");
    SmPairReqRspMsg_S msg = {0};
    SetPairingReqRspMsg(&msg);
    bool ret = SmSendMessage(slink, SM_NEGO_PAIRING_RESPONSE, (const uint8_t *)&msg, sizeof(SmPairReqRspMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_NEGO_PAIRING_CONFIRM, SM_RECV_TIMEOUT_TIME);
}

static void RecvPairingRsp(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] Start to recv pairing response.");
    NLSTK_CHECK_RETURN_VOID(slink != NULL, "[SM] Pairing rsp slink is null.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmPairReqRspMsg_S), "[SM] Pairing rsp recv pkg size(%d) error.", size);
    SmPairReqRspMsg_S *msg = (SmPairReqRspMsg_S *)pkg;

    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN_VOID(localParams != NULL, "[SM] Local params null pointer");

    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_RECV_PAIR_RESP_TIME);
    uint8_t gMitmDefend = localParams->authReq.mitmDefend;
    uint8_t tMitmDefend = msg->authReq.mitmDefend;

    slink->gNode.ioAbility = localParams->ioAbility;
    slink->gNode.pskFlag = localParams->pskFlag;
    slink->tNode.ioAbility = msg->ioAbility;
    slink->tNode.pskFlag = msg->pskFlag;
    uint32_t ret = PairingNego(msg, gMitmDefend, tMitmDefend, slink);
    if (ret != SM_OK) {
        NLSTK_LOG_ERROR("[SM] Pairing negotiation fail.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_AUTHENTICATION_REQUIREMENTS });
        return;
    }
    NLSTK_SmPairingStart_S params = {0};
    params.bondStatus = slink->negoParams.secAttribute;
    params.lcid = slink->lcid;
    params.addr.type = slink->rmtAddr.type;
    (void)memcpy_s(params.addr.addr, SLE_ADDR_LEN, slink->rmtAddr.addr, SLE_ADDR_LEN);
    SmExternalCbks(SM_CBK_EVENT_PAIRING_START, &params);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_SEND_PAIR_CFM_TIME);
    SendPairingCfm(slink);
}

static void SendPairingCfm(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to send pairing confirm.");
    SmPairCfmMsg_S msg = {0};
    msg.secKeyLen = slink->negoParams.secKeyMaxLen;
    msg.authMethod = slink->negoParams.authMethod;
    (void)memcpy_s(msg.codeAlgoCap, SM_OCTETS_4, slink->negoParams.codeAlgoCap, SM_OCTETS_4);
    /* 基于协商算法生成本地公私钥对 */
    NLSTK_SmKeyPair_S keyPair = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY],
    };
    if (!SmGenPubPriKey(&keyPair)) {
        NLSTK_LOG_ERROR("[SM] Generate pubkey and prikey fail.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, keyPair.priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    /* 发送公钥 */
    (void)memcpy_s(msg.gNodePubKey, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    bool ret = SmSendMessage(slink, SM_NEGO_PAIRING_CONFIRM, (const uint8_t *)&msg, sizeof(SmPairCfmMsg_S));
    NLSTK_CHECK_RETURN_VOID(ret == true, "[SM] Send Message failed.");
    SmSLinkWaitExpectOpCode(slink, SM_NEGO_PAIRING_INIT_INFO, SM_RECV_TIMEOUT_TIME);
}

static void RecvPairingCfm(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] Start to recv pairing confirm.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmPairCfmMsg_S), "[SM] Pairing confirm recv pkg size(%d) error.", size);
    SmPairCfmMsg_S *msg = (SmPairCfmMsg_S *)pkg;
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP, SM_DFT_T_NEGO_RECV_PAIR_CFM_TIME);
    if (slink->negoParams.secKeyMaxLen != msg->secKeyLen) {
        NLSTK_LOG_ERROR("[SM] Security key size error.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    if (slink->negoParams.authMethod != msg->authMethod) {
        NLSTK_LOG_ERROR("[SM] Auth method not satisfy requirements.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_AUTHENTICATION_REQUIREMENTS });
        return;
    }
    for (int i = 0; i < SM_OCTETS_4; i++) {
        slink->negoParams.codeAlgoCap[i] = msg->codeAlgoCap[i];
    }
    (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, msg->gNodePubKey, SM_PUBLIC_KEY_LEN);
    SendPairingInitInfo(slink);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_T_NEGO_EXCEP, SM_DFT_T_NEGO_SEND_PAIR_INIT_TIME);
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_NEGO_SUCCESS });
}

static void SendPairingInitInfo(SmSLink_S *slink)
{
    NLSTK_LOG_DEBUG("[SM] Start to send pairing init info.");
    SmPairInitInfoMsg_S msg = {0};
    /* 基于协商算法生成本地公私钥对 */
    NLSTK_SmKeyPair_S keyPair = {
        .algo = slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY],
    };
    if (!SmGenPubPriKey(&keyPair)) {
        NLSTK_LOG_ERROR("[SM] Generate pubkey and prikey fail.");
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        return;
    }
    (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, keyPair.priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    /* 发送公钥 */
    (void)memcpy_s(msg.tNodePubKey, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    SmSendMessage(slink, SM_NEGO_PAIRING_INIT_INFO, (const uint8_t *)&msg, sizeof(SmPairInitInfoMsg_S));
}

static void RecvPairingInitInfo(SmSLink_S *slink, const uint8_t *pkg, size_t size)
{
    NLSTK_LOG_DEBUG("[SM] Start to recv pairing init info.");
    NLSTK_CHECK_RETURN_VOID(size >= sizeof(SmPairInitInfoMsg_S),
                              "[SM] Pairing init info recv pkg size(%d) error.", size);
    SmPairInitInfoMsg_S *msg = (SmPairInitInfoMsg_S *)pkg;
    (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, msg->tNodePubKey, SM_PUBLIC_KEY_LEN);
    SmDftCacheTimestamp(&slink->rmtAddr, NLSTK_DFT_EVENT_SM_G_NEGO_EXCEP, SM_DFT_G_NEGO_RECV_PAIR_INIT_TIME);
    STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_NEGO_SUCCESS });
}

/*****************************************************************************************
                                    Helper Functions
*****************************************************************************************/

static void SetPairingReqRspMsg(SmPairReqRspMsg_S *msg)
{
    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN_VOID(localParams != NULL, "[SM] Local params null pointer");
    msg->ioAbility = localParams->ioAbility;
    msg->oobDataFlag = localParams->oobDataFlag;
    msg->authReq.secAttribute = localParams->authReq.secAttribute;
    msg->authReq.mitmDefend = localParams->authReq.mitmDefend;
    msg->authReq.kpressNotif = localParams->authReq.kpressNotif;
    msg->secKeyMaxLen = localParams->secKeyMaxLen;
    msg->secInfoDis = (uint8_t)(localParams->distIrkFlag | (localParams->distAddrFlag << SM_SHIFT_BITS_1));
    msg->codeAlgoCap[SM_ENC_ALGO_ABILITY] = localParams->codeAlgoCap[SM_ENC_ALGO_ABILITY];
    msg->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = localParams->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY];
    msg->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = localParams->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY];
    msg->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = localParams->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY];
    msg->pskFlag = localParams->pskFlag;
    NLSTK_LOG_INFO("[SM] io_ability: %u, oob_flag %u, psk_flag: %u", msg->ioAbility, msg->oobDataFlag, msg->pskFlag);
}

static uint32_t PairingParamCheck(SmPairReqRspMsg_S *msg)
{
    if ((msg->ioAbility >= SM_IO_RESERVED) || (msg->pskFlag >= SM_RESERVED) ||
        (msg->oobDataFlag > SM_WITH_OOB_DATA) || (msg->secKeyMaxLen < SM_MAX_KEY_MIN_LEN) ||
        (msg->secInfoDis > (SM_SECURITY_INFORMATION_DISTRIBUTION_IRK | SM_SECURITY_INFORMATION_DISTRIBUTION_ADDR))) {
        return SM_ERR_AUTHENTICATION_REQUIREMENTS;
    }
    return SM_OK;
}

/**
 * @brief 基于G节点与T节点的IO能力选择鉴权方式
 */
static uint8_t GetAuthMethod(uint8_t gNodeIo, uint8_t tNodeIo)
{
    static uint8_t map[SM_AUTH_METHOD_MAX][SM_AUTH_METHOD_MAX] = {
        {SM_AUTH_NO_ENTRY, SM_AUTH_NO_ENTRY, SM_AUTH_PASSCODE,
            SM_AUTH_NO_ENTRY, SM_AUTH_PASSCODE},
        {SM_AUTH_NO_ENTRY, SM_AUTH_NUMBER_COMPARE, SM_AUTH_PASSCODE,
            SM_AUTH_NO_ENTRY, SM_AUTH_NUMBER_COMPARE},
        {SM_AUTH_PASSCODE, SM_AUTH_PASSCODE, SM_AUTH_PASSWORD_ENTRY,
            SM_AUTH_NO_ENTRY, SM_AUTH_PASSWORD_ENTRY},
        {SM_AUTH_NO_ENTRY, SM_AUTH_NO_ENTRY, SM_AUTH_NO_ENTRY,
            SM_AUTH_NO_ENTRY, SM_AUTH_NO_ENTRY},
        {SM_AUTH_PASSCODE, SM_AUTH_NUMBER_COMPARE, SM_AUTH_PASSWORD_ENTRY,
            SM_AUTH_NO_ENTRY, SM_AUTH_NUMBER_COMPARE},
    };
    if (gNodeIo >= SM_AUTH_METHOD_MAX || tNodeIo >= SM_AUTH_METHOD_MAX) {
        return SM_AUTH_NO_ENTRY;
    }
    return map[tNodeIo][gNodeIo];
}

static void NegoEncAlgo(SmPairReqRspMsg_S *msg, SmSLink_S *slink)
{
    NLSTK_CHECK_RETURN_VOID(msg != NULL && slink != NULL, "[SM] Negotiate encrypt params is invalid.");

    /* 如果对端密钥交互算法为0，默认置为ECDH-P256 */
    if (msg->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] == 0) {
        msg->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
        NLSTK_LOG_INFO("[SM] Set logic link device type OLD.");
        NLSTK_CHECK_RETURN_VOID(CM_SetLogicLinkDeviceType(slink->lcid, CM_DEVTYPE_OLD) == CM_SUCCESS,
            "[SM] CM_SetLogicLinkDeviceType failed.");
    }

    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN_VOID(localParams != NULL, "[SM] Local params null pointer");

    uint8_t *negoAlgo = slink->negoParams.codeAlgoCap;
    for (int8_t i = 0; i < SM_OCTETS_4; i++) {
        negoAlgo[i] = msg->codeAlgoCap[i] & localParams->codeAlgoCap[i];
    }

    /* 加密算法优先选择AES（比特位2），其次选择SM（比特位1） */
    for (int8_t i = SM_OCTETS_3; i >= 0; i--) {
        if ((negoAlgo[SM_ENC_ALGO_ABILITY] & negoAlgo[SM_INTG_PROTECT_ALGO_ABILITY] &
             negoAlgo[SM_KEY_DERIV_ALGO_ABILITY] & (1 << i)) != 0) {
            negoAlgo[SM_ENC_ALGO_ABILITY] = (uint8_t)(i + 1);
            negoAlgo[SM_INTG_PROTECT_ALGO_ABILITY] = (uint8_t)(i + 1);
            negoAlgo[SM_KEY_DERIV_ALGO_ABILITY] = (uint8_t)(i + 1);
            break;
        }
    }

    /* 密钥协商算法优先选择ECDH-P256（比特位2） */
    for (int8_t i = SM_OCTETS_3; i >= 0; i--) {
        if ((negoAlgo[SM_KEY_NEGO_ALGO_ABILITY] & (1 << i)) != 0) {
            negoAlgo[SM_KEY_NEGO_ALGO_ABILITY] = (uint8_t)(i + 1);
            break;
        }
    }

    NLSTK_LOG_INFO("[SM] Algorithm Negotiate result: %u %u %u %u.",
                 negoAlgo[SM_ENC_ALGO_ABILITY], negoAlgo[SM_INTG_PROTECT_ALGO_ABILITY],
                 negoAlgo[SM_KEY_DERIV_ALGO_ABILITY], negoAlgo[SM_KEY_NEGO_ALGO_ABILITY]);
}

static uint32_t PairingNego(SmPairReqRspMsg_S *msg, uint8_t gMitmDefend, uint8_t tMitmDefend, SmSLink_S *slink)
{
    uint32_t ret = PairingParamCheck(msg);
    NLSTK_CHECK_RETURN(ret == SM_OK, ret, "[SM] Pairing parameters check failed.");

    NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
    NLSTK_CHECK_RETURN(localParams != NULL, SM_ERR_INVALID_PARAMETERS, "[SM] Local params null pointer");

    /* 鉴权方式协商 */
    if ((slink->gNode.pskFlag & slink->tNode.pskFlag) == 1) {
        slink->negoParams.authMethod = SM_AUTH_PSK;
    } else if ((msg->oobDataFlag & localParams->oobDataFlag) == 1) {
        slink->negoParams.authMethod = SM_AUTH_OUT_OF_BAND;
    } else if ((gMitmDefend == 0) && (tMitmDefend == 0)) {
        slink->negoParams.authMethod = SM_AUTH_NO_ENTRY;
    } else {
        slink->negoParams.authMethod =
            GetAuthMethod(slink->gNode.ioAbility, slink->tNode.ioAbility);
    }

    if (((localParams->authMethodMask >> slink->negoParams.authMethod) & 0x1) == 0) {
        NLSTK_LOG_ERROR("[SM] Authentication method negotiate fail, method: 0x%u.", slink->negoParams.authMethod);
        return SM_ERR_AUTHENTICATION_REQUIREMENTS;
    }

    slink->negoParams.secAttribute = msg->authReq.secAttribute &
        localParams->authReq.secAttribute;                        /* 绑定 */
    slink->negoParams.kpressNotif = msg->authReq.kpressNotif &
        localParams->authReq.kpressNotif;                         /* 按键提示 */
    slink->negoParams.secKeyMaxLen = (msg->secKeyMaxLen < localParams->secKeyMaxLen) ?
        msg->secKeyMaxLen : localParams->secKeyMaxLen;            /* 最大密钥长度 */
    slink->negoParams.distIrkFlag = (msg->secInfoDis & 0x1) &
        localParams->distIrkFlag;                                 /* 分发IRK指示 */
    slink->negoParams.distAddrFlag = ((msg->secInfoDis >> SM_SHIFT_BITS_1) & 0x1) &
        localParams->distAddrFlag;                                /* 分发真实地址指示 */

    NLSTK_LOG_INFO("[SM] negoParams: authMethod=0x%X, secAttribute=0x%X, kpressNotif=0x%X, "
                 "secKeyMaxLen=%u, distIrkFlag=0x%X, distAddrFlag=0x%X",
                 slink->negoParams.authMethod, slink->negoParams.secAttribute,
                 slink->negoParams.kpressNotif, slink->negoParams.secKeyMaxLen,
                 slink->negoParams.distIrkFlag, slink->negoParams.distAddrFlag);
    /* 协商加密算法 */
    NegoEncAlgo(msg, slink);

    return SM_OK;
}