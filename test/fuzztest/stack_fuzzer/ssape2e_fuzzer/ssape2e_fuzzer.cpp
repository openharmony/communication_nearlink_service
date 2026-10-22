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
#include "ssape2e_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include <string.h>
#include <sched.h>
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"
#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "sdf_vector.h"
#include "sdf_map.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "ssap_type.h"
#include "ssap_utils.h"
#include "dli_layer_callback.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_server.h"
#include "sm_slink.h"
#include "sm_stm.h"
#include "sle_logic_link_mgr.h"

extern SDF_Map *g_slinkMap;

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

/* 对端模拟闭环：同一进程内建两条 SSAP 链路（服务端/客户端），发送回调互相
 * 把出向报文投递到对端链路的 SSAP_Recv——客户端请求走服务端真实解析
 * （SSAPS_*ReqHandle 全族），服务端响应回喂客户端解析（SSAPC_*RspHandle
 * 全族），任务生命周期/opcode 期待/分片白名单全部走真实逻辑。
 * PR-4 在此底座上补：结果观测（appCallback）、真实 appId 注册、app 路径
 * 服务句柄实录、读写/方法族、权限成对输入、授权闭环、订阅对抗、多值写、
 * 交错场景、分片回喂与超时/优先级（SS-00~03/A/R1~R6/F/T）。 */

/* ---------- SS-01：观测底座。请求-响应在本进程内同步回环完成（内层
 * SSAPC_* 直驱），少数异步腿（AddService/AuthorizeResult/SendMethodCallRes
 * 经 SchedulePostTask 上 worker）由 E2eWaitObs 有界自旋等待；单飞纪律：
 * 主线程等待期间不触碰链路，无同结构并发写。 ---------- */
enum E2eReqKind {
    E2E_KNONE = 0,
    E2E_EXCHANGE,   /* arg: SSAP_ExchangeComplete_S*（栈） */
    E2E_FIND,       /* arg: SSAP_DiscoveryComplete_S*（栈） */
    E2E_FINDUUID,   /* arg: SSAP_DiscoveryComplete_S*（栈） */
    E2E_WRITE_REQ,  /* arg: SSAP_ValuePkt_S*（堆） */
    E2E_WRITE_CMD,  /* 无响应，无回调 */
    E2E_READ,       /* arg: SSAP_ValuePkt_S*（堆） */
    E2E_READ_MULTI, /* arg: SSAP_ReadByHandleComplete_S*（栈，readVals 向量） */
    E2E_READ_UUID,  /* arg: SSAP_ReadByUuidComplete_S*（栈） */
    E2E_METHOD_REQ, /* arg: SSAP_MethodResult_S*（堆） */
    E2E_METHOD_CMD, /* 无响应，无回调 */
};

struct E2eObs {
    volatile bool cbArrived;
    volatile uint8_t cbCount;
    volatile uint8_t errCode;
    volatile uint16_t handle;
    volatile uint16_t valLen;
    uint8_t valFirst[4];
    volatile uint32_t itemCount;
    volatile int32_t cbAppId;
};

static E2eObs g_obs;
static int g_curKind = E2E_KNONE;
/* 完成顺序日志（SS-T 优先级/超时核对）：只记 kind 序列 */
static uint8_t g_seqLog[8];
static uint8_t g_seqLen;

static void E2eObsReset(int kind)
{
    (void)memset_s(&g_obs, sizeof(g_obs), 0, sizeof(g_obs));
    g_curKind = kind;
}

static void E2eObsRecordValue(const uint8_t *val, uint16_t len)
{
    g_obs.valLen = len;
    for (uint16_t i = 0; i < sizeof(g_obs.valFirst) && i < len; i++) {
        g_obs.valFirst[i] = val[i];
    }
}

static void E2eTaskCallback(int32_t appId, void *arg)
{
    if (arg == nullptr) {
        return;
    }
    g_obs.cbArrived = true;
    g_obs.cbCount++;
    g_obs.cbAppId = appId;
    if (g_seqLen < sizeof(g_seqLog)) {
        g_seqLog[g_seqLen++] = (uint8_t)g_curKind;
    }
    switch (g_curKind) {
        case E2E_EXCHANGE: {
            SSAP_ExchangeComplete_S *complete = (SSAP_ExchangeComplete_S *)arg;
            g_obs.errCode = complete->errCode;
            break;
        }
        case E2E_FIND:
        case E2E_FINDUUID: {
            SSAP_DiscoveryComplete_S *complete = (SSAP_DiscoveryComplete_S *)arg;
            g_obs.errCode = complete->errCode;
            break;
        }
        case E2E_READ_MULTI: {
            SSAP_ReadByHandleComplete_S *complete = (SSAP_ReadByHandleComplete_S *)arg;
            g_obs.errCode = complete->errCode;
            if (complete->readVals != nullptr) {
                g_obs.itemCount = complete->readVals->size;
                if (complete->readVals->size > 0) {
                    SSAP_ValuePkt_S *item = (SSAP_ValuePkt_S *)SDF_VectorElementAt(complete->readVals, 0);
                    if (item != nullptr) {
                        g_obs.errCode = item->errorCode;
                        g_obs.handle = item->handle;
                        E2eObsRecordValue(item->value.value, item->value.len);
                    }
                }
            }
            break;
        }
        case E2E_READ_UUID: {
            SSAP_ReadByUuidComplete_S *complete = (SSAP_ReadByUuidComplete_S *)arg;
            g_obs.errCode = complete->errCode;
            if (complete->readVals != nullptr) {
                g_obs.itemCount = complete->readVals->size;
            }
            break;
        }
        case E2E_METHOD_REQ: {
            SSAP_MethodResult_S *result = (SSAP_MethodResult_S *)arg;
            g_obs.errCode = result->errorCode;
            g_obs.handle = result->handle;
            E2eObsRecordValue(result->value.value, result->value.len);
            break;
        }
        default: {
            /* 单读/写回/错误路径：SSAP_ValuePkt_S*（堆，回调返回后产品释放） */
            SSAP_ValuePkt_S *pkt = (SSAP_ValuePkt_S *)arg;
            g_obs.errCode = pkt->errorCode;
            g_obs.handle = pkt->handle;
            E2eObsRecordValue(pkt->value.value, pkt->value.len);
            break;
        }
    }
}

/* 有界自旋等待观测到达：同步腿在调用栈内已完成（立即通过），异步腿
 * （worker 上构建响应→回喂→回调）通常微秒级；超时返回 false 由调用方
 * 记录并跳过依赖段，不冒充成功路径。 */
static void E2eQuiesce(void);

/* 有界自旋等待观测到达：同步腿在调用栈内已完成（立即通过），异步腿
 * （worker 上构建响应→回喂→回调）通常微秒级；超时返回 false 由调用方
 * 记录并跳过依赖段，不冒充成功路径。回调到达≠级联结束——异步腿完成
 * 请求后会同步级联排空队列，主线程必须等全部链路回到 IDLE 再继续
 * （UAF 根治：PR#336 设备 ASAN 轮实锤 worker 在 DeliverToPeer 读到主
 * 线程已释放的 buff，与 PR#331 Dtap 同族——主/worker 不得并发触碰
 * SSAP 状态与分配器）。sched_yield 让核使真机等待高效。 */
static bool E2eWaitObs(void)
{
    for (volatile int32_t i = 0; i < 8000000 && !g_obs.cbArrived; i++) {
        (void)sched_yield();
    }
    if (g_obs.cbArrived) {
        E2eQuiesce();
    }
    return g_obs.cbArrived;
}

/* 不变量断言：仅用于安全约束类违例（受保值被改/错收件方/重复完成/订阅
 * 残留），违例即 trap 交由 libFuzzer 报告（蓝队最小种子口径）；普通值
 * 期望只记录不 trap，避免驱动误判污染。 */
#define E2E_INVARIANT(cond, msg)                                    \
    do {                                                            \
        if (!(cond)) {                                              \
            NAI_LOG_ERROR("[E2E-INVARIANT] %s", msg);               \
            __builtin_trap();                                       \
        }                                                           \
    } while (0)

/* ---------- SS-00/SS-02：真实注册与 app 路径服务句柄实录 ---------- */
static int32_t g_cliAppId = -1;
static int32_t g_srvAppId = -1;
static volatile bool g_appSvcCbSeen = false;
static volatile bool g_appSvcOk = false;
static uint16_t g_appSvcStart = 0;
static uint16_t g_appSvcEnd = 0;
/* 权限分层属性句柄：0 无保护/1 需认证/2 需加密/3 需授权/4 可通知(带CCCD)/5 大值 */
enum { TIER_FREE = 0, TIER_AUTHN, TIER_ENC, TIER_AUTHZ, TIER_NTF, TIER_BIG, TIER_NUM };
static uint16_t g_propHandle[TIER_NUM] = {0};
static uint16_t g_methodHandle = 0;
static volatile uint16_t g_authReqId = 0;
static volatile bool g_authReqSeen = false;
static volatile uint16_t g_methReqId = 0;
static volatile bool g_methReqSeen = false;
static volatile uint32_t g_cliNtfCount = 0;
static volatile uint32_t g_cliNtfCountB = 0;

static const uint8_t g_stdBase[SSAP_UUID_STD_BASE_OFFSET] = {
    0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00};
static NLSTK_SsapUuid_S g_tierUuid[TIER_NUM];
static NLSTK_SsapUuid_S g_methodUuid;
static NLSTK_SsapUuid_S g_appSvcUuid = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x20,
    0xEA, 0x11, 0xFC, 0x70, 0x80, 0xA8, 0xBE, 0x39};
#define E2E_BIG_VALUE_LEN 600
static uint8_t g_bigValue[E2E_BIG_VALUE_LEN];

static void E2eInitTierUuids(void)
{
    /* TIER_FREE 用 STD 形态（前 14 字节 = STD 基，末 2 字节为 UUID16）：
     * 同时驱动 SSAP_CheckUuidStd 与按 UUID 读的 STD 控制码路径；其余 CUS。 */
    (void)memset_s(g_tierUuid, sizeof(g_tierUuid), 0, sizeof(g_tierUuid));
    for (int i = 0; i < TIER_NUM; i++) {
        (void)memcpy_s(g_tierUuid[i].uuid, SSAP_UUID_STD_BASE_OFFSET, g_stdBase, SSAP_UUID_STD_BASE_OFFSET);
        g_tierUuid[i].uuid[14] = (uint8_t)(0x40 + i);
        g_tierUuid[i].uuid[15] = 0x51;
    }
    /* 认证/加密/授权/通知/大值档改 CUS（高字节脱离 STD 基） */
    for (int i = TIER_AUTHN; i < TIER_NUM; i++) {
        g_tierUuid[i].uuid[0] = 0xC0 + (uint8_t)i;
    }
    (void)memcpy_s(&g_methodUuid, sizeof(NLSTK_SsapUuid_S), &g_tierUuid[TIER_FREE],
        sizeof(NLSTK_SsapUuid_S));
    g_methodUuid.uuid[14] = 0x60;
    for (int i = 0; i < E2E_BIG_VALUE_LEN; i++) {
        g_bigValue[i] = (uint8_t)(i & 0xFF);
    }
}

static int E2eFindTierByUuid(const NLSTK_SsapUuid_S *uuid)
{
    for (int i = 0; i < TIER_NUM; i++) {
        if (memcmp(g_tierUuid[i].uuid, uuid->uuid, SSAP_UUID128_LEN) == 0) {
            return i;
        }
    }
    return -1;
}

static void SrvOnAddService(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
{
    (void)appId;
    g_appSvcCbSeen = true;
    if (ret != NLSTK_ERRCODE_SUCCESS || service == nullptr) {
        return;
    }
    g_appSvcOk = true;
    g_appSvcStart = service->handle;
    g_appSvcEnd = service->endHandle;
    if (service->properties != nullptr) {
        for (size_t i = 0; i < service->properties->size; i++) {
            SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, i);
            if (property == nullptr) {
                continue;
            }
            int tier = E2eFindTierByUuid(&property->uuid);
            if (tier >= 0 && g_propHandle[tier] == 0) {
                g_propHandle[tier] = property->handle;
            }
        }
    }
    if (service->methods != nullptr && service->methods->size > 0) {
        SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
        if (method != nullptr) {
            g_methodHandle = method->handle;
        }
    }
}

static void SrvOnWriteAuthReq(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerWritePropertyInfo_S *property)
{
    (void)appId;
    (void)property;
    g_authReqId = requestId;
    g_authReqSeen = true;
}

static void SrvOnReadAuthReq(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerReadPropertyInfo_S *property)
{
    (void)appId;
    (void)property;
    g_authReqId = requestId;
    g_authReqSeen = true;
}

static void SrvOnCallMethod(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerCallMethodRequestInfo_S *method, bool needReturn, bool needAuth)
{
    (void)needAuth;
    (void)method;
    g_methReqId = requestId;
    g_methReqSeen = true;
    if (!needReturn) {
        return;
    }
    /* 真实回结果：echo 确定字节序列（可独立核对），经产品
     * NLSTK_SsapServerSendMethodCallRes → SSAPS_SendMethodCallRes →
     * requestId 配对回 CALL_METHOD_RSP。 */
    static uint8_t echoBuf[8];
    NLSTK_VariableData_S value = {.len = sizeof(echoBuf), .data = echoBuf};
    for (size_t i = 0; i < sizeof(echoBuf); i++) {
        echoBuf[i] = (uint8_t)(0xE0 + i);
    }
    (void)NLSTK_SsapServerSendMethodCallRes(appId, requestId, &value);
}

static void CliOnPropertyChanged(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    (void)appId;
    (void)property;
    /* UAPI 通知到达（尽力而为观测；收件方正负以链路级计数为准） */
    g_cliNtfCount++;
}

static NLSTK_SsapAppServerCb_S g_srvCb;

static void E2eRegisterApps(const SLE_Addr_S *serverAddr)
{
    /* SS-00：真实注册取 appId（失败保持 -1，依赖段按缺置跳过，不冒充成功） */
    NLSTK_SsapAppClientCb_S cliCb = {};
    cliCb.onPropertyChanged = CliOnPropertyChanged;
    (void)NLSTK_SsapClientRegApp(&g_cliAppId, &cliCb, const_cast<SLE_Addr_S *>(serverAddr));

    g_srvCb = NLSTK_SsapAppServerCb_S();
    g_srvCb.onAddService = SrvOnAddService;
    g_srvCb.onWritePropertyAuthorizeRequest = SrvOnWriteAuthReq;
    g_srvCb.onReadPropertyAuthorizeRequest = SrvOnReadAuthReq;
    g_srvCb.onCallMethod = SrvOnCallMethod;
    (void)NLSTK_SsapServerRegApp(&g_srvCb, &g_srvAppId);
}

static bool E2eSetupAppService(void)
{
    /* SS-02：app 路径逐字段构造——四档权限属性 + 可通知(带 CLIENT_CONFIG
     * 描述符) + 大值 + 方法。句柄以 onAddService 回调实录为准。 */
    static uint16_t cccdValue = 0x0001;
    NLSTK_SsapServiceDescriptorParam_S descriptor = {};
    descriptor.type = DESC_TYPE_CLIENT_CONFIG;
    descriptor.operation.operationValue = SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    descriptor.val.data = (uint8_t *)&cccdValue;
    descriptor.val.len = sizeof(cccdValue);

    NLSTK_SsapServicePropertyParam_S property[TIER_NUM] = {};
    const uint8_t perm[TIER_NUM] = {SSAP_PERMISSION_NO_NEED, SSAP_PERMISSION_AUTHENTICATION_NEED,
        SSAP_PERMISSION_ENCRYPTION_NEED, SSAP_PERMISSION_AUTHORIZATION_NEED, SSAP_PERMISSION_NO_NEED,
        SSAP_PERMISSION_NO_NEED};
    for (int i = 0; i < TIER_NUM; i++) {
        property[i].type = ITEM_TYPE_STD_PROPERTY;
        (void)memcpy_s(property[i].uuid.uuid, SSAP_UUID128_LEN, g_tierUuid[i].uuid, SSAP_UUID128_LEN);
        property[i].permission.permissionValue = perm[i];
        property[i].operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE;
        property[i].val.data = g_bigValue;
        property[i].val.len = 2; /* 初值 {0x00,0x01}，BIG 档单独放大 */
        if (i == TIER_NTF) {
            property[i].operation.operationValue |= SSAP_OPERATE_INDICATION_NOTIFY;
            property[i].descriptorNum = 1;
            property[i].descriptors = &descriptor;
        }
        if (i == TIER_BIG) {
            property[i].val.len = E2E_BIG_VALUE_LEN;
        }
    }

    NLSTK_SsapServiceMethodParam_S method = {};
    method.type = ITEM_TYPE_STD_METHOD;
    (void)memcpy_s(method.uuid.uuid, SSAP_UUID128_LEN, g_methodUuid.uuid, SSAP_UUID128_LEN);
    method.permission.permissionValue = SSAP_PERMISSION_NO_NEED;

    NLSTK_ServiceParam_S service = {};
    (void)memcpy_s(service.serviceStatement.uuid.uuid, SSAP_UUID128_LEN, g_appSvcUuid.uuid,
        SSAP_UUID128_LEN);
    service.serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    service.servicePropertyNum = TIER_NUM;
    service.property = property;
    service.serviceMethodNum = 1;
    service.method = &method;

    g_appSvcCbSeen = false;
    g_appSvcOk = false;
    (void)NLSTK_SsapServerAddService(g_srvAppId, &service);
    for (volatile int32_t i = 0; i < 8000000 && !g_appSvcCbSeen; i++) {
        (void)sched_yield();
    }
    E2eQuiesce(); /* AddService 尾部 UpdateHash→ServiceChangeNtf 会在链路上排通知任务，等收尾 */
    return g_appSvcOk;
}

/* ---------- SS-R1：SM 鉴权/加密判定夹具。SmIsSLinkAuthComplete(lcid) 经
 * CM_GetLogicLinkByLcid → g_slinkMap[addr] → curStateIndex==SM_STATE_FULL
 * （nlstk_sm_api.c:248-260）。夹具 = 逻辑链路登记 + SLink 构造为 FULL 态，
 * 代表"已完成真实配对加密的链路"环境代入；轮间拆除。 ---------- */
static SleLogicLink_S *g_cmNode = nullptr;
static bool g_slinkOwned = false;

static void SlinkFixtureSet(const SLE_Addr_S *peerAddr, uint16_t lcid)
{
    SleLogicLinkInit();
    SleLogicLink_S *node = SleLogicLinkAdd(const_cast<SLE_Addr_S *>(peerAddr));
    if (node == nullptr) {
        return;
    }
    node->lcid = lcid;
    g_cmNode = node;
    if (g_slinkMap == nullptr) {
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (key == nullptr) {
            return;
        }
        (void)memcpy_s(key, sizeof(SLE_Addr_S), peerAddr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(peerAddr);
        if (slink == nullptr) {
            SDF_MemFree(key);
            return;
        }
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        if (g_slinkMap == nullptr) {
            SDF_MemFree(key);
            return;
        }
        SDF_MapMoveInsert(g_slinkMap, key, slink);
        g_slinkOwned = true;
        slink->curStateIndex = SM_STATE_FULL; /* 已配对且已加密终态 */
    }
}

static void SlinkFixtureClear(void)
{
    if (g_cmNode != nullptr) {
        SleLogicLinkRemove(g_cmNode);
        g_cmNode = nullptr;
    }
    if (g_slinkOwned && g_slinkMap != nullptr) {
        SDF_MapDtor(g_slinkMap);
        g_slinkMap = nullptr;
        g_slinkOwned = false;
    }
}

/* ---------- 双链路回环（沿用 PR-1/2 底座，扩展 B 端与观测钩子） ---------- */
static DTAP_Data_Info_S g_serverInfo = {0};
static DTAP_Data_Info_S g_clientInfo = {0};
static DTAP_Data_Info_S g_serverInfoB = {0};
static DTAP_Data_Info_S g_clientInfoB = {0};
static SSAP_Link_S *g_clientLink = nullptr;
static SSAP_Link_S *g_serverLink = nullptr;   /* 链路指针文件域化：静默屏障/统一清理需跨函数访问 */
static SSAP_Link_S *g_serverLinkB = nullptr;
static SSAP_Link_S *g_clientLinkB = nullptr;

/* 主/worker 串行化屏障：等所有在活链路回 IDLE（有界）。队列级联必终态
 * IDLE（无新请求则不再自续），故 IDLE 即 worker 侧收尾。 */
static bool E2eAllLinksIdle(void)
{
    if (g_clientLink != nullptr && g_clientLink->status != SSAP_LINK_IDLE) {
        return false;
    }
    if (g_serverLink != nullptr && g_serverLink->status != SSAP_LINK_IDLE) {
        return false;
    }
    if (g_clientLinkB != nullptr && g_clientLinkB->status != SSAP_LINK_IDLE) {
        return false;
    }
    if (g_serverLinkB != nullptr && g_serverLinkB->status != SSAP_LINK_IDLE) {
        return false;
    }
    return true;
}

static void E2eQuiesce(void)
{
    for (volatile int32_t i = 0; i < 8000000 && !E2eAllLinksIdle(); i++) {
        (void)sched_yield();
    }
}

static NLSTK_SsapUuid_S g_uuid1 = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x20,
    0xEA, 0x11, 0xFC, 0x70, 0x80, 0xA8, 0xBE, 0x37};
static NLSTK_SsapUuid_S g_uuid2 = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x20,
    0xEA, 0x11, 0xFC, 0x70, 0x80, 0xA8, 0xBE, 0x38};
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
    0x02, 0x01, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};

static uint8_t g_valueBuf[16] = {0};

/* SS-F：分片回喂模式（>0 各值）；SS-T：扣留响应模式 */
static uint8_t g_fragMode = 0;
static bool g_holdMode = false;
static SDF_Buff_S *g_holdBuff = nullptr;
static volatile uint32_t g_fragFed = 0;

/* 出向报文所有权口径（与产品单测 MockSendCb 一致）：
 * 带 REPLY_MASK 的 REQ/RSP/IND 出向报文经 SSAP_LinkSetTask 存为 curTask
 * （供重建/超时路径），CMD/NTF/ACK 由发送回调释放；
 * 投递给对端的是副本，避免跨链路所有权纠缠。 */
static void DeliverToPeer(const DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    uint32_t len = (uint32_t)SDF_DataLenGet(buff);
    if (len == 0) {
        return;
    }
    SDF_Buff_S *copy = SDF_BuffNew(len);
    if (copy == nullptr) {
        return;
    }
    uint8_t *dst = SDF_BuffAppend(copy, len);
    if (dst != nullptr) {
        (void)memcpy_s(dst, len, SDF_DataOffset(buff), len);
        (void)SSAP_Recv(const_cast<DTAP_Data_Info_S *>(info), copy);
    }
    SDF_BuffFree(copy);
}

/* SS-F：按产品片头格式切片回喂（SSAP_BuildFragBuff 为 static，格式照
 * ssap_manager.c:533-568 复刻：[opcode][baseCtrl|BEGIN/MID/END]，片载荷
 * ≤ mtu-2；合法序列走对端真实 SSAP_ReassemFragment 重组）。
 * fragErr: 1=合法 2=缺首片 3=重复尾片 4=截断 5=单片超长 */
static void FragFeedToClient(const SDF_Buff_S *buff, uint8_t fragErr)
{
    const uint8_t *src = SDF_DataOffset(const_cast<SDF_Buff_S *>(buff));
    uint32_t total = (uint32_t)SDF_DataLenGet(const_cast<SDF_Buff_S *>(buff));
    if (total <= SSAP_PDU_BASE_LEN || g_clientLink == nullptr) {
        return;
    }
    uint32_t maxPayload = (uint32_t)g_clientLink->mtu - SSAP_PDU_BASE_LEN;
    if (maxPayload == 0) {
        return;
    }
    uint32_t payloadSize = total - SSAP_PDU_BASE_LEN;
    uint32_t fragCount = (payloadSize + maxPayload - 1) / maxPayload;
    uint8_t opcode = src[0];
    uint8_t baseCtrl = src[1] & (uint8_t)~SSAP_CTRL_FRAG_MASK;
    for (uint32_t index = (fragErr == 2) ? 1 : 0; index < fragCount; index++) {
        uint8_t fragType = SSAP_CTRL_FRAG_MID;
        if (index == 0) {
            fragType = SSAP_CTRL_FRAG_BEGIN;
        } else if (index == fragCount - 1) {
            fragType = SSAP_CTRL_FRAG_END;
        }
        uint32_t offset = SSAP_PDU_BASE_LEN + index * maxPayload;
        uint32_t fragPayloadLen = (total - offset > maxPayload) ? maxPayload : (total - offset);
        uint32_t fragLen = SSAP_PDU_BASE_LEN + fragPayloadLen;
        if (fragErr == 5) {
            fragLen = (uint32_t)g_clientLink->mtu + 10; /* 单片超长矛盾 → 丢弃清上下文 */
        }
        SDF_Buff_S *fragBuff = SDF_BuffNew(fragLen);
        if (fragBuff == nullptr) {
            return;
        }
        uint8_t *dst = SDF_BuffAppend(fragBuff, fragLen);
        if (dst != nullptr) {
            dst[0] = opcode;
            dst[1] = baseCtrl | fragType;
            uint32_t copyLen = fragLen - SSAP_PDU_BASE_LEN;
            if (copyLen > 0) {
                (void)memcpy_s(dst + SSAP_PDU_BASE_LEN, copyLen, src + offset, copyLen);
            }
            (void)SSAP_Recv(&g_clientInfo, fragBuff);
            g_fragFed++;
            if (fragErr == 3 && index == fragCount - 1) {
                /* 重复尾片：END 处理完 reassemBuff 已置空，再投一片同 END 走缺首片 DROP */
                (void)SSAP_Recv(&g_clientInfo, fragBuff);
                g_fragFed++;
            }
        }
        SDF_BuffFree(fragBuff);
        if (fragErr == 4 && index + 2 >= fragCount) {
            break; /* 截断：丢弃尾片 */
        }
    }
}

static void ServerSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    if (opcode == SSAP_VALUE_NTF || opcode == SSAP_VALUE_IND) {
        g_cliNtfCount++;
    }
    if (g_holdMode) {
        /* SS-T：扣留出向报文（副本），制造在途任务 */
        if (g_holdBuff != nullptr) {
            SDF_BuffFree(g_holdBuff);
        }
        uint32_t len = (uint32_t)SDF_DataLenGet(buff);
        g_holdBuff = SDF_BuffNew(len);
        if (g_holdBuff != nullptr) {
            uint8_t *dst = SDF_BuffAppend(g_holdBuff, len);
            if (dst != nullptr) {
                (void)memcpy_s(dst, len, SDF_DataOffset(buff), len);
            }
        }
    } else if (g_fragMode != 0 && g_clientLink != nullptr &&
        SDF_DataLenGet(buff) > g_clientLink->mtu) {
        FragFeedToClient(buff, g_fragMode);
    } else {
        DeliverToPeer(&g_clientInfo, buff);
    }
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

static void ServerSendCbB(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    if (opcode == SSAP_VALUE_NTF || opcode == SSAP_VALUE_IND) {
        g_cliNtfCountB++; /* B 端收件方计数（错收件方不变量） */
    }
    DeliverToPeer(&g_clientInfoB, buff);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

static void ClientSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    DeliverToPeer(&g_serverInfo, buff);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

static void ClientSendCbB(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    DeliverToPeer(&g_serverInfoB, buff);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

static void AddService1()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_StartService(NULL);
}

static void AddService2()
{
    SSAP_ParamAddService_S *serviceParam1 =
        (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam1->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam1);
    SDF_MemFree(serviceParam1);
    SSAP_ParamAddProperty_S *propertyParam2 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 2);
    memcpy_s(&propertyParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    propertyParam2->val.len = 2;
    propertyParam2->val.value[0] = 0xAB;
    propertyParam2->val.value[1] = 0xCD;
    propertyParam2->operation.operationValue = 0x1F; /* 读|写|通知 */
    SSAP_CacheProperty(propertyParam2);
    SDF_MemFree(propertyParam2);
    SSAP_StartService(NULL);
}

/* SS-01：带观测的请求发起（appId 用真实注册值） */
static void RunRequestTask(SSAP_Link_S *link, SSAP_ProcessTaskFunc func, void *arg, int kind)
{
    SSAP_TaskParam_S taskParam = {.appId = g_cliAppId, .arg = arg, .freeFunc = SDF_MemFree,
        .func = func, .timeout = 3000, .valid = true, .appCallback = E2eTaskCallback};
    E2eObsReset(kind);
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/* SS-R5：两 item 多值写直投服务端收路径（客户端 API 无多值写，协议现状，
 * 如实记录）。item1 = 合法可写句柄；item2 = 无效句柄/服务变更句柄/需认证
 * 属性；交换顺序再投一次覆盖顺序语义（部分成功逐 item 对账）。 */
static void RunMultiItemWrite(SSAP_Link_S *serverLink, uint16_t okHandle, uint16_t badHandle)
{
    uint8_t pdu[32];
    SSAP_PduWriteReq_S *req = (SSAP_PduWriteReq_S *)pdu;
    req->msgCode = SSAP_WRITE_REQ;
    req->ctrl.fragment = SSAP_CTRL_NO_FRAG;
    req->ctrl.multi = 1;
    uint16_t handles[2] = {okHandle, badHandle};
    uint8_t *cur = pdu + sizeof(SSAP_PduWriteReq_S);
    for (int i = 0; i < 2; i++) {
        SSAP_PduWriteMultiItem_S *item = (SSAP_PduWriteMultiItem_S *)cur;
        item->handle = handles[i];
        item->subItemCount = 1;
        cur += sizeof(SSAP_PduWriteMultiItem_S);
        SSAP_PduWriteMultiSubItem_S *sub = (SSAP_PduWriteMultiSubItem_S *)cur;
        sub->type = SSAP_TYPE_DATA;
        sub->len = 2;
        cur += sizeof(SSAP_PduWriteMultiSubItem_S);
        cur[0] = 0x5A;
        cur[1] = 0xA5;
        cur += 2;
    }
    SDF_Buff_S *buff = SDF_BuffNew((uint32_t)(cur - pdu));
    if (buff == nullptr) {
        return;
    }
    uint8_t *dst = SDF_BuffAppend(buff, (uint32_t)(cur - pdu));
    if (dst != nullptr) {
        (void)memcpy_s(dst, (size_t)(cur - pdu), pdu, (size_t)(cur - pdu));
        SSAPS_WriteReqHandle(serverLink, buff);
    }
    SDF_BuffFree(buff);
}

/* 挂起任务冲刷：授权错配/分片丢包等场景客户端链路可能停留 BUSY，直调
 * 超时回调（与真实触发同一函数）清任务续跑队列，避免后续段排队饿死；
 * 回调按 kind=NONE 计数（arg 为栈上完成结构，只计数不解引用）。 */
static void E2eFlushBusyTask(SSAP_Link_S *link)
{
    if (link == nullptr) {
        return;
    }
    E2eQuiesce(); /* 先等 worker 级联收尾，避免与异步完成并发清同一任务（双完成=UAF） */
    if (link->status == SSAP_LINK_BUSY) {
        E2eObsReset(E2E_KNONE);
        SSAP_TimeoutCbk(link);
    }
}

/* 统一收尾（编译修复：原 goto CLEANUP 跨越 600 行变量初始化，C++ 禁止
 * ——改为就地调用；早退路径与正常路径同一清理面）。顺序：真实注销（内含
 * CleanTaskByAppId/cpcd 缓存清）→ 让注销腿在 worker 上先行 → 链路静默 →
 * 扣留缓冲/夹具释放 → 按 addr 删链路（B 客户端/B 服务端/A 服务端/A 客户端）
 * → 逆序 DeInit（ServiceDeInit 复位 g_curHandle/分配器，下轮布局确定）。 */
static void E2eTeardown(const SLE_Addr_S *serverAddr, const SLE_Addr_S *clientAddr,
    const SLE_Addr_S *clientAddrB, const SLE_Addr_S *serverAddrB2)
{
    if (g_cliAppId >= 0) {
        NLSTK_SsapClientDeregApp(g_cliAppId);
        g_cliAppId = -1;
    }
    if (g_srvAppId >= 0) {
        NLSTK_SsapServerDeregisterApplication(g_srvAppId);
        g_srvAppId = -1;
    }
    for (volatile int32_t i = 0; i < 1000000; i++) {
        (void)sched_yield();
    } /* 注销腿经调度：CleanTaskByAppId 遍历链路表，主线程待其先行后再删链路 */
    E2eQuiesce();
    if (g_holdBuff != nullptr) {
        SDF_BuffFree(g_holdBuff);
        g_holdBuff = nullptr;
    }
    SlinkFixtureClear();
    if (g_clientLinkB != nullptr) {
        SSAP_DeleteSsapLinkByAddr(const_cast<SLE_Addr_S *>(serverAddrB2));
        g_clientLinkB = nullptr;
    }
    if (g_serverLinkB != nullptr) {
        SSAP_DeleteSsapLinkByAddr(const_cast<SLE_Addr_S *>(clientAddrB));
        g_serverLinkB = nullptr;
    }
    if (g_serverLink != nullptr) {
        SSAP_DeleteSsapLinkByAddr(const_cast<SLE_Addr_S *>(serverAddr));
        g_serverLink = nullptr;
    }
    if (g_clientLink != nullptr) {
        SSAP_DeleteSsapLinkByAddr(const_cast<SLE_Addr_S *>(clientAddr));
        g_clientLink = nullptr;
    }
    SSAPS_ServiceDeInit();
    SSAP_LinkDeInit();
}

void FuzzSsapE2eSequence(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S serverAddr = {0};
    SLE_Addr_S clientAddr = {0};
    serverAddr.type = fdp.ConsumeIntegral<uint8_t>() % 2;
    clientAddr.type = fdp.ConsumeIntegral<uint8_t>() % 2;
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        serverAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        clientAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint16_t serverLcid = fdp.ConsumeIntegral<uint16_t>();
    uint16_t clientLcid = static_cast<uint16_t>(serverLcid ^ 0x8000); /* 保证互异 */
    uint16_t mtu = fdp.ConsumeIntegral<uint16_t>();
    uint8_t findType = fdp.ConsumeIntegral<uint8_t>() % 6; /* SS-C1：六类型全分发（含 0x02/0x05 拒绝分支） */
    uint16_t handle = fdp.ConsumeIntegral<uint16_t>();
    uint8_t writeType = fdp.ConsumeIntegral<uint8_t>();
    uint8_t valueLen = static_cast<uint8_t>(1 + fdp.ConsumeIntegral<uint8_t>() % sizeof(g_valueBuf));
    for (uint8_t i = 0; i < valueLen; i++) {
        g_valueBuf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    NLSTK_SsapUuid_S findUuid = {0};
    if (fdp.ConsumeIntegral<uint8_t>() % 2 == 0) {
        /* 半数用已缓存服务 UUID：驱动按 UUID 发现的匹配命中路径（全随机永远 miss） */
        (void)memcpy_s(&findUuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    } else {
        for (size_t i = 0; i < sizeof(findUuid.uuid); i++) {
            findUuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
    }
    /* ---- 以下新增消费全部追加在尾部（旧 corpus 耗尽取 0 → 走保守默认） ---- */
    uint8_t appMode = fdp.ConsumeIntegral<uint8_t>() % 2;   /* 0=遗留服务模式(旧口径) 1=app 路径全闭环 */
    uint8_t permCase = fdp.ConsumeIntegral<uint8_t>() % 3;  /* SS-R1：0 拒绝侧 1 允许侧 2 加密档 */
    uint8_t authzAllow = fdp.ConsumeIntegral<uint8_t>() % 2; /* SS-R2：先 allow 还是先 deny */
    uint8_t authzMis = fdp.ConsumeIntegral<uint8_t>() % 4;  /* SS-R3：错配变体 */
    uint8_t subCase = fdp.ConsumeIntegral<uint8_t>() % 3;   /* SS-R4：0 订阅 1 取消 2 断链重连 */
    uint8_t fragCase = fdp.ConsumeIntegral<uint8_t>() % 5;  /* SS-F：0 整包 1-4 见 FragFeedToClient */
    uint8_t timeCase = fdp.ConsumeIntegral<uint8_t>() % 3;  /* SS-T：0 关 1 超时直调 2 优先级插队 */
    uint8_t modeB = fdp.ConsumeIntegral<uint8_t>() % 2;     /* SS-03/SS-R4：B→A 第二客户端 */
    uint8_t multiCase = fdp.ConsumeIntegral<uint8_t>() % 3; /* SS-R5：item2 形态 */
    uint8_t dupCase = fdp.ConsumeIntegral<uint8_t>() % 2;   /* SS-R6：重复响应注入 */

    E2eInitTierUuids();
    uint32_t linkInitRet = SSAP_LinkInit();
    uint32_t svcInitRet = SSAPS_ServiceInit();
    if (appMode == 0 && linkInitRet == 0 && svcInitRet == 0) {
        AddService1();
        AddService2();
    }

    /* 链路指针登记文件域（静默屏障/统一清理跨函数访问）；B 端地址与 appOk
     * 声明前置，配合下方就地清理+提前 return 消除 goto 跨初始化（编译修复） */
    SLE_Addr_S clientAddrB = {0};
    SLE_Addr_S serverAddrB2 = {0};
    bool appOk = false;
    g_serverLink = SSAP_CreateSsapLink(&serverAddr, serverLcid, ServerSendCb);
    g_clientLink = SSAP_CreateSsapLink(&clientAddr, clientLcid, ClientSendCb);
    if (g_serverLink == nullptr || g_clientLink == nullptr) {
        E2eTeardown(&serverAddr, &clientAddr, &clientAddrB, &serverAddrB2);
        return;
    }
    g_serverInfo.lcid = serverLcid;
    g_clientInfo.lcid = clientLcid;
    g_cliNtfCount = 0;
    g_cliNtfCountB = 0;
    g_fragMode = 0;
    g_holdMode = false;
    g_fragFed = 0;
    g_seqLen = 0;

    /* SS-03：B→A 拓扑（第二客户端）。addr 语义：cpcd 副本键与 ByAddr 链路
     * 查找都用服务端链路的 link->addr（ssaps_server_write.c:740 传
     * &link->addr；ssaps_server.c:1420 FindSsapLinkByAddr），故 A 的身份
     * 标签=serverAddr、B 的=clientAddrB；B 客户端链路用独立标签
     * serverAddrB2，避免与 serverAddr 撞导致 ByAddr 查找歧义。 */
    if (modeB != 0) {
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            clientAddrB.addr[i] = (uint8_t)(clientAddr.addr[i] ^ 0x55);
            serverAddrB2.addr[i] = (uint8_t)(serverAddr.addr[i] ^ 0xAA);
        }
        clientAddrB.type = clientAddr.type;
        serverAddrB2.type = serverAddr.type;
        uint16_t serverLcidB = static_cast<uint16_t>(serverLcid ^ 0x4000);
        uint16_t clientLcidB = static_cast<uint16_t>(clientLcid ^ 0x4000);
        g_serverLinkB = SSAP_CreateSsapLink(&clientAddrB, serverLcidB, ServerSendCbB);
        g_clientLinkB = SSAP_CreateSsapLink(&serverAddrB2, clientLcidB, ClientSendCbB);
        if (g_serverLinkB != nullptr && g_clientLinkB != nullptr) {
            g_serverInfoB.lcid = serverLcidB;
            g_clientInfoB.lcid = clientLcidB;
        }
    }

    /* SS-00：真实注册 + app 路径服务（失败则 appMode 相关段全部跳过） */
    E2eRegisterApps(&serverAddr);
    for (int i = 0; i < TIER_NUM; i++) {
        g_propHandle[i] = 0;
    }
    g_methodHandle = 0;
    if (appMode != 0 && g_cliAppId >= 0 && g_srvAppId >= 0) {
        appOk = E2eSetupAppService();
    }

    /* 1. MTU/版本协商（version ≥ 0x0103 使能分片/多处理上下文） */
    {
        SSAP_ExchangeInfoReqInfo_S *req = (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_ExchangeInfoReqInfo_S));
        if (req != nullptr) {
            req->mtu = (mtu == 0) ? 0xFF : mtu;
            if (fragCase != 0) {
                req->mtu = 128; /* SS-F：小 MTU 才能触发分片路径 */
            }
            req->version = 0x0103;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            RunRequestTask(g_clientLink, SSAPC_ExchangeInfoReq, req, E2E_EXCHANGE);
            (void)E2eWaitObs();
        }
    }

    /* 2. 服务发现（服务端走缓存服务库真实组装响应；六类型含拒绝分支） */
    {
        SSAP_ParamFind_S *req = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
        if (req != nullptr) {
            req->appId = g_cliAppId;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->type = static_cast<SSAP_FindType_E>(findType);
            req->startHandle = 1; /* 产品守卫拒绝 startHandle==0 */
            req->endHandle = 0xFFFF;
            RunRequestTask(g_clientLink, SSAPC_FindReq, req, E2E_FIND);
            (void)E2eWaitObs();
        }
    }

    /* 3. 按 UUID 发现 */
    {
        SSAP_ParamFindByUuid_S *req = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
        if (req != nullptr) {
            req->appId = g_cliAppId;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->type = static_cast<SSAP_FindType_E>(findType);
            (void)memcpy_s(&req->uuid, sizeof(NLSTK_SsapUuid_S), &findUuid, sizeof(NLSTK_SsapUuid_S));
            req->startHandle = 1; /* 同上 */
            req->endHandle = 0xFFFF;
            RunRequestTask(g_clientLink, SSAPC_FindByUuidReq, req, E2E_FINDUUID);
            (void)E2eWaitObs();
        }
    }

    /* 4. 写请求（有响应）/写命令（无响应）：合法句柄与随机句柄两条路 */
    {
        uint16_t writeHandle = handle;
        if (appOk && g_propHandle[TIER_FREE] != 0) {
            writeHandle = g_propHandle[TIER_FREE]; /* SS-02：真实句柄合法路径 */
        }
        SSAP_WriteReqInfo_S *req = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_WriteReqInfo_S) + valueLen);
        if (req != nullptr) {
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->handle = writeHandle;
            req->type = writeType;
            req->value.len = valueLen;
            for (uint8_t i = 0; i < valueLen; i++) {
                req->value.value[i] = g_valueBuf[i];
            }
            RunRequestTask(g_clientLink, SSAPC_WriteReq, req, E2E_WRITE_REQ);
            (void)E2eWaitObs();
        }
        SSAP_WriteCmdInfo_S *cmd = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_WriteCmdInfo_S) + valueLen);
        if (cmd != nullptr) {
            (void)memcpy_s(&cmd->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            cmd->handle = handle; /* 随机句柄扰动路径保留 */
            cmd->type = writeType;
            cmd->value.len = valueLen;
            for (uint8_t i = 0; i < valueLen; i++) {
                cmd->value.value[i] = g_valueBuf[i];
            }
            RunRequestTask(g_clientLink, SSAPC_WriteCmd, cmd, E2E_WRITE_CMD);
        }
    }

    if (appOk) {
        /* 5. SS-A：读族。单读（期望初值 {0x00,0x01}）→多读（2 真实+1 无效，
         * 逐 item 状态而非总长）→按 UUID 读（STD 控制码路径） */
        {
            SSAP_ReadReqInfo_S *req = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
            if (req != nullptr) {
                req->appId = g_cliAppId;
                (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                req->handle = g_propHandle[TIER_FREE];
                req->type = SSAP_TYPE_DATA;
                RunRequestTask(g_clientLink, SSAPC_ReadReq, req, E2E_READ);
                if (E2eWaitObs()) {
                    NAI_LOG_INFO("[E2E] read rsp err=%u len=%u v0=%u v1=%u", g_obs.errCode,
                        g_obs.valLen, g_obs.valFirst[0], g_obs.valFirst[1]);
                }
            }
            SSAP_ReadPropsInfo_S *multi = (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_ReadPropsInfo_S) + 3 * sizeof(uint16_t));
            if (multi != nullptr) {
                multi->appId = g_cliAppId;
                (void)memcpy_s(&multi->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                multi->type = SSAP_TYPE_DATA;
                multi->num = 3;
                multi->handles[0] = g_propHandle[TIER_FREE];
                multi->handles[1] = g_propHandle[TIER_BIG];
                multi->handles[2] = 0x0F0F; /* 无效句柄：逐 item 状态 */
                RunRequestTask(g_clientLink, SSAPC_ReadProps, multi, E2E_READ_MULTI);
                (void)E2eWaitObs();
            }
            SSAP_ReadByUuidReqInfo_S *byUuid = (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_ReadByUuidReqInfo_S));
            if (byUuid != nullptr) {
                byUuid->appId = g_cliAppId;
                (void)memcpy_s(&byUuid->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                (void)memcpy_s(&byUuid->uuid, sizeof(NLSTK_SsapUuid_S), &g_tierUuid[TIER_FREE],
                    sizeof(NLSTK_SsapUuid_S));
                byUuid->handleStart = g_appSvcStart;
                byUuid->handleEnd = g_appSvcEnd;
                byUuid->dataType = SSAP_TYPE_DATA;
                RunRequestTask(g_clientLink, SSAPC_ReadByUuidReq, byUuid, E2E_READ_UUID);
                (void)E2eWaitObs();
            }
        }

        /* 6. SS-A：方法族。Req（真实方法句柄，服务端 onCallMethod 真实回结果）
         * + Cmd（无响应路径）+ 错误句柄（属性句柄冒充方法句柄） */
        {
            SSAP_CallMethodReqInfo_S *req = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_CallMethodReqInfo_S) + 4);
            if (req != nullptr) {
                (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                req->handle = g_methodHandle;
                req->value.len = 4;
                req->value.value[0] = 0xDE;
                req->value.value[1] = 0xAD;
                req->value.value[2] = 0xBE;
                req->value.value[3] = 0xEF;
                RunRequestTask(g_clientLink, SSAPC_CallMethodReq, req, E2E_METHOD_REQ);
                (void)E2eWaitObs(); /* 回包经 SendMethodCallRes 异步腿 */
            }
            SSAP_CallMethodCmdInfo_S *cmd = (SSAP_CallMethodCmdInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_CallMethodCmdInfo_S) + 4);
            if (cmd != nullptr) {
                (void)memcpy_s(&cmd->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                cmd->handle = g_methodHandle;
                cmd->value.len = 4;
                cmd->value.value[0] = 0x11;
                cmd->value.value[1] = 0x22;
                cmd->value.value[2] = 0x33;
                cmd->value.value[3] = 0x44;
                RunRequestTask(g_clientLink, SSAPC_CallMethodCmd, cmd, E2E_METHOD_CMD);
            }
            SSAP_CallMethodReqInfo_S *badReq = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_CallMethodReqInfo_S) + 2);
            if (badReq != nullptr) {
                (void)memcpy_s(&badReq->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                badReq->handle = g_propHandle[TIER_FREE];
                badReq->value.len = 2;
                badReq->value.value[0] = 0x00;
                badReq->value.value[1] = 0x01;
                RunRequestTask(g_clientLink, SSAPC_CallMethodReq, badReq, E2E_METHOD_REQ);
                (void)E2eWaitObs();
            }
        }

        /* 7. SS-R1：权限成对输入。无 slink：需认证/加密属性写期待 0x08/0x0A
         * 拒绝且受保值不变；slink FULL 夹具后同属性期待放行。 */
        {
            uint16_t guardHandle = g_propHandle[TIER_AUTHN];
            if (permCase == 2) {
                guardHandle = g_propHandle[TIER_ENC];
            }
            if (guardHandle != 0) {
                SSAP_WriteReqInfo_S *w = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                    sizeof(SSAP_WriteReqInfo_S) + 2);
                if (w != nullptr) {
                    (void)memcpy_s(&w->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    w->handle = guardHandle;
                    w->type = SSAP_TYPE_DATA;
                    w->value.len = 2;
                    w->value.value[0] = 0x77;
                    w->value.value[1] = 0x88;
                    RunRequestTask(g_clientLink, SSAPC_WriteReq, w, E2E_WRITE_REQ);
                    bool denied = E2eWaitObs() && g_obs.errCode != 0;
                    if (denied && permCase != 0) {
                        SlinkFixtureSet(&clientAddr, serverLcid);
                        SSAP_ReadReqInfo_S *r = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(
                            sizeof(SSAP_ReadReqInfo_S));
                        if (r != nullptr) {
                            r->appId = g_cliAppId;
                            (void)memcpy_s(&r->addr, sizeof(SLE_Addr_S), &clientAddr,
                                sizeof(SLE_Addr_S));
                            r->handle = guardHandle;
                            r->type = SSAP_TYPE_DATA;
                            RunRequestTask(g_clientLink, SSAPC_ReadReq, r, E2E_READ);
                            if (E2eWaitObs() && g_obs.errCode == 0 && g_obs.valLen >= 2) {
                                E2E_INVARIANT(!(g_obs.valFirst[0] == 0x77 && g_obs.valFirst[1] == 0x88),
                                    "R1: denied write leaked into guarded value");
                            }
                        }
                        SSAP_WriteReqInfo_S *w2 = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                            sizeof(SSAP_WriteReqInfo_S) + 2);
                        if (w2 != nullptr) {
                            (void)memcpy_s(&w2->addr, sizeof(SLE_Addr_S), &clientAddr,
                                sizeof(SLE_Addr_S));
                            w2->handle = guardHandle;
                            w2->type = SSAP_TYPE_DATA;
                            w2->value.len = 2;
                            w2->value.value[0] = 0x34;
                            w2->value.value[1] = 0x12;
                            RunRequestTask(g_clientLink, SSAPC_WriteReq, w2, E2E_WRITE_REQ);
                            (void)E2eWaitObs();
                        }
                        SlinkFixtureClear();
                    }
                }
            }
        }

        /* 8. SS-R2：授权闭环。需授权属性写→真实 requestId→allow(值变)/
         * deny(UNAUTHORIZED 且值不变，读回核对不变量) */
        {
            if (g_propHandle[TIER_AUTHZ] != 0) {
                g_authReqSeen = false;
                g_authReqId = 0;
                SSAP_WriteReqInfo_S *w = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                    sizeof(SSAP_WriteReqInfo_S) + 2);
                if (w != nullptr) {
                    (void)memcpy_s(&w->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    w->handle = g_propHandle[TIER_AUTHZ];
                    w->type = SSAP_TYPE_DATA;
                    w->value.len = 2;
                    w->value.value[0] = 0x99;
                    w->value.value[1] = 0xAA;
                    RunRequestTask(g_clientLink, SSAPC_WriteReq, w, E2E_WRITE_REQ);
                    /* 挂起（无立即回包）；授权请求回调在服务端处理栈内同步到达 */
                    if (g_authReqSeen && authzAllow == 0) {
                        (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, g_authReqId, true);
                        (void)E2eWaitObs();
                        g_authReqSeen = false;
                        SSAP_WriteReqInfo_S *w2 = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                            sizeof(SSAP_WriteReqInfo_S) + 2);
                        if (w2 != nullptr) {
                            (void)memcpy_s(&w2->addr, sizeof(SLE_Addr_S), &clientAddr,
                                sizeof(SLE_Addr_S));
                            w2->handle = g_propHandle[TIER_AUTHZ];
                            w2->type = SSAP_TYPE_DATA;
                            w2->value.len = 2;
                            w2->value.value[0] = 0xBC;
                            w2->value.value[1] = 0xDE;
                            RunRequestTask(g_clientLink, SSAPC_WriteReq, w2, E2E_WRITE_REQ);
                            if (g_authReqSeen) {
                                (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, g_authReqId, false);
                                if (E2eWaitObs()) {
                                    /* deny 后读回（读授权属性同样挂起，allow 完成）：
                                     * 值不得为 deny 写入值（不变量） */
                                    g_authReqSeen = false;
                                    SSAP_ReadReqInfo_S *r = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(
                                        sizeof(SSAP_ReadReqInfo_S));
                                    if (r != nullptr) {
                                        r->appId = g_cliAppId;
                                        (void)memcpy_s(&r->addr, sizeof(SLE_Addr_S), &clientAddr,
                                            sizeof(SLE_Addr_S));
                                        r->handle = g_propHandle[TIER_AUTHZ];
                                        r->type = SSAP_TYPE_DATA;
                                        RunRequestTask(g_clientLink, SSAPC_ReadReq, r, E2E_READ);
                                        if (g_authReqSeen) {
                                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId,
                                                g_authReqId, true);
                                            if (E2eWaitObs() && g_obs.errCode == 0 &&
                                                g_obs.valLen >= 2) {
                                                E2E_INVARIANT(
                                                    !(g_obs.valFirst[0] == 0xBC &&
                                                    g_obs.valFirst[1] == 0xDE),
                                                    "R2: denied authorize leaked into value");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else if (g_authReqSeen) {
                        /* 对称路径：先 deny（值不变，只记录），后续输入再变异 allow */
                        (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, g_authReqId, false);
                        (void)E2eWaitObs();
                    }
                }
            }
            E2eFlushBusyTask(g_clientLink); /* 授权挂起未完成的写不得阻塞后续段 */
        }

        /* 9. SS-R3：授权错配（错 appId/重放/无关 id/断链后晚到），单一扰动；
         * 核对口径=同一 requestId 完成次数 ≤1（不复活）。 */
        {
            if (g_propHandle[TIER_AUTHZ] != 0) {
                g_authReqSeen = false;
                SSAP_WriteReqInfo_S *w = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                    sizeof(SSAP_WriteReqInfo_S) + 2);
                if (w != nullptr) {
                    (void)memcpy_s(&w->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    w->handle = g_propHandle[TIER_AUTHZ];
                    w->type = SSAP_TYPE_DATA;
                    w->value.len = 2;
                    w->value.value[0] = 0x0F;
                    w->value.value[1] = 0xF0;
                    RunRequestTask(g_clientLink, SSAPC_WriteReq, w, E2E_WRITE_REQ);
                    if (g_authReqSeen) {
                        uint16_t rid = g_authReqId;
                        uint8_t cbBefore = g_obs.cbCount;
                        if (authzMis == 0) {
                            /* 错 appId：产品按全局 requestId 查找不校验归属
                             * （nlstk_ssap_app_server.c:234-247），按当前契约记录 */
                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId + 7, rid, true);
                            (void)E2eWaitObs();
                        } else if (authzMis == 1) {
                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, rid, true);
                            (void)E2eWaitObs();
                            uint8_t afterOnce = g_obs.cbCount;
                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, rid, true);
                            E2E_INVARIANT(g_obs.cbCount <= afterOnce,
                                "R3: consumed requestId re-completed");
                        } else if (authzMis == 2) {
                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId,
                                (uint16_t)(rid + 100), false);
                        } else {
                            SSAP_DeleteSsapLinkByAddr(&serverAddr);
                            g_serverLink = SSAP_CreateSsapLink(&serverAddr, serverLcid, ServerSendCb);
                            (void)NLSTK_SsapServerAuthorizeResult(g_srvAppId, rid, true);
                        }
                        NAI_LOG_INFO("[E2E] R3 mis=%u rid=%u cb=%u->%u", authzMis, rid, cbBefore,
                            (uint8_t)g_obs.cbCount);
                    }
                }
            }
            E2eFlushBusyTask(g_clientLink); /* 错配路径未完成的授权写同上冲刷 */
        }

        /* 10. SS-R4：订阅对抗。A 写 CCCD(0x0001 订阅/0x0000 取消) → 服务端
         * 定向通知（A 收、B 不收=不变量）；断链重连后无残留订阅=不变量。 */
        {
            if (g_propHandle[TIER_NTF] != 0) {
                SSAP_WriteReqInfo_S *sub = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
                    sizeof(SSAP_WriteReqInfo_S) + 2);
                if (sub != nullptr) {
                    (void)memcpy_s(&sub->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    sub->handle = g_propHandle[TIER_NTF];
                    sub->type = DESC_TYPE_CLIENT_CONFIG; /* 描述符写：dataType=描述符类型 */
                    sub->value.len = 2;
                    sub->value.value[0] = (subCase == 1) ? 0x00 : 0x01;
                    sub->value.value[1] = 0x00;
                    RunRequestTask(g_clientLink, SSAPC_WriteReq, sub, E2E_WRITE_REQ);
                    (void)E2eWaitObs();
                    uint32_t ntfA0 = g_cliNtfCount;
                    uint32_t ntfB0 = g_cliNtfCountB;
                    static uint8_t ntfVal[4] = {0x4E, 0x54, 0x46, 0x21};
                    NLSTK_VariableData_S value = {.len = sizeof(ntfVal), .data = ntfVal};
                    /* cpcd 副本键 = 服务端链路 link->addr（=serverAddr），ByAddr
                     * 通知必须用同一 addr 才能查到 g_serverLink（ssaps_server.c:1420） */
                    (void)NLSTK_SsapServerUpdateAndNotifyProperty(g_srvAppId, g_propHandle[TIER_NTF],
                        &serverAddr, &value);
                    for (volatile int32_t i = 0; i < 8000000 && g_cliNtfCount == ntfA0; i++) {
                        (void)sched_yield();
                    }
                    E2eQuiesce(); /* 通知链尾（ValueNtf 任务/对端解码）收尾后再比对计数 */
                    if (subCase == 0) {
                        E2E_INVARIANT(g_cliNtfCountB == ntfB0,
                            "R4: notification leaked to non-subscriber");
                    } else if (subCase == 1) {
                        E2E_INVARIANT(g_cliNtfCount == ntfA0,
                            "R4: notification after unsubscribe");
                    }
                    NAI_LOG_INFO("[E2E] R4 sub=%u ntfA=%u ntfB=%u", subCase,
                        (uint32_t)g_cliNtfCount, (uint32_t)g_cliNtfCountB);
                    if (subCase == 2) {
                        /* 断链重连（同 lcid 复用）。M1 修复（自检报告）：直删 SSAP 链路
                         * 不触发 cpcd 清理——产品仅在 CM 断链事件分支调用
                         * SSAPS_CleanServiceCpcd（ssap_manager.c:662-666），此处补调
                         * 代入 CM 语义，否则残留副本使重连通知照发、不变量必误触发 */
                        E2eQuiesce();
                        SSAP_DeleteSsapLinkByAddr(&serverAddr);
                        SSAPS_CleanServiceCpcd(&serverAddr);
                        g_serverLink = SSAP_CreateSsapLink(&serverAddr, serverLcid, ServerSendCb);
                        if (g_serverLink != nullptr) {
                            ntfA0 = g_cliNtfCount;
                            (void)NLSTK_SsapServerUpdateAndNotifyProperty(g_srvAppId,
                                g_propHandle[TIER_NTF], &serverAddr, &value);
                            for (volatile int32_t i = 0; i < 8000000 && g_cliNtfCount == ntfA0;
                                i++) {
                                (void)sched_yield();
                            }
                            E2eQuiesce();
                            E2E_INVARIANT(g_cliNtfCount == ntfA0,
                                "R4: subscription survived link reconnect");
                        }
                    }
                }
            }
        }

        /* 11. SS-R5：两 item 多值写（item2=无效句柄/服务变更句柄/需认证属性），
         * 交换顺序各投一次（协议允许部分成功：逐 item errList 对账） */
        {
            uint16_t badHandle = 0x0F0F;
            if (multiCase == 1) {
                badHandle = SSAP_SERVICE_CHANGE_EVENT_HANDLE; /* 不可写句柄 */
            } else if (multiCase == 2) {
                badHandle = g_propHandle[TIER_AUTHN]; /* 需认证（无 slink 时拒） */
            }
            if (g_propHandle[TIER_FREE] != 0 && badHandle != 0) {
                RunMultiItemWrite(g_serverLink, g_propHandle[TIER_FREE], badHandle);
                RunMultiItemWrite(g_serverLink, badHandle, g_propHandle[TIER_FREE]);
            }
        }

        /* 12. SS-R6：重复响应注入（IDLE 态 opcode 配对拒绝，无二次回调） */
        {
            if (dupCase != 0 && g_propHandle[TIER_FREE] != 0) {
                SSAP_ReadReqInfo_S *req = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
                if (req != nullptr) {
                    req->appId = g_cliAppId;
                    (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    req->handle = g_propHandle[TIER_FREE];
                    req->type = SSAP_TYPE_DATA;
                    RunRequestTask(g_clientLink, SSAPC_ReadReq, req, E2E_READ);
                    if (E2eWaitObs()) {
                        uint8_t cbBefore = g_obs.cbCount;
                        static const uint8_t dupRsp[] = {0x0e, 0x03, 0x11, 0x00, 0x00, 0x02, 0xAA, 0xBB};
                        SDF_Buff_S *bad = SDF_BuffNew(sizeof(dupRsp));
                        if (bad != nullptr) {
                            uint8_t *dst = SDF_BuffAppend(bad, sizeof(dupRsp));
                            if (dst != nullptr) {
                                (void)memcpy_s(dst, sizeof(dupRsp), dupRsp, sizeof(dupRsp));
                                (void)SSAP_Recv(&g_clientInfo, bad); /* IDLE 态期待拒绝 */
                            }
                            SDF_BuffFree(bad);
                        }
                        E2E_INVARIANT(g_obs.cbCount <= cbBefore,
                            "R6: duplicate response re-completed task");
                    }
                }
            }
        }

        /* 13. SS-F：分片。读大值属性（600B > mtu128）→ 响应经 FragFeedToClient
         * 切片回喂客户端真实重组；错误序列各自走 DROP 分支。发侧直调已按
         * M2 移除（见下方注释，环境前置不满足降级为记录+跳过）。 */
        {
            if (fragCase != 0 && g_propHandle[TIER_BIG] != 0) {
                g_fragMode = fragCase;
                SSAP_ReadReqInfo_S *req = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
                if (req != nullptr) {
                    req->appId = g_cliAppId;
                    (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    req->handle = g_propHandle[TIER_BIG];
                    req->type = SSAP_TYPE_DATA;
                    RunRequestTask(g_clientLink, SSAPC_ReadReq, req, E2E_READ);
                    bool ok = E2eWaitObs();
                    NAI_LOG_INFO("[E2E] frag case=%u fed=%u ok=%d err=%u len=%u", fragCase,
                        g_fragFed, (int)ok, g_obs.errCode, g_obs.valLen);
                }
                g_fragMode = 0;
                /* 错误序列（缺首片/重复尾片/截断/超长）丢弃重组 → 请求任务
                 * 无响应可配，冲刷避免 BUSY 残留 */
                E2eFlushBusyTask(g_clientLink);
                /* M2（自检报告）：发侧原为 SSAP_SendFragPkt 直调——其逐片经
                 * SSAP_SendBuffToDTAP→DTAP_DataSend 进入真实 DTAP 数据面，而本
                 * 工程进程未做 DTAP/DLI 初始化（真机回退态），半初始化态直调
                 * 属未验证契约（PR#331 SdfFuzz 同类崩溃）。按"环境前置不满足
                 * 降级为记录+跳过"原则移除直调；发侧覆盖由收侧 FragFeedToClient
                 * 的产品片头格式复刻 + 真机 DTAP 环境另行验证补齐。 */
            }
        }

        /* 14. SS-T：超时与优先级。扣留响应制造 busy → 排队普通请求 →
         * （优先级头插 / 超时直调）→ 放行扣留响应续跑；超时后不复活=不变量 */
        {
            if (timeCase != 0 && g_propHandle[TIER_FREE] != 0) {
                g_holdMode = true;
                SSAP_ParamFind_S *first = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
                if (first != nullptr) {
                    first->appId = g_cliAppId;
                    (void)memcpy_s(&first->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
                    first->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
                    first->startHandle = 1;
                    first->endHandle = 0xFFFF;
                    RunRequestTask(g_clientLink, SSAPC_FindReq, first, E2E_FIND);
                    /* 响应被扣留 → 链路 busy，首任务在途 */
                    SSAP_ParamFindByUuid_S *normal = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(
                        sizeof(SSAP_ParamFindByUuid_S));
                    if (normal != nullptr) {
                        normal->appId = g_cliAppId;
                        (void)memcpy_s(&normal->addr, sizeof(SLE_Addr_S), &clientAddr,
                            sizeof(SLE_Addr_S));
                        normal->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
                        (void)memcpy_s(&normal->uuid, sizeof(NLSTK_SsapUuid_S), &g_appSvcUuid,
                            sizeof(NLSTK_SsapUuid_S));
                        normal->startHandle = 1;
                        normal->endHandle = 0xFFFF;
                        SSAP_TaskParam_S normalParam = {.appId = g_cliAppId, .arg = normal,
                            .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq, .timeout = 3000,
                            .valid = true, .appCallback = E2eTaskCallback};
                        E2eObsReset(E2E_FINDUUID);
                        if (timeCase == 2) {
                            /* 高优先级头插：应先于任何已排队普通任务执行 */
                            SSAP_ProcessHighPriorityRequestTask(g_clientLink, &normalParam, false);
                        } else {
                            SSAP_ProcessRequestTask(g_clientLink, &normalParam, false);
                        }
                    }
                    if (timeCase == 1) {
                        /* 超时直调（与真实触发同一函数）：首任务 TIMEOUT 收尾，
                         * 队列续跑；二次直调不复活 */
                        E2eObsReset(E2E_FIND);
                        SSAP_TimeoutCbk(g_clientLink);
                        (void)E2eWaitObs();
                        uint8_t seqAfterFirst = g_seqLen;
                        SSAP_TimeoutCbk(g_clientLink);
                        E2E_INVARIANT(g_seqLen <= seqAfterFirst + 1,
                            "T: task resurrected after timeout");
                    }
                    g_holdMode = false;
                    E2eQuiesce(); /* 扣留期间 worker 可能仍在级联，先静默再投递 */
                    if (g_holdBuff != nullptr) {
                        SDF_Buff_S *held = g_holdBuff;
                        g_holdBuff = nullptr;
                        if (g_clientLink->status == SSAP_LINK_BUSY) {
                            (void)SSAP_Recv(&g_clientInfo, held);
                        }
                        SDF_BuffFree(held);
                    }
                }
            }
        }
    }

    /* 非法路径保留：向服务端链路直接注入未请求的响应（opcode 期待不匹配/
     * 空闲态拒绝分支），以及服务端回错的错误响应闭环 */
    {
        static const uint8_t unsolicitedRsp[] = {0x0e, 0x03, 0x11, 0x00, 0x00, 0x06};
        SDF_Buff_S *bad = SDF_BuffNew(sizeof(unsolicitedRsp));
        if (bad != nullptr) {
            uint8_t *dst = SDF_BuffAppend(bad, sizeof(unsolicitedRsp));
            if (dst != nullptr) {
                (void)memcpy_s(dst, sizeof(unsolicitedRsp), unsolicitedRsp, sizeof(unsolicitedRsp));
                (void)SSAP_Recv(&g_serverInfo, bad);
            }
            SDF_BuffFree(bad);
        }
    }

    /* SS-03：轮间清理（编译修复：goto 跨初始化消除后，就地调用统一清理） */
    E2eTeardown(&serverAddr, &clientAddr, &clientAddrB, &serverAddrB2);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    /* SS-00：RegApp/AddService/AuthorizeResult/SendMethodCallRes 走
     * SchedulePostTask*，调度器不活则全部失败（uapi fuzzer 同款先例；
     * InitStack 回滚会销毁调度器，末尾重建幂等）。 */
    ScheduleEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSsapE2eSequence(fuzzData, size);
    free(fuzzData);
    return 0;
}
