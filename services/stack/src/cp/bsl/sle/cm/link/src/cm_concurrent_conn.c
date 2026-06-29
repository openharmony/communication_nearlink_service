/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cm_concurrent_conn.h"
#include <string.h>
#include "cm.h"
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sdf_dlist.h"
#include "sdf_timer.h"
#include "cp_worker.h"
#include "dli_def.h"
#include "dli_errno.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_api.h"
#include "cm_util.h"
#include "sle_logic_link_mgr.h"
#include "cm_logic_link_listener_mgr.h"
#include "cm_inner_api.h"
#include "collab_ext_func_wrapper.h"
#include "collab_reg_ext_func.h"
#include "cm_link_collab_func.h"

#define CM_DIRECT_CONN_TIMEOUT_MS 10000U  // 10s
#define CM_BG_CONN_MAX_ADDR_SIZE 16       // 暂时先限制规格为16，后续跟可一次性添加白名单列表保持一致

// 白名单连接设置参数
// 主动连接参数
#ifdef TV_STANDARD
// 通过算法计算的参数，实际验证，该组参数直连效果更优
#define CM_DIRECT_CONN_SCAN_PRIVATE_WINDOW  0x66F   // 0x66F * 0.125 = 205.875ms
#define CM_DIRECT_CONN_SCAN_PRIVATE_INTERVAL 0xB48  // 0xB48 * 0.125 = 361ms
#else
#define CM_DIRECT_CONN_SCAN_PRIVATE_WINDOW  0xF0   // 0xF0 * 0.125 = 30ms
#define CM_DIRECT_CONN_SCAN_PRIVATE_INTERVAL 0x1E0 // 0x1E0 * 0.125 = 60ms
#endif
// 背景连接参数
#define CM_BG_CONN_SCAN_PRIVATE_WINDOW   0x190     // 0x190 * 0.125 = 50ms
#define CM_BG_CONN_SCAN_PRIVATE_INTERVAL 0x960     // 0x960 * 0.125 = 300ms
// 主动连接和背景连接共同参数
#define CM_CONN_PRIVATE_MIN_INTERVAL 0x50          // 0x50 * 0.125 = 10ms
#define CM_CONN_PRIVATE_MAX_INTERVAL 0x50          // 0x50 * 0.125 = 10ms
#define CM_CONN_INITIATE_PHYS        1             // 链路扫描通信带宽： 1:1M, 2:2M
#define CM_CONN_NEGOTIATE            0x1           // 链路建立时是否进行G和T交互
#define CM_CONN_PRIVATE_TIMEOUT      0xC8          // uint:10ms

// 当前最大待连接数，防止无限制连接添加导致产生内存超限等问题
#define CM_MAX_CONNECTING_DEV_NUM 1024

// 用户连接发起方式
typedef enum {
    CM_CONN_INITIATE_TYPE_EMPTY = 0x00,            // 不存在连接
    CM_CONN_INITIATE_TYPE_ONLY_DIRECT = 0x01,      // 只存在主动连接
    CM_CONN_INITIATE_TYPE_ONLY_BG = 0x10,          // 只存在背景连接
    CM_CONN_INITIATE_TYPE_DIRECT_AND_BG = 0x11,    // 既存在主动连接，又存在背景连接
} CM_ConnInitiateType_E;

// 主动连接超时超时回调处理函数
typedef void (*CM_DoingDirectConnTimeoutCbk)(void *args);

typedef struct {
    SDF_DListEntry_S entry;   // 链表节点
    uint8_t moduleId;
    bool isBypass;
} CM_DoingBgConnNode_S;

typedef struct {
    SDF_DListHead_S list;  // 保存正在运行背景连接的用户应用模块标识集合（保存进来时，已去重），成员为CM_DoingBgConnNode_S
} CM_DoingBgConnSet_S;

typedef struct {
    void *args;
    int timerId;
    uint8_t frameType;
} CM_DoingDirectConnAlarm_S;

typedef struct {
    // moduleID, 记录id用于回调失败上报
    uint8_t moduleId;
    // Key: moduleId. Ids of clients doing background connection to given device.
    CM_DoingBgConnSet_S doingBgConnSet;
    // Key: moduleId, Value: CM_DoingDirectConnAlarm_S. App modules trying to do direct connection with a timeout alarm
    SDF_Map *doingDirectConnMap;
    size_t sizeDoingDirect;
} CM_AppConnectingDev_S;

typedef struct {
    uint8_t moduleId;
    SLE_Addr_S addr;
} CM_DirectConnTimoutArgs_S;

typedef struct {
    uint8_t moduleId;
    SLE_Addr_S addr;
    uint8_t frameType;
} CM_DirectConnectAddReq_S;

typedef struct {
    uint8_t moduleId;
    SLE_Addr_S addr;
} CM_BgConnectRmvReq_S;

typedef struct {
    uint8_t moduleId;
    SLE_Addr_S addr;
    uint8_t discReason;
} CM_DirectConnectRemoveReq_S;

typedef struct {
    uint8_t moduleId;
} CM_BgConnectClearReq_S;

typedef struct {
    uint8_t moduleId;
    CM_BgConnAddrParam_S *addrArr;
    uint8_t addrArrCount;
} CM_BgConnectAddReq_S;

typedef struct {
    CM_ConnInitiateType_E initiateType;
} CM_InitiateConnDevStatus_S;

// Key: rawAddress, Value: CM_AppConnectingDev_S
static SDF_Map *g_cmConnectingDevMap = NULL;  // CM模块未初始化或者已去初始化，该变量值为NULL
static size_t g_cmSizeConnectingDev = 0; // CM模块未初始化或者已去初始化，该变量值为0

static uint32_t CM_CollabStartConnReq(void);
static void CM_NotifyConnFailed(void);

static inline int CM_ModuleIdCompare(const void *lhs_, const void *rhs_)
{
    uint8_t lhs = *(uint8_t *)lhs_;
    uint8_t rhs = *(uint8_t *)rhs_;
    return lhs - rhs;
}

static void CM_SimpleDataDtor(void *arg)
{
    if (arg == NULL) {
        return;
    }
    SDF_MemFree(arg);
}

static void CM_BgConnNodeDtor(SDF_DListEntry_S *entry)
{
    if (entry == NULL) {
        return;
    }
    SDF_MemFree(entry);
}

static void CM_BgConnectReqDtor(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_BgConnectAddReq_S *req = (CM_BgConnectAddReq_S *)arg;
    SDF_MemFree(req->addrArr);
    SDF_MemFree(req);
}

static inline void CM_DestroyDoingBgConnSet(CM_DoingBgConnSet_S *doingBgConnSet)
{
    CM_LOGI("app module connecting map dtor, DoingBgConnSet count:%u", SDF_DListCount(&doingBgConnSet->list));
    SDF_DListDestroy(&doingBgConnSet->list, CM_BgConnNodeDtor);
}

// 1）销毁全局连接中设备列表节点
static void CM_AppModuleConnectingMapDtor(void *args)
{
    CM_CHECK_RETURN(args != NULL, "args is null");
    CM_AppConnectingDev_S *appList = (CM_AppConnectingDev_S *)args;
    CM_DestroyDoingBgConnSet(&appList->doingBgConnSet);
    if (appList->doingDirectConnMap != NULL) {
        SDF_MapDtor(appList->doingDirectConnMap);
        appList->doingDirectConnMap = NULL;
        appList->sizeDoingDirect = 0;
    }
    SDF_MemFree(appList);
}

// 1）创建全局连接中设备列表
static uint32_t CM_CreateConnectingDevMap(void)
{
    g_cmConnectingDevMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(CM_AppModuleConnectingMapDtor, NULL));
    CM_CHECK_RETURN_RET(g_cmConnectingDevMap != NULL, CM_MEM_ERR, "malloc failed.");
    g_cmSizeConnectingDev = 0;
    return CM_SUCCESS;
}

uint32_t CM_ConcurrentConnInit(void)
{
    CM_ConcurrentConnDeInit();
    COLLAB_CollabCmCbk_S cbk = {
        .startConnReq = CM_CollabStartConnReq,
        .notifyConnFailed = CM_NotifyConnFailed,
    };
    COLLAB_CmCollabFunc_S func = {
        .regCollabFunc = CM_LinkCollabRegFunc,
        .unRegCollabFunc = CM_LinkCollabUnRegFunc,
    };
    if (COLLAB_CmInit(&cbk, &func) != 0) {
        CM_LOGW("collab stm cm init failed");
    }

    uint32_t ret = CM_CreateConnectingDevMap();
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "create connecting dev map failed.");
    CM_LOGI("connection manager init success");
    return CM_SUCCESS;
}

// 1.1）销毁主动连接设备列表节点
static void CM_DoingDirectConnMapDtor(void *args)
{
    CM_CHECK_RETURN(args != NULL, "args is null");
    CM_DoingDirectConnAlarm_S *alarm = (CM_DoingDirectConnAlarm_S *)args;
    CM_LOGI("doing direct conn map dtor, timerId:%d", alarm->timerId);
    CP_TimerDel(alarm->timerId);
    SDF_MemFree(alarm->args);
    alarm->args = NULL;
    SDF_MemFree(alarm);
}

// 1.1）创建主动连接设备列表
static SDF_Map *CM_CreateDoingDirectConnMap(void)
{
    SDF_Traits keyTraits = {
        .dtor = CM_SimpleDataDtor,
        .cmptor = CM_ModuleIdCompare,
    };
    SDF_Traits valTraits = {
        .dtor = CM_DoingDirectConnMapDtor,
        .cmptor = NULL,
    };

    SDF_Map *map = SDF_MapCtor(keyTraits, valTraits);
    CM_CHECK_RETURN_RET(map != NULL, NULL, "malloc failed.");
    return map;
}

static inline bool CM_ConnectingDevMapIsEmpty(void)
{
    CM_LOGD("connecting dev map size:%zu", g_cmSizeConnectingDev);
    return (g_cmConnectingDevMap->entry == NULL);
}

static inline void CM_DestroyDoingDirectConnMap(CM_AppConnectingDev_S *connectingList)
{
    if (connectingList->sizeDoingDirect == 0 && connectingList->doingDirectConnMap != NULL) {
        CM_LOGI("destroy doing direct conn map");
        SDF_MemFree(connectingList->doingDirectConnMap);
        connectingList->doingDirectConnMap = NULL;
        connectingList->sizeDoingDirect = 0;
    }
}

static void CM_EraseDoingDirectConnMap(CM_AppConnectingDev_S *devList, uint8_t moduleId)
{
    if (devList->doingDirectConnMap != NULL) {
        if (SDF_MapErase(devList->doingDirectConnMap, &moduleId)) {
            devList->sizeDoingDirect--;
            CM_LOGI("erase a doing direct connecting node, moduleId:%hhu, DoingDirectConnMap size:%zu",
                moduleId, devList->sizeDoingDirect);
        } else {
            CM_LOGW("erase doing direct connecting node failed, moduleId:%hhu, DoingDirectConnMap size:%zu",
                moduleId, devList->sizeDoingDirect);
        }
    }
    CM_DestroyDoingDirectConnMap(devList);
}

static CM_AppConnectingDev_S *CM_ConnectingDevFind(const SLE_Addr_S *addr)
{
    SDF_MapIter *iter = SDF_MapFind(g_cmConnectingDevMap, (SLE_Addr_S *)addr);
    if (iter != NULL) {
        return iter->val;
    }
    return NULL;
}

// 连接参数自动换挡设置，若存在主动连接，则使用高频连接间隔，否则只使用低频连接间隔
static uint32_t CM_ConnectSetAllowListParam(bool isDirectDoing, uint8_t bitFrameType)
{
    CM_ConnectSetParamReq_S setParam = {0};
    setParam.bitFrameType = bitFrameType;
    uint16_t scanInterval = CM_BG_CONN_SCAN_PRIVATE_INTERVAL;
    uint16_t scanWindow = CM_BG_CONN_SCAN_PRIVATE_WINDOW;
    if (isDirectDoing) {
        scanInterval = CM_DIRECT_CONN_SCAN_PRIVATE_INTERVAL;
        scanWindow = CM_DIRECT_CONN_SCAN_PRIVATE_WINDOW;
    }
    setParam.enableFilterPolicy = true;
    setParam.scanInterval = scanInterval;
    setParam.scanWindow = scanWindow;
    setParam.initiatingPhys = CM_CONN_INITIATE_PHYS;
    setParam.gtNegotiate = CM_CONN_NEGOTIATE;
    setParam.minInterval = CM_CONN_PRIVATE_MIN_INTERVAL;
    setParam.maxInterval = CM_CONN_PRIVATE_MAX_INTERVAL;
    setParam.timeout = CM_CONN_PRIVATE_TIMEOUT;
    return CM_ConnectSetParamReq(&setParam);
}

static bool CM_CheckAddrIsBypass(const CM_AppConnectingDev_S *connectingList)
{
    if (connectingList->doingDirectConnMap != NULL && connectingList->sizeDoingDirect != 0) {
        // 如果有主动连接，就不需要Bypass
        return false;
    }
    // 背景连接该地址只有全部设置了Bypass，那么才认为需要Bypass
    const CM_DoingBgConnSet_S *doingBgConnSet = &connectingList->doingBgConnSet;

    CM_DoingBgConnNode_S *node = NULL;
    if (SDF_DListCount((SDF_DListHead_S *)&doingBgConnSet->list) == 0) {
        return false;
    }
    bool isBypass = true;
    SDF_DListElmForeach(node, &(doingBgConnSet->list), entry) {
        if (!node->isBypass) {
            isBypass = false;
            break;
        }
    }
    return isBypass;
}

static uint8_t CM_CalcAddrBitFrameType(CM_AppConnectingDev_S *connectingList)
{
    uint8_t bitFrameType = 0;
    const CM_DoingBgConnSet_S *doingBgConnSet = &connectingList->doingBgConnSet;

    if (SDF_DListCount((SDF_DListHead_S *)&doingBgConnSet->list) != 0) {
        // 当前背景连接只有帧1
        bitFrameType |= DLI_SCAN_BIT_FRAME_FORMAT_1_IND;
    }
    if (connectingList->doingDirectConnMap == NULL || connectingList->sizeDoingDirect == 0) {
        // 只有背景连接
        return bitFrameType;
    }

    for (SDF_MapIter *cur = connectingList->doingDirectConnMap->entry; cur != NULL; cur = cur->next) {
        CM_DoingDirectConnAlarm_S *connParam = (CM_DoingDirectConnAlarm_S *)cur->val;
        if (connParam->frameType == CM_CONN_PARAM_FRAME_TYPE_1) {
            bitFrameType |= DLI_SCAN_BIT_FRAME_FORMAT_1_IND;
        }
        if (connParam->frameType == CM_CONN_PARAM_FRAME_TYPE_4) {
            bitFrameType |= DLI_SCAN_BIT_FRAME_FORMAT_4_IND;
        }
    }
    return bitFrameType;
}

static uint32_t CM_SetConnectParamAndResetAllowList(void)
{
    if (CM_ConnectingDevMapIsEmpty()) {
        return CM_SUCCESS;
    }
    size_t count = 0;
    uint32_t ret = CM_SUCCESS;
    CM_ClearAcceptFilterList();

    bool isDirectDoing = false;
    uint8_t bitFrameType = 0;
    for (SDF_MapIter *cur = g_cmConnectingDevMap->entry; cur != NULL; cur = cur->next) {
        SLE_Addr_S *addr = (SLE_Addr_S *)cur->key;
        CM_AppConnectingDev_S *connectingList = (CM_AppConnectingDev_S *)cur->val;
        if (connectingList->doingDirectConnMap != NULL && connectingList->sizeDoingDirect != 0) {
            isDirectDoing = true;
            CM_LOGI("addr:%s is a direct conn doing device", GET_ENC_ADDR(addr));
        }
        bool isBypass = CM_CheckAddrIsBypass(connectingList);
        uint8_t bitOneFrameType = CM_CalcAddrBitFrameType(connectingList);
        bitFrameType |= bitOneFrameType;
        ret = CM_AddDeviceToAcceptFilterList(addr, isBypass);
        CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "add a addr:%s to allow list failed", GET_ENC_ADDR(addr));
        count++;
        CM_LOGI("add a addr:%s, isBypass:%d, bitOneFrameType:0x%02x, bitFrameType:0x%02x to allow list, sum:%zu",
            GET_ENC_ADDR(addr), isBypass, bitOneFrameType, bitFrameType, count);
    }
    ret = CM_ConnectSetAllowListParam(isDirectDoing, bitFrameType);
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "set allow list param addr failed");
    return CM_SUCCESS;
}

static void CM_NotifyDirectConnectFailed(uint8_t moduleId, uint16_t lcid, const SLE_Addr_S *addr, uint8_t discReason)
{
    CM_LOGI("notify link state changed, lcid:%04x, addr:%s, discReason:%hhu", lcid, GET_ENC_ADDR(addr), discReason);
    CM_LogicLinkState_S param = {0};
    param.lcid = lcid;
    param.role = 0;
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param.result = CM_LINK_STATE_DISCONNECTED;
    param.discReason = discReason;
    CM_ExecLogicLinkModuleCbks(moduleId, &param);
}

static void CM_TryDestroyConnectingDevEmptyNode(CM_AppConnectingDev_S *node, const SLE_Addr_S *addr)
{
    uint32_t sizeDoingBg = SDF_DListCount(&node->doingBgConnSet.list);
    size_t sizeDoingDirect = node->sizeDoingDirect;
    CM_LOGI("addr:%s, doing bg conn set size:%d, doing direct conn map size:%zu",
        GET_ENC_ADDR(addr), sizeDoingBg, sizeDoingDirect);
    if (sizeDoingBg == 0 && sizeDoingDirect == 0) {
        if (!SDF_MapErase(g_cmConnectingDevMap, (void *)addr)) {
            CM_LOGW("dev:%s is not in connecting dev map", GET_ENC_ADDR(addr));
            return;
        }
        g_cmSizeConnectingDev--;
        CM_LOGI("remove a connecting dev map node, current size:%zu", g_cmSizeConnectingDev);
    }
}

static void CM_NotifyConnFailed(void)
{
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    if (CM_ConnectingDevMapIsEmpty()) {
        CM_LOGI("connecting dev list is empty");
        return;
    }
    for (SDF_MapIter *cur = g_cmConnectingDevMap->entry; cur != NULL; cur = cur->next) {
        SLE_Addr_S *addr = (SLE_Addr_S *)cur->key;
        CM_AppConnectingDev_S *connectingList = (CM_AppConnectingDev_S *)cur->val;
        if (connectingList->doingDirectConnMap == NULL || connectingList->sizeDoingDirect == 0) {
            // 调用失败处理，背景连接无需失败上报
            continue;
        }
        uint8_t moduleId = connectingList->moduleId;
        CM_NotifyDirectConnectFailed(moduleId, CM_INVALID_LCID, addr, DLI_MEMORY_CAPACITY_EXCEEDED);
    }
}

static uint32_t CM_StartAllowListConnReqInner(void)
{
    CM_LOGI("start allow list conn req Inner");
    uint32_t ret = CM_SUCCESS;

    CM_ConnectParamReq_S connReqParam = {0};
    connReqParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    connReqParam.version = CM_CONNECT_VERSION_1_0;
    ret = CM_ConnectEstablishReq(&connReqParam);
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "connect establish req failed");
    return ret;
}

static CM_InitiateConnDevStatus_S CM_GetInitiateConnDevStatus(void)
{
    CM_InitiateConnDevStatus_S result = {
        .initiateType = CM_CONN_INITIATE_TYPE_EMPTY,
    };
    if (CM_ConnectingDevMapIsEmpty()) {
        return result;
    }
    for (SDF_MapIter *cur = g_cmConnectingDevMap->entry; cur != NULL; cur = cur->next) {
        CM_AppConnectingDev_S *devNode = (CM_AppConnectingDev_S *)cur->val;
        if (devNode->doingDirectConnMap != NULL && devNode->sizeDoingDirect != 0) {
            result.initiateType |= CM_CONN_INITIATE_TYPE_ONLY_DIRECT;
        }
        uint32_t sizeDoingBg = SDF_DListCount(&devNode->doingBgConnSet.list);
        if (sizeDoingBg != 0) {
            result.initiateType |= CM_CONN_INITIATE_TYPE_ONLY_BG;
        }
    }
    return result;
}

static uint32_t CM_StartAllowListConnReq(const SLE_Addr_S *directConnAddr)
{
    if (!CM_ConnectingDevMapIsEmpty() && CM_IsNeedLinkCollabReq()) {
        CM_LOGI("start collab conn req");
        CM_InitiateConnDevStatus_S initiateConn = CM_GetInitiateConnDevStatus();
        CM_LOGI("start link collab req, conn initiate type:%d", initiateConn.initiateType);
        CM_CHECK_RETURN_RET(CM_StartLinkCollabReq((uint8_t)initiateConn.initiateType, directConnAddr),
            CM_FAIL, "link collab failed");
        return CM_SUCCESS;
    }
    uint32_t ret = CM_SetConnectParamAndResetAllowList();
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "set connect param and reset allow list failed");
    if (g_cmSizeConnectingDev == 0) {
        CM_LOGW("allow list count is 0, no need start establish request");
        return CM_SUCCESS;
    }
    return CM_StartAllowListConnReqInner();
}

static uint32_t CM_CollabStartConnReq(void)
{
    CM_CHECK_RETURN_RET(g_cmConnectingDevMap != NULL, CM_NOT_INITED, "CM has not inited, connecting dev map is null.");
    if (!CM_ConnectingDevMapIsEmpty()) {
        uint32_t ret = CM_SetConnectParamAndResetAllowList();
        CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "set connect param and reset allow list failed");
        return CM_StartAllowListConnReqInner();
    } else {
        CM_LOGW("current connecting dev map is empty, no need start establish req");
        CM_ConcurrentConnNotifyLinkCollabResult();
        return CM_SUCCESS;
    }
}

static uint32_t CM_CurrentConnCancelReq(void)
{
    uint32_t ret = CM_ConnectCancelReq();
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "cancel connect establish req failed");
    return CM_SUCCESS;
}

static void CM_DirectConnTimeoutCbkInner(void *args)
{
    if (g_cmConnectingDevMap == NULL) {
        return;
    }
    CM_DoingDirectConnAlarm_S *alarm = (CM_DoingDirectConnAlarm_S *)args;
    CM_CHECK_RETURN(alarm != NULL, "alarm is null");
    CM_DirectConnTimoutArgs_S *tArgs = (CM_DirectConnTimoutArgs_S *)alarm->args;
    CM_CHECK_RETURN(tArgs != NULL, "tArgs is null");
    CM_LOGI("direct conn timeout, moduleId:%hhu, timerId:%d, addr:%s, current size:%zu",
        tArgs->moduleId, alarm->timerId, GET_ENC_ADDR(&tArgs->addr), g_cmSizeConnectingDev);
    // 定时器里的设备地址将会被销毁，此处需要先拷贝出来
    SLE_Addr_S addr;
    uint8_t moduleId = tArgs->moduleId;
    (void)memcpy_s(&addr, sizeof(SLE_Addr_S), &tArgs->addr, sizeof(SLE_Addr_S));
    // 只删除主动连接节点列表中的数据，不删除背景连接的数据(存在数据情况下)
    CM_AppConnectingDev_S *node = CM_ConnectingDevFind(&tArgs->addr);
    if (node != NULL) {
        CM_EraseDoingDirectConnMap(node, tArgs->moduleId);
        CM_TryDestroyConnectingDevEmptyNode(node, &addr);
    } else {
        CM_LOGW("find addr:%s not in connecting device map", GET_ENC_ADDR(&addr));
    }
    uint32_t ret = CM_CurrentConnCancelReq();
    CM_CHECK_RETURN(ret == CM_SUCCESS, "cancel connect establish req failed");
    // 判断当前是否存在其他地址还在连接中
    if (!CM_ConnectingDevMapIsEmpty()) {
        ret = CM_StartAllowListConnReq(NULL);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "start allow list conn req failed");
    } else {
        CM_ConcurrentConnNotifyLinkCollabResult();
    }
    CM_NotifyDirectConnectFailed(moduleId, CM_INVALID_LCID, &addr, DLI_CONNECTION_TIMEOUT);
}

static CM_AppConnectingDev_S *CM_CreateAppModuleConnectingDev(const SLE_Addr_S *addr)
{
    CM_AppConnectingDev_S *node = CM_ConnectingDevFind(addr);
    if (node != NULL) {
        CM_LOGE("already has the connecting dev:%s", GET_ENC_ADDR(addr));
        return node;
    }
    CM_LOGD("appConnectingList is not exist, create it");
    node = (CM_AppConnectingDev_S *)SDF_MemZalloc(sizeof(CM_AppConnectingDev_S));
    CM_CHECK_RETURN_RET(node != NULL, NULL, "malloc for connecting node failed");
    SLE_Addr_S *reqAddr = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
    if (reqAddr == NULL) {
        SDF_MemFree(node);
        return NULL;
    }
    (void)memcpy_s(reqAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));

    if (!SDF_MapMoveInsert(g_cmConnectingDevMap, (void *)reqAddr, node)) {
        SDF_MemFree(node);
        SDF_MemFree(reqAddr);
        return NULL;
    }
    g_cmSizeConnectingDev++;
    SDF_DListHeadInit(&(node->doingBgConnSet.list));
    CM_LOGI(
        "insert a connecting dev map node, addr:%s, current size:%zu", GET_ENC_ADDR(addr), g_cmSizeConnectingDev);
    return node;
}

static uint32_t CM_StartRmvAllowListConnReq(void)
{
    CM_LOGI("start rmv allow list conn req");
    // 删除白名单，先取消建链
    uint32_t ret = CM_CurrentConnCancelReq();
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "cancel connect establish req failed");
    if (!CM_ConnectingDevMapIsEmpty()) {
        ret = CM_StartAllowListConnReq(NULL);
        CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "start allow list conn req failed");
    } else {
        CM_ConcurrentConnNotifyLinkCollabResult();
    }
    return ret;
}

static void CM_NotifyConnecting(uint8_t moduleId, const SLE_Addr_S *addr)
{
    CM_LOGI("notify link state connecting, moduleId:%hhu, addr:%s", moduleId, GET_ENC_ADDR(addr));
    CM_LogicLinkState_S param = {0};
    param.lcid = CM_INVALID_LCID;
    param.role = 0;
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param.result = CM_LINK_STATE_CONNECTING;
    param.discReason = 0;  // 连接中，断连原因取0即可
    CM_ExecLogicLinkModuleCbks(moduleId, &param);
}

static CM_DirectConnTimoutArgs_S *CM_DirectConnTimoutArgsNew(uint8_t moduleId, const SLE_Addr_S *addr)
{
    CM_DirectConnTimoutArgs_S *args = (CM_DirectConnTimoutArgs_S *)SDF_MemZalloc(sizeof(CM_DirectConnTimoutArgs_S));
    CM_CHECK_RETURN_RET(args != NULL, NULL, "alloc direct conn timeout args null.");
    args->moduleId = moduleId;
    (void)memcpy_s(&args->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    return args;
}

static void CM_DoingConnDirectInsertFailProc(uint8_t *newModuleId, CM_DirectConnTimoutArgs_S *tArgs,
    CM_DoingDirectConnAlarm_S *alarm)
{
    // newModuleId 由调用者保证不为空
    CM_CHECK_RETURN(newModuleId != NULL, "newModuleId is null");
    SDF_MemFree(newModuleId);
    if (tArgs != NULL) {
        SDF_MemFree(tArgs);
    }
    if (alarm != NULL) {
        if (alarm->timerId != 0) {
            CP_TimerDel(alarm->timerId);
        }
        SDF_MemFree(alarm);
    }
}

static void CM_DoingConnDirectAfterInsertFailProc(
    const uint8_t *newModuleId, CM_AppConnectingDev_S *devNode, const SLE_Addr_S *addr)
{
    // newModuleId 由调用者保证不为空
    CM_CHECK_RETURN(newModuleId != NULL, "newModuleId is null");
    CM_EraseDoingDirectConnMap(devNode, *newModuleId);
    CM_TryDestroyConnectingDevEmptyNode(devNode, addr);
}

static void CM_DoingConnDirectSetTimerParam(SDF_TimerParam *param, CM_DoingDirectConnAlarm_S *alarm)
{
    param->expires = CM_DIRECT_CONN_TIMEOUT_MS;
    param->period = false,
    param->callback = CM_DirectConnTimeoutCbkInner;
    param->args = alarm;
}

static uint32_t CM_DoingConnDirectInsert(CM_AppConnectingDev_S *devNode, uint8_t moduleId, const SLE_Addr_S *addr,
    uint8_t frameType)
{
    uint32_t ret = CM_SUCCESS;
    SDF_TimerParam param = { 0 };
    uint8_t *newModuleId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    CM_CHECK_RETURN_RET(newModuleId != NULL, CM_MEM_ERR, "malloc for newModuleId failed");
    *newModuleId = moduleId;
    SDF_Map *doingDirectConnMap = devNode->doingDirectConnMap;
    CM_DirectConnTimoutArgs_S *tArgs = CM_DirectConnTimoutArgsNew(moduleId, addr);
    if (tArgs == NULL) {
        CM_LOGE("direct conn timeout args alloc failed");
        SDF_MemFree(newModuleId);
        return CM_MEM_ERR;
    }
    CM_DoingDirectConnAlarm_S *alarm = (CM_DoingDirectConnAlarm_S *)SDF_MemZalloc(sizeof(CM_DoingDirectConnAlarm_S));
    if (alarm == NULL) {
        goto CM_DOING_DIRECT_CONN_INSERT_FAILED;
    }
    CM_DoingConnDirectSetTimerParam(&param, alarm);
    ret = CP_TimerAdd(&alarm->timerId, &param);
    if (ret != SDF_OK) {
        CM_LOGE("create a timer failed, ret: 0x%08x", ret);
        goto CM_DOING_DIRECT_CONN_INSERT_FAILED;
    }
    alarm->args = tArgs;
    alarm->frameType = frameType;

    // 在添加新待连接设备之前，判断已有的待连接设备列表是否存在正在连接中的设备，若是则需要先取消连接后，再次发起连接
    if (!CM_ConnectingDevMapIsEmpty()) {
        // 此前存在连接中的设备，则先取消。等取消完成后，再发起建链
        // 注意: 若没有下发过Create_Connection命令，则Controller侧将返回一个命令不允许(0x0B)的错误码
        ret = CM_CurrentConnCancelReq();
        if (ret != CM_SUCCESS) {
            goto CM_DOING_DIRECT_CONN_INSERT_FAILED;
        }
    }
    if (!SDF_MapMoveInsert(doingDirectConnMap, newModuleId, alarm)) {
        CM_LOGE("add node to doing direct conn map failed, moduleId:%hhu", moduleId);
        goto CM_DOING_DIRECT_CONN_INSERT_FAILED;
    }
    devNode->sizeDoingDirect++;
    ret = CM_StartAllowListConnReq(addr);
    if (ret != CM_SUCCESS) {
        goto CM_DOING_DIRECT_CONN_AFTER_INSERT_FAILED;
    }
    CM_LOGI("insert a doing direct connecting node, moduleId:%hhu, timerId:%d, frameType:%hhu, doingDirect size:%zu",
        moduleId, alarm->timerId, alarm->frameType, devNode->sizeDoingDirect);
    return CM_SUCCESS;
CM_DOING_DIRECT_CONN_INSERT_FAILED:
    CM_DoingConnDirectInsertFailProc(newModuleId, tArgs, alarm);
    return CM_FAIL;
CM_DOING_DIRECT_CONN_AFTER_INSERT_FAILED:
    // newModuleId, tArgs, alarm 在从容器中删除时，释放内存
    CM_DoingConnDirectAfterInsertFailProc(newModuleId, devNode, addr);
    return CM_FAIL;
}

void CM_ConcurrentConnDeInit(void)
{
    COLLAB_CmDeInit();

    if (g_cmConnectingDevMap != NULL) {
        CM_LOGI("connection dev map dtor, size:%zu", g_cmSizeConnectingDev);
        SDF_MapDtor(g_cmConnectingDevMap);
        g_cmConnectingDevMap = NULL;
    }
    g_cmSizeConnectingDev = 0;

    CM_LOGI("connection manager deinit success");
}

static uint32_t CM_RestartAllowListConnReq(bool isNeedDoCancelBeforeConnect)
{
    uint32_t ret = CM_SUCCESS;
    if (isNeedDoCancelBeforeConnect) {
        // 此前存在连接中的设备，则先取消。等取消完成后，再发起建链
        // 注意: 若没有下发过Create_Connection命令，则Controller侧将返回一个命令不允许(0x0B)的错误码
        ret = CM_CurrentConnCancelReq();
        CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "cancel connect establish req failed");
    }
    ret = CM_StartAllowListConnReq(NULL);
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "start allow list conn req failed");
    return ret;
}

static bool CM_ConnCompleteProcAndCheckHasConnecting(const SLE_Addr_S *addr, uint8_t connectResult)
{
    // connectResult 有两种状态：已连接成功或者(因主动连接超时或者取消连接的)断开连接
    if (connectResult == CM_LINK_STATE_CONNECTED) {
        if (!SDF_MapErase(g_cmConnectingDevMap, (SLE_Addr_S *)addr)) {
            // 有可能是从别处建链来的
            CM_LOGE("dev:%s is not in connecting dev map exist", GET_ENC_ADDR(addr));
            return false;
        }
        g_cmSizeConnectingDev--;
        CM_LOGI("complete connect, erase a connecting dev map node, dev:%s, current size:%zu",
            GET_ENC_ADDR(addr), g_cmSizeConnectingDev);
    }
    return !CM_ConnectingDevMapIsEmpty();
}

void CM_ConcurrentConnDoingComplete(const SLE_Addr_S *addr, uint8_t connectResult)
{
    CM_CHECK_RETURN(addr != NULL, "param addr is null");
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    CM_LOGI("connection complete enter, addr:%s, connectResult:%hhu", GET_ENC_ADDR(addr), connectResult);

    if (!CM_IsEmptyAddr(addr) && SDF_MapFind(g_cmConnectingDevMap, (SLE_Addr_S *)addr) == NULL) {
        // 过滤正常主动断链和被动断链的地址，还有其他主动连接的地址
        return;
    }

    CM_LOGI("connection complete start, addr:%s", GET_ENC_ADDR(addr));
    if (CM_ConnCompleteProcAndCheckHasConnecting(addr, connectResult)) {
        // 未连接成功的，当前已全部取消完成，现在开始重新发起建链
        CM_LOGI("connection complete cancel ok, now start connect");
        // 注意: 在连接完成后，连接已经停止，不需要取消连接，否则会陷入死循环
        uint32_t ret = CM_StartAllowListConnReq(NULL);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "start allow list conn req failed");
    }
}

static CM_DoingBgConnNode_S *CM_DoingConnBgFind(CM_DoingBgConnSet_S *doingBgConnSet, uint8_t moduleId)
{
    CM_DoingBgConnNode_S *node = NULL;
    SDF_DListElmForeach(node, &(doingBgConnSet->list), entry)
    {
        if (node->moduleId == moduleId) {
            return node;
        }
    }
    return NULL;
}

static uint32_t CM_DoingConnBgInsert(CM_AppConnectingDev_S *connectingList, uint8_t moduleId,
    const CM_BgConnAddrParam_S *bgAddr)
{
    CM_DoingBgConnSet_S *doingBgConnSet = &connectingList->doingBgConnSet;
    CM_DoingBgConnNode_S *doingNode = CM_DoingConnBgFind(doingBgConnSet, moduleId);
    if (doingNode != NULL) {
        CM_LOGE("addr:%s, moduleId:%hhu has in doing bg conn set, doing isBypass:%d, new isBypass:%d",
            GET_ENC_ADDR(&bgAddr->addr), moduleId, doingNode->isBypass, bgAddr->isBypass);
        doingNode->isBypass = bgAddr->isBypass;
        return CM_SUCCESS;
    }

    CM_DoingBgConnNode_S *node = (CM_DoingBgConnNode_S *)SDF_MemZalloc(sizeof(CM_DoingBgConnNode_S));
    if (node == NULL) {
        return CM_MEM_ERR;
    }
    node->moduleId = moduleId;
    node->isBypass = bgAddr->isBypass;
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(&doingBgConnSet->list, node, entry);
    CM_LOGD("add a doing bg conn node, size:%u", SDF_DListCount(&doingBgConnSet->list));
    return CM_SUCCESS;
}

static uint32_t CM_DoingConnBgRemove(CM_AppConnectingDev_S *devNode, uint8_t moduleId, const SLE_Addr_S *addr)
{
    CM_DoingBgConnSet_S *doingBgConnSet = &devNode->doingBgConnSet;
    CM_DoingBgConnNode_S *node = CM_DoingConnBgFind(doingBgConnSet, moduleId);
    if (node == NULL) {
        CM_LOGE("addr:%s has not in doing bg conn set", GET_ENC_ADDR(addr));
        return CM_FAIL;
    }
    SDF_DListElmDel(&doingBgConnSet->list, node, entry);
    SDF_MemFree(node);
    node = NULL;
    CM_TryDestroyConnectingDevEmptyNode(devNode, addr);
    return CM_SUCCESS;
}

static void CM_DoingConnDirectRemove(uint8_t moduleId, const SLE_Addr_S *addr)
{
    CM_LOGI("already has a connecting device:%s, erase it and first cancel", GET_ENC_ADDR(addr));
    CM_AppConnectingDev_S *node = CM_ConnectingDevFind(addr);
    if (node == NULL) {
        CM_LOGE("dev:%s is not in connecting dev map exist", GET_ENC_ADDR(addr));
        CM_NotifyDirectConnectFailed(moduleId, CM_INVALID_LCID, addr, DLI_UNKNOWN_CONNECTION_IDENTIFIER);
        return;
    }
    // 删除连接中的设备前，通知一下对应的moduleId，取消成功，错误原因为0即可
    if (node->doingDirectConnMap != NULL) {
        for (SDF_MapIter *cur = node->doingDirectConnMap->entry; cur != NULL; cur = cur->next) {
            uint8_t doingModuleId = *(uint8_t *)cur->key;
            CM_LOGI("moduleId:%hhu, dev:%s is in doing direct conn dev map", doingModuleId, GET_ENC_ADDR(addr));
            CM_NotifyDirectConnectFailed(doingModuleId, CM_INVALID_LCID, addr, DLI_SUCCESS);
            // 此处需继续往下走
        }
    }
    CM_DoingBgConnNode_S *bgNode = NULL;
    SDF_DListElmForeach(bgNode, &(node->doingBgConnSet.list), entry)
    {
        CM_LOGI("moduleId:%hhu, dev:%s is in doing bg conn set", bgNode->moduleId, GET_ENC_ADDR(addr));
        CM_NotifyDirectConnectFailed(bgNode->moduleId, CM_INVALID_LCID, addr, DLI_SUCCESS);
        // 此处需继续往下走
    }
    if (!SDF_MapErase(g_cmConnectingDevMap, (SLE_Addr_S *)addr)) {
        CM_LOGE("dev:%s is not in connecting dev map exist", GET_ENC_ADDR(addr));
        return;
    }
    g_cmSizeConnectingDev--;
    CM_LOGI("connecting dev map size:%zu", g_cmSizeConnectingDev);
    CM_StartRmvAllowListConnReq();
}

static void CM_DestroyConnectingDevMap(uint8_t addrArrCount, CM_BgConnAddrParam_S *addrArr)
{
    for (uint8_t i = 0; i < addrArrCount; i++) {
        CM_BgConnAddrParam_S *bgAddr = &addrArr[i];
        if (!SDF_MapErase(g_cmConnectingDevMap, (SLE_Addr_S *)&bgAddr->addr)) {
            CM_LOGE("dev:%s is not in connecting dev map exist", GET_ENC_ADDR(&bgAddr->addr));
            continue;
        }
        g_cmSizeConnectingDev--;
        CM_LOGI("connecting dev map size:%zu", g_cmSizeConnectingDev);
    }
}

static uint32_t CM_BgConnectAddDoingConnBgList(uint8_t moduleId, uint8_t addrArrCount, CM_BgConnAddrParam_S *addrArr,
    uint32_t *changeCount)
{
    for (uint8_t i = 0; i < addrArrCount; i++) {
        CM_BgConnAddrParam_S *bgAddr = &addrArr[i];
        CM_LOGI("bg connect add index[%hhu], addr:%s, isBypass:%d", i, GET_ENC_ADDR(&bgAddr->addr), bgAddr->isBypass);
        CM_AppConnectingDev_S *node = CM_ConnectingDevFind(&bgAddr->addr);
        if (node != NULL) {
            CM_LOGW("addr:%s has in doing conn set", GET_ENC_ADDR(&bgAddr->addr));
            CM_DoingBgConnNode_S *doingNode = CM_DoingConnBgFind(&node->doingBgConnSet, moduleId);
            if (doingNode != NULL) {
                CM_LOGW("addr:%s, moduleId:%hhu has in doing bg conn set, doing isBypass:%d, new isBypass:%d",
                    GET_ENC_ADDR(&bgAddr->addr), moduleId, doingNode->isBypass, bgAddr->isBypass);
                doingNode->isBypass = bgAddr->isBypass;
                (*changeCount)++; // 有需要刷新到isBypass标识
                CM_NotifyConnecting(moduleId, &bgAddr->addr);
                // 目标地址已经在对应模块的背景连接中，则不再添加
                continue;
            } else {
                // 目标地址如果不在背景连接中，但在主动连接中，可继续添加到背景连接列表中。当主动连接超时时，需要继续进行背景连接
                CM_NotifyConnecting(moduleId, &bgAddr->addr);
            }
        }
        SleLogicLink_S *link = SleLogicLinkGetByAddr(&bgAddr->addr);
        if (link != NULL && link->status == CM_LINK_STATE_CONNECTED) {
            CM_LOGE("already has the connected dev:%s", GET_ENC_ADDR(&bgAddr->addr));
            // 对于已添加的设备，若此时本端与对端设备建链已完成，则忽略通知该用户CONNECTED事件
            continue;
        }
        CM_AppConnectingDev_S *appConnectingDev = CM_CreateAppModuleConnectingDev(&bgAddr->addr);
        if (appConnectingDev == NULL) {
            CM_LOGE("generate a app module connecting device failed");
            return CM_FAIL;
        }
        appConnectingDev->moduleId = moduleId;
        CM_LOGI("bg conn add, moduleId:%hhu, addr:%s, isBypass:%d", moduleId,
            GET_ENC_ADDR(&bgAddr->addr), bgAddr->isBypass);
        if (CM_DoingConnBgInsert(appConnectingDev, moduleId, bgAddr) != CM_SUCCESS) {
            return CM_FAIL;
        }
        (*changeCount)++;
    }
    return CM_SUCCESS;
}

static void CM_BgConnectAddInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    CM_CHECK_RETURN((g_cmSizeConnectingDev <= CM_MAX_CONNECTING_DEV_NUM), "connecting dev size:%zu has reached max "
        "num limit, ignored the req", g_cmSizeConnectingDev);

    CM_BgConnectAddReq_S *req = (CM_BgConnectAddReq_S *)arg;
    uint8_t moduleId = req->moduleId;
    uint8_t addrArrCount = req->addrArrCount;
    CM_BgConnAddrParam_S *addrArr = (CM_BgConnAddrParam_S *)SDF_MemZalloc(addrArrCount * sizeof(CM_BgConnAddrParam_S));
    CM_CHECK_RETURN(addrArr != NULL, "mem zalloc addr arr failed");
    (void)memcpy_s(addrArr, (addrArrCount * sizeof(CM_BgConnAddrParam_S)),
        req->addrArr, (addrArrCount * sizeof(CM_BgConnAddrParam_S)));
    bool isNeedDoCancelBeforeConnect = !CM_ConnectingDevMapIsEmpty();

    uint32_t changeCount = 0;
    uint32_t ret = CM_BgConnectAddDoingConnBgList(moduleId, addrArrCount, addrArr, &changeCount);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_BgConnectAddDoingConnBgList failed, ret:0x%08x", ret);
        goto CM_BG_CONNECT_ADD_INNER_FAILED;
    }
    if (changeCount == 0) {
        CM_LOGI("current bg conn change no new address to connecting dev list");
        SDF_MemFree(addrArr);
        return;
    }
    ret = CM_RestartAllowListConnReq(isNeedDoCancelBeforeConnect);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_RestartAllowListConnReq failed, ret:0x%08x", ret);
        goto CM_BG_CONNECT_ADD_INNER_FAILED;
    }
    CM_LOGI("insert a doing bg connecting node, moduleId:%hhu", moduleId);
    SDF_MemFree(addrArr);
    return;
CM_BG_CONNECT_ADD_INNER_FAILED:
    CM_DestroyConnectingDevMap(addrArrCount, addrArr);
    SDF_MemFree(addrArr);
}

uint32_t CM_BackgroundConnectAdd(uint8_t moduleId, uint8_t addrArrCount, const CM_BgConnAddrParam_S *addrArr)
{
    CM_CHECK_RETURN_RET(moduleId == CM_MODULE_ADPT, CM_INVALID_PARAM_ERR, "moduleId:%hhu is invalid", moduleId);
    CM_CHECK_RETURN_RET((addrArrCount > 0) && (addrArrCount <= CM_BG_CONN_MAX_ADDR_SIZE) && (addrArr != NULL),
        CM_INVALID_PARAM_ERR, "param addrArrCount[%hhu] is invalid", addrArrCount);
    CM_LOGI("concurrent conn api, background connect add, moduleId:%hhu, addrArrCount:%hhu", moduleId, addrArrCount);
    uint32_t ret;
    CM_BgConnectAddReq_S *reqParam = (CM_BgConnectAddReq_S *)SDF_MemZalloc(sizeof(CM_BgConnectAddReq_S));
    CM_CHECK_RETURN_RET(reqParam != NULL, CM_MEM_ERR, "mem zalloc error");
    reqParam->moduleId = moduleId;
    reqParam->addrArr = (CM_BgConnAddrParam_S *)SDF_MemZalloc(addrArrCount * sizeof(CM_BgConnAddrParam_S));
    if (reqParam->addrArr == NULL) {
        SDF_MemFree(reqParam);
        return CM_MEM_ERR;
    }
    (void)memcpy_s(reqParam->addrArr, addrArrCount * sizeof(CM_BgConnAddrParam_S), addrArr,
        addrArrCount * sizeof(CM_BgConnAddrParam_S));
    reqParam->addrArrCount = addrArrCount;
    ret = CP_PostTask((SDF_WorkCb)CM_BgConnectAddInner, (void *)reqParam, (SDF_FreeWorkArg)CM_BgConnectReqDtor);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_BgConnectRmvInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    uint32_t ret;
    CM_BgConnectRmvReq_S *req = (CM_BgConnectRmvReq_S *)arg;
    uint8_t moduleId = req->moduleId;
    SLE_Addr_S *addr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CM_CHECK_RETURN(addr != NULL, "mem zalloc addr arr failed");
    (void)memcpy_s(addr, sizeof(SLE_Addr_S), &req->addr, (sizeof(SLE_Addr_S)));

    CM_LOGI("bg conn remove, moduleId:%hhu, addr:%s", moduleId, GET_ENC_ADDR(addr));
    CM_AppConnectingDev_S *appConnectingDev = CM_ConnectingDevFind(addr);
    if (appConnectingDev == NULL) {
        CM_LOGE("generate a app module connecting device failed");
        // 删除无效地址，不通知用户，静默处理即可
        goto CM_BG_CONNECT_RMV_INNER_FAILED;
    }
    ret = CM_DoingConnBgRemove(appConnectingDev, moduleId, addr);
    if (ret != CM_SUCCESS) {
        CM_LOGE("doing conn bg remove failed");
        goto CM_BG_CONNECT_RMV_INNER_FAILED;
    }
    CM_LOGI("remove a doing bg connecting node, moduleId:%hhu", moduleId);
    ret = CM_StartRmvAllowListConnReq();
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_StartRmvAllowListConnReq failed, ret:0x%08x", ret);
        goto CM_BG_CONNECT_RMV_INNER_FAILED;
    }
    SDF_MemFree(addr);
    return;
CM_BG_CONNECT_RMV_INNER_FAILED:
    // 若有内存失败，等系统析构时，一次性释放记录在全局变量里的内存
    SDF_MemFree(addr);
}

uint32_t CM_BackgroundConnectRemove(uint8_t moduleId, const SLE_Addr_S *addr)
{
    CM_CHECK_RETURN_RET(moduleId == CM_MODULE_ADPT, CM_INVALID_PARAM_ERR, "moduleId:%hhu is invalid", moduleId);
    CM_CHECK_RETURN_RET((addr != NULL), CM_INVALID_PARAM_ERR, "addr is null");
    CM_LOGI("concurrent conn api, background connect remove, moduleId:%hhu, addr:%s", moduleId, GET_ENC_ADDR(addr));
    CM_BgConnectRmvReq_S *reqParam = (CM_BgConnectRmvReq_S *)SDF_MemZalloc(sizeof(CM_BgConnectRmvReq_S));
    CM_CHECK_RETURN_RET(reqParam != NULL, CM_MEM_ERR, "mem zalloc error");
    reqParam->moduleId = moduleId;
    (void)memcpy_s(&reqParam->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = CP_PostTask((SDF_WorkCb)CM_BgConnectRmvInner, (void *)reqParam, (SDF_FreeWorkArg)CM_SimpleDataDtor);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_DoingConnBgClear(uint8_t moduleId)
{
    CM_LOGI("bg clear, connecting dev current size:%zu", g_cmSizeConnectingDev);
    SDF_MapIter *cur = g_cmConnectingDevMap->entry;
    while (cur != NULL) {
        CM_AppConnectingDev_S *connectingList = (CM_AppConnectingDev_S *)cur->val;
        SDF_MapIter *next = cur->next;
        CM_DoingBgConnSet_S *doingBgConnSet = &connectingList->doingBgConnSet;
        CM_DoingBgConnNode_S *node = CM_DoingConnBgFind(doingBgConnSet, moduleId);
        if (node == NULL) {
            CM_LOGW("moduleId:%hhu has not in doing bg conn set", moduleId);
        } else {
            SDF_DListElmDel(&doingBgConnSet->list, node, entry);
            SDF_MemFree(node);
            node = NULL;
        }
        CM_LOGI("remove a doing bg conn node, size:%d", SDF_DListCount(&doingBgConnSet->list));
        CM_TryDestroyConnectingDevEmptyNode(connectingList, (const SLE_Addr_S *)cur->key);
        cur = next;
    }
}

static void CM_BgConnectClearInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    uint32_t ret;
    CM_BgConnectClearReq_S *req = (CM_BgConnectClearReq_S *)arg;
    uint8_t moduleId = req->moduleId;

    CM_LOGI("bg conn clear, moduleId:%hhu", moduleId);
    CM_DoingConnBgClear(moduleId);
    ret = CM_StartRmvAllowListConnReq();
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_StartRmvAllowListConnReq failed, ret:0x%08x", ret);
        return;
    }
}

uint32_t CM_BackgroundConnectClear(uint8_t moduleId)
{
    CM_CHECK_RETURN_RET(moduleId == CM_MODULE_ADPT, CM_INVALID_PARAM_ERR, "moduleId:%hhu is invalid", moduleId);
    CM_LOGI("concurrent conn api, background connect clear, moduleId:%hhu", moduleId);
    CM_BgConnectClearReq_S *reqParam = (CM_BgConnectClearReq_S *)SDF_MemZalloc(sizeof(CM_BgConnectClearReq_S));
    CM_CHECK_RETURN_RET(reqParam != NULL, CM_MEM_ERR, "mem zalloc error");
    reqParam->moduleId = moduleId;
    uint32_t ret =
        CP_PostTask((SDF_WorkCb)CM_BgConnectClearInner, (void *)reqParam, (SDF_FreeWorkArg)CM_SimpleDataDtor);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_NotifyDisconnecting(uint8_t moduleId, uint16_t lcid, const SLE_Addr_S *addr)
{
    CM_LOGI("notify link state disconnecting, lcid:%04x, addr:%s", lcid, GET_ENC_ADDR(addr));
    CM_LogicLinkState_S param = {0};
    param.lcid = lcid;
    param.role = 0;
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param.result = CM_LINK_STATE_DISCONNECTTING;
    param.discReason = 0;  // 断连中，断连原因取0即可
    CM_ExecLogicLinkModuleCbks(moduleId, &param);
}

static uint32_t CM_DirectDoingConnRestartTimer(CM_DoingDirectConnAlarm_S *connAlarm)
{
    CP_TimerDel(connAlarm->timerId);
    SDF_TimerParam param = { 0 };
    CM_DoingConnDirectSetTimerParam(&param, connAlarm);
    uint32_t ret = CP_TimerAdd(&connAlarm->timerId, &param);
    if (ret != SDF_OK) {
        CM_LOGE("create a timer failed, ret: 0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static uint32_t CM_DirectConnectDoingConnList(uint8_t moduleId, const SLE_Addr_S *addr, uint8_t frameType)
{
    SDF_Map *doingDirectConnMap = NULL;
    // step: 1）将当前需要连接的设备插入到待连接设备列表中，并同时启动一个定时器，
    //       2）由step 1) 结果进行发起或者取消连接操作
    CM_AppConnectingDev_S *devNode = CM_CreateAppModuleConnectingDev(addr);
    if (devNode == NULL) {
        CM_LOGE("generate a app module connecting device failed");
        return CM_FAIL;
    }
    devNode->moduleId = moduleId;
    CM_LOGI("direct conn add, moduleId:%hhu, addr:%s, frameType:%hhu", moduleId, GET_ENC_ADDR(addr), frameType);
    doingDirectConnMap = devNode->doingDirectConnMap;
    if (doingDirectConnMap == NULL) {
        doingDirectConnMap = CM_CreateDoingDirectConnMap();
        CM_CHECK_RETURN_RET(doingDirectConnMap != NULL, CM_FAIL, "malloc for node doingDirectConnMap failed");
        devNode->doingDirectConnMap = doingDirectConnMap;
        devNode->sizeDoingDirect = 0;
    }
    if (devNode->sizeDoingDirect != 0) {
        SDF_MapIter *iter = SDF_MapFind(devNode->doingDirectConnMap, &moduleId);
        if (iter != NULL) {
            CM_DoingDirectConnAlarm_S *connAlarm = (CM_DoingDirectConnAlarm_S *)iter->val;
            int oldTimerId = connAlarm->timerId;
            // 最新触发主动连接后，需要重新计时
            uint32_t ret = CM_DirectDoingConnRestartTimer(connAlarm);
            CM_CHECK_RETURN_RET(ret == CM_SUCCESS, CM_FAIL, "restart conn alarm failed");
            CM_NotifyConnecting(moduleId, addr);
            CM_LOGI("doing direct connecting node moduleId:%hhu, map size:%zu, timerId old:%d, new:%d, frameType:%hhu",
                moduleId, devNode->sizeDoingDirect, oldTimerId, connAlarm->timerId, connAlarm->frameType);
            return CM_SUCCESS;
        }
    }
    uint32_t sizeDoingBg = SDF_DListCount(&devNode->doingBgConnSet.list);
    CM_LOGI("doing bg conn set size:%d, doing direct conn map size:%zu", sizeDoingBg, devNode->sizeDoingDirect);
    if (sizeDoingBg != 0) {
        CM_NotifyConnecting(moduleId, addr);
        // 1. 直接返回，会导致若可能主动连接一个已经背景连接过的但又不存在的地址，不会超时
        // 2. 但此时下发连接指令后，会提示连接已存在，依靠取消连接返回后，重新启动发起连接
    }
    return CM_DoingConnDirectInsert(devNode, moduleId, addr, frameType);
}

static void CM_DirectConnectAddInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    CM_CHECK_RETURN((g_cmSizeConnectingDev <= CM_MAX_CONNECTING_DEV_NUM), "connecting dev size:%zu has reached max "
        "num limit, ignored the req", g_cmSizeConnectingDev);

    CM_DirectConnectAddReq_S *req = (CM_DirectConnectAddReq_S *)arg;
    uint8_t moduleId = req->moduleId;
    uint8_t discReason = DLI_MEMORY_CAPACITY_EXCEEDED;
    SLE_Addr_S *addr = &req->addr;
    uint32_t ret = CM_SUCCESS;
    SleLogicLink_S *link = SleLogicLinkGetByAddr(addr);
    if (link != NULL && link->status == CM_LINK_STATE_CONNECTED) {
        CM_LOGE("already has the connected dev:%s", GET_ENC_ADDR(addr));
        // 对于已添加的设备，若此时本端与对端设备建链已完成，则忽略通知该用户CONNECTED事件
        return;
    }

    ret = CM_DirectConnectDoingConnList(moduleId, addr, req->frameType);
    if (ret != CM_SUCCESS) {
        CM_LOGE("doing conn direct insert failed, ret:0x%08x", ret);
        goto CM_DIRECT_CONNECT_ADD_INNER_FAILED;
    }
    return;
CM_DIRECT_CONNECT_ADD_INNER_FAILED:
    CM_NotifyDirectConnectFailed(req->moduleId, CM_INVALID_LCID, addr, discReason);
    CM_BgConnAddrParam_S addrArr = { .addr = *addr, .isBypass = false };
    CM_DestroyConnectingDevMap(1, &addrArr);
}

uint32_t CM_DirectConnectAdd(uint8_t moduleId, const CM_DirectConnAddrParam_S *param)
{
    CM_CHECK_RETURN_RET(((moduleId == CM_MODULE_ADPT) || (moduleId == CM_MODULE_SSAP) || (moduleId == CM_MODULE_BNL)),
        CM_INVALID_PARAM_ERR, "moduleId:%hhu is invalid", moduleId);
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    CM_CHECK_RETURN_RET((param->frameType == CM_CONN_PARAM_FRAME_TYPE_1 ||
        param->frameType == CM_CONN_PARAM_FRAME_TYPE_4),
        CM_INVALID_PARAM_ERR, "param type:%hhu is not support", param->frameType);
    CM_LOGI("concurrent conn api, direct connect add, moduleId:%hhu, addr:%s, frameType:%hhu",
        moduleId, GET_ENC_ADDR(&param->addr), param->frameType);
    CM_DirectConnectAddReq_S *reqParam = (CM_DirectConnectAddReq_S *)SDF_MemZalloc(sizeof(CM_DirectConnectAddReq_S));
    CM_CHECK_RETURN_RET(reqParam != NULL, CM_MEM_ERR, "mem zalloc error");
    reqParam->moduleId = moduleId;
    (void)memcpy_s(&reqParam->addr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    reqParam->frameType = param->frameType;
    uint32_t ret =
        CP_PostTask((SDF_WorkCb)CM_DirectConnectAddInner, (void *)reqParam, (SDF_FreeWorkArg)CM_SimpleDataDtor);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static void CM_DirectConnectRmvInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    CM_DirectConnectRemoveReq_S *reqParam = (CM_DirectConnectRemoveReq_S *)arg;
    uint8_t moduleId = reqParam->moduleId;
    SLE_Addr_S *addr = &reqParam->addr;
    uint8_t discReason = reqParam->discReason;
    SleLogicLink_S *link = NULL;

    // step1: 先判断该地址当前已连接后的状态
    link = SleLogicLinkGetByAddr(addr);
    if (link != NULL) {
        if (link->status == CM_LINK_STATE_CONNECTED) {
            CM_DisconnectParamReq_S disParam = {0};
            CM_LOGI("conn remove, moduleId:%hhu, addr:%s", moduleId, GET_ENC_ADDR(addr));
            (void)memcpy_s(&disParam.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
            disParam.discReason = discReason;
            uint32_t ret = CM_ConnectReleaseReq(&disParam);
            if (ret != CM_SUCCESS) {
                CM_LOGI("connect release req failed, ret:0x%08x", ret);
                CM_NotifyDirectConnectFailed(moduleId, CM_INVALID_LCID, addr, DLI_INVALID_PARAMETERS);
                return;
            }
            return;
        } else if (link->status == CM_LINK_STATE_DISCONNECTTING) {
            CM_NotifyDisconnecting(moduleId, link->lcid, addr);
            return;
        }
    }
    // step2: 若存在正在连接中的设备，则需要取消连接
    CM_DoingConnDirectRemove(moduleId, addr);
}

uint32_t CM_DirectConnectRemove(uint8_t moduleId, const SLE_Addr_S *addr, uint8_t discReason)
{
    CM_CHECK_RETURN_RET(((moduleId == CM_MODULE_ADPT) || (moduleId == CM_MODULE_SSAP) || (moduleId == CM_MODULE_BNL)),
        CM_INVALID_PARAM_ERR, "moduleId:%hhu is invalid", moduleId);
    CM_CHECK_RETURN_RET((addr != NULL), CM_INVALID_PARAM_ERR, "addr is null");
    CM_CHECK_RETURN_RET(((discReason == CM_DISC_REASON_REMOTE_USER_TERMINATED) ||
        (discReason == CM_DISC_REASON_CANCEL_PAIR) || (discReason == CM_DISC_REASON_COMMAND_TIMEOUT)),
        CM_INVALID_PARAM_ERR, "discReason:0x%02x is invalid", discReason);
    CM_LOGI("concurrent conn api, direct connect remove, moduleId:%hhu, addr:%s, discReason:0x%02x",
        moduleId, GET_ENC_ADDR(addr), discReason);
    CM_DirectConnectRemoveReq_S *reqParam =
        (CM_DirectConnectRemoveReq_S *)SDF_MemZalloc(sizeof(CM_DirectConnectRemoveReq_S));
    CM_CHECK_RETURN_RET(reqParam != NULL, CM_MEM_ERR, "mem zalloc error");
    reqParam->moduleId = moduleId;
    (void)memcpy_s(&reqParam->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    reqParam->discReason = discReason;
    uint32_t ret =
        CP_PostTask((SDF_WorkCb)CM_DirectConnectRmvInner, (void *)reqParam, (SDF_FreeWorkArg)CM_SimpleDataDtor);
    if (ret != CP_OK) {
        CM_LOGE("CP_PostTask failed, ret:0x%08x", ret);
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

static bool CM_IsNeedNotifyLinkCollabResult(void)
{
    if (!CM_IsNeedLinkCollabReq()) {
        return false;
    }
    if (!CM_ConnectingDevMapIsEmpty()) {
        CM_LOGI("connecting dev list is not empty");
        return false;
    }
    return true;
}

void CM_ConcurrentConnNotifyLinkCollabResult(void)
{
    CM_CHECK_RETURN(g_cmConnectingDevMap != NULL, "CM has not inited, connecting dev map is null.");
    if (CM_IsNeedNotifyLinkCollabResult()) {
        if (!CM_NotifyLinkCollabResult()) {
            CM_LOGW("notify link collab result failed");
        }
    }
}