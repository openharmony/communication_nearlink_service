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
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "sdf_vector.h"
#include "cpfwk_log.h"
#include "nbc_api.h"
#include "ssap_common.h"
#include "ssap_pkt.h"
#include "ssap_link.h"
#include "ssap_utils.h"
#include "ssapc_app.h"
#include "nlstk_devd_api.h"
#include "nlstk_cfgdb.h"
#include "cdsm_tbl.h"
#include "cdsm_event.h"
#include "cdsm_encp.h"
#include "devd_adv.h"
#include "devd_local.h"
#include "cdsm_control.h"

#define UUID_FIFTEENTH_BYTE_INDEX 14
#define UUID_SIXTEENTH_BYTE_INDEX 15
#define UUID_BASE_LEN 14

#define CDSM_MEMBER_NUM_LEN 1
#define CDSM_MEMBER_ADDR_LEN(coopSet) (((coopSet)->num - 1) * (SLE_ADDR_LEN + 1) + 1)

#define ADV_EVENT_ENABLE            3
#define ADV_EVENT_DISABLE           4
#define ADV_EVENT_REMOVE            5
#define ADV_EVENT_TERMINATED        7

// 广播参数
#define CDSM_SLE_ADV_ACCESS_MODE_SLE 0x02
#define CDSM_SLE_ADV_FLAG_GENERAL_DISC 0x01
#define CDSM_SLE_ADV_GT_ROLE_G_CAN_NOT_NEGO 0x03
#define CDSM_ADV_INTERVAL_MIN 0x0000A0
#define CDSM_ADV_INTERVAL_DEFAULT 0x001388
#define CDSM_ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY 0x00
#define CDSM_ADV_CHNL_ALL 0x07
#define CDSM_CM_CONN_PRIVATE_MIN_INTERVAL 0x50
#define CDSM_CM_CONN_PRIVATE_MAX_INTERVAL 0x50
#define CDSM_CM_CONN_MAX_LATENCY 0x00
#define CDSM_CM_CONN_PRIVATE_TIMEOUT 0xC8

// 广播数据
#define ADV_DATA_LEN 18                 // 广播数据长度
#define CDSM_ADV_DATA_TYPE 0x03         // 合作集广播数据类型
#define CDSM_ADV_DATA_LEN 16            // 合作集广播数据长度
#define CDSM_DEV_SET_DATA_TYPE 0x11     // 定向连接合作设备集合邀请

#define CDSM_READ_PROPERTY_NUM 4
#define OCTETS_0               0
#define OCTETS_1               1
#define OCTETS_2               2
#define OCTETS_3               3

typedef enum {
    CDSM_SERVICE_UUID = 0x0600,
    CDSM_KEY_INFO_UUID = 0x1000,
    CDSM_MEMBER_NUM_UUID = 0x1001,
    CDSM_COOPSET_ROLE_UUID = 0x1002,
    CDSM_DISCOVER_MOD_UUID = 0x1003,
    CDSM_SERVICE_TABLE_UUID = 0x1004,
    CDSM_MEMBER_ADDR_UUID = 0x107A,
    CDSM_KEY_UUID = 0x1079,
} CdsmUuid_E;

typedef struct {
    uint16_t keyInfoHandle;
    uint16_t keyHandle;
    uint16_t memberNumHandle;
    uint16_t coopsetRoleHandle;
    uint16_t discModHandle;
    uint16_t srvcTableHandle;
    uint16_t memberAddrHandle;
} CdsmCacheService_S;

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint16_t CdsmConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[UUID_FIFTEENTH_BYTE_INDEX]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[UUID_SIXTEENTH_BYTE_INDEX]);
    return uuid;
}

NLSTK_SsapUuid_S CdsmConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < UUID_FIFTEENTH_BYTE_INDEX; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[UUID_FIFTEENTH_BYTE_INDEX] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[UUID_SIXTEENTH_BYTE_INDEX] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

static void BuildCdsmServiceCache(CdsmCacheService_S *cache, NLSTK_SsapPrty_S *property)
{
    uint16_t uuid = CdsmConvertUuidTo16Bits(property->uuid);
    CP_LOG_INFO("[CDSM] uuid = 0x%x", uuid);
    switch (uuid) {
        case CDSM_KEY_INFO_UUID:
            cache->keyInfoHandle = property->handle;
            break;
        case CDSM_MEMBER_NUM_UUID:
            cache->memberNumHandle = property->handle;
            break;
        case CDSM_COOPSET_ROLE_UUID:
            cache->coopsetRoleHandle = property->handle;
            break;
        case CDSM_DISCOVER_MOD_UUID:
            cache->discModHandle = property->handle;
            break;
        case CDSM_SERVICE_TABLE_UUID:
            cache->srvcTableHandle = property->handle;
            break;
        case CDSM_MEMBER_ADDR_UUID:
            cache->memberAddrHandle = property->handle;
            break;
        case CDSM_KEY_UUID:
            cache->keyHandle = property->handle;
            break;
        default:
            CP_LOG_ERROR("[CDSM] unknown uuid");
            break;
    }
}

static void CdsmAdvEventResultCbk(NLSTK_DevdAdvCbkParam_S *param)
{
    CP_LOG_INFO("enter CdsmAdvEventResultCbk");
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[CDSM] param is null");
    uint8_t event = param->event;
    uint8_t advHandle = param->advHandle;
    uint8_t result = param->result;
    CdsmCoopSet_S *coopSet = NULL;
    CP_LOG_INFO("[CDSM] event = %u, handle = %u, result = %u", event, advHandle, result);
    switch (event) {
        case ADV_EVENT_ENABLE:
            if (result != 0) {
                CP_LOG_ERROR("[CDSM] enable adv failed! result=%u", result);
                CP_CHECK_LOG_RETURN_VOID(NLSTK_DevdRemoveAdv(&advHandle) == NLSTK_ERRCODE_SUCCESS,
                    "[CDSM] remove adv failed when recv error enable event cbk!");
                return;
            }
            break;
        case ADV_EVENT_DISABLE:
        case ADV_EVENT_TERMINATED:
            CP_CHECK_LOG_RETURN_VOID(NLSTK_DevdRemoveAdv(&advHandle) == NLSTK_ERRCODE_SUCCESS,
                "[CDSM] remove adv failed when recv disable or terminate event cbk!");
            break;
        case ADV_EVENT_REMOVE:
            coopSet = CdsmFindCoopSetByAdvHandle(advHandle);
            CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] not find coopSet");
            coopSet->advHandle = INVALID_ADV_HANDLE;
            break;
        default:
            break;
    }
}

static void CdsmSetAdvData(CdsmCoopSet_S *coopSet, NLSTK_DevdSetAdvParams_S *setParam)
{
    CP_CHECK_LOG_RETURN_VOID(setParam != NULL, "[CDSM] setParam is null");
    NLSTK_DevdAdvData_S *data = &setParam->data;
    data->advDataLen = ADV_DATA_LEN;
    data->advData = (uint8_t *)SDF_MemZalloc(data->advDataLen);
    CP_CHECK_LOG_RETURN_VOID(data->advData != NULL, "[CDSM] advData malloc fail");
    uint8_t *advData = data->advData;
    (*advData++) = CDSM_ADV_DATA_TYPE;
    (*advData++) = CDSM_ADV_DATA_LEN;
    uint16_t uuid = CDSM_SERVICE_UUID;
    (void)memcpy_s(advData, sizeof(uuid), &uuid, sizeof(uuid));
    advData += sizeof(uuid);
    (*advData++) = CDSM_DEV_SET_DATA_TYPE;
    (*advData++) = 0; // 设备类型非必选以0填充
    (*advData++) = coopSet->keyId;
    (*advData++) = coopSet->algoId;
    uint16_t rand = 0;
    uint32_t setId = 0;
    CDSM_GenCoopSetId(coopSet->key, CDSM_KEY_LEN, coopSet->algoId, &rand, &setId);
    (void)memcpy_s(advData, sizeof(uint16_t), &rand, sizeof(uint16_t));
    advData += sizeof(uint16_t);
    (void)memcpy_s(advData, sizeof(uint32_t), &setId, sizeof(uint32_t));
    advData += sizeof(uint32_t);
    CP_LOG_DEBUG("[CDSM] rand: 0x%x, setId: 0x%x", rand, setId);
    (*advData++) = 0; // 成员数量非必选以0填充
    (*advData++) = 0; // RFU
    (*advData++) = 0; // 标准服务数量非必选以0填充
    (*advData++) = 0; // 自定义服务数量非必选以0填充
    data->scanRspDataLen = data->advDataLen;
    data->scanRspData = (uint8_t *)SDF_MemZalloc(data->scanRspDataLen);
    if (data->scanRspData == NULL) {
        CP_LOG_ERROR("[CDSM] scanRspData malloc fail");
        SDF_MemFree(data->advData);
        data->advData = NULL;
        return;
    }
    (void)memcpy_s(data->scanRspData, data->scanRspDataLen, data->advData, data->advDataLen);
    CP_LOG_DEBUG("[CDSM] adv data: %s", SDF_GET_UINT8_STR(data->advData, data->advDataLen));
}

static CdsmCoopSetMeb_S *GetFirstDisconnectedMeb(CdsmCoopSet_S *coopSet)
{
    CdsmCoopSetMeb_S *meb = NULL;
    for (size_t i = 0; i < coopSet->mebs->size; i++) {
        meb = SDF_VectorElementAt(coopSet->mebs, i);
        if (meb->state == CDSM_DISCONNECTED) {
            return meb;
        }
    }
    return NULL;
}

void CdsmStartAdv(SLE_Addr_S *addr)
{
    CP_LOG_INFO("enter CdsmStartAdv");
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CdsmCoopSetMeb_S *meb = GetFirstDisconnectedMeb(coopSet);
    CP_CHECK_LOG_RETURN_VOID(meb != NULL, "[CDSM] all members connect");
    if (coopSet->advHandle != INVALID_ADV_HANDLE) {
        CP_LOG_INFO("[CDSM] already created adv,handle:%u", coopSet->advHandle);
        return;
    }
    coopSet->advHandle = DevdCreateAdvHandle(CdsmAdvEventResultCbk);
    CP_CHECK_LOG_RETURN_VOID(coopSet->advHandle != INVALID_ADV_HANDLE, "[CDSM] no valid handle");
    NLSTK_DevdSetAdvParams_S setParam = {0};
    NLSTK_DevdConnParam_S connParam = {0};
    setParam.accessMode = CDSM_SLE_ADV_ACCESS_MODE_SLE;
    setParam.discoveryLevel = CDSM_SLE_ADV_FLAG_GENERAL_DISC;
    setParam.param.advHandle = coopSet->advHandle;
    setParam.param.advMode = ADV_MODE_CONNECTABLE_DIRECTED;
    setParam.param.advGtRole = ADV_GT_ROLE_G_CAN_NEGO; /* 合作集邀请广播暂定为：手机作为G节点，可协商 */
    setParam.param.advIntervalMin = CDSM_ADV_INTERVAL_MIN;       // ADV_INTERVAL_MIN
    setParam.param.advIntervalMax = CDSM_ADV_INTERVAL_DEFAULT;   // ADV_INTERVAL_DEFAULT
    setParam.param.advChannelMap = CDSM_ADV_CHNL_ALL;
    (void)memcpy_s(&setParam.param.ownAddr, sizeof(SLE_Addr_S), NBC_GetPublicAddress(), sizeof(SLE_Addr_S));
    connParam.intervalMin = CDSM_CM_CONN_PRIVATE_MIN_INTERVAL;
    connParam.intervalMax = CDSM_CM_CONN_PRIVATE_MAX_INTERVAL;
    connParam.maxLatency = CDSM_CM_CONN_MAX_LATENCY;
    connParam.supervisionTimeout = CDSM_CM_CONN_PRIVATE_TIMEOUT;
    setParam.param.connParam = &connParam;
    (void)memcpy_s(&setParam.param.peerAddr, sizeof(SLE_Addr_S), &meb->addr, sizeof(SLE_Addr_S));
    CdsmSetAdvData(coopSet, &setParam);
    if (NLSTK_DevdStartAdv(&setParam) != NLSTK_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[CDSM] start adv failed!");
        DevdRemoveAdvNode(coopSet->advHandle, DEVD_ADV_LIST);   // 释放DevdCreateAdvHandle时申请的node的空间
    }
    SDF_MemFree(setParam.data.advData);
    SDF_MemFree(setParam.data.scanRspData);
}

void CdsmStopAdv(CdsmCoopSet_S *coopSet)
{
    CP_LOG_INFO("enter CdsmStopAdv");
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CP_CHECK_LOG_RETURN_VOID(coopSet->advHandle != INVALID_ADV_HANDLE, "[CDSM] not start adv");
    NLSTK_DevdSetAdvEnable_S advEnable = {0};
    advEnable.advHandle = coopSet->advHandle;
    if (NLSTK_DevdEnableAdv(&advEnable) != NLSTK_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[CDSM] stop adv failed!");
    }
}

void CdsmConnectProfile(CdsmCoopSetMeb_S *setMeb)
{
    CP_LOG_DEBUG("enter CdsmConnectProfile");
    CP_CHECK_LOG_RETURN_VOID(setMeb != NULL, "[CDSM] setMeb is null");
    if (setMeb->state == CDSM_CONNECTED) {
        CdsmNotifyStateChange(&setMeb->addr, CDSM_EVENT_CONNECT);
        return;
    } else if (setMeb->state != CDSM_DISCONNECTED) {
        // 当前ssap没有超时处理，后续添加超时处理需要通知这里
        setMeb->state = CDSM_DISCONNECTED;
        CP_LOG_ERROR("[CDSM] read state not idle");
    }
    setMeb->state = CDSM_ACB_CONNECTING;
    CP_CHECK_LOG_RETURN_VOID(NLSTK_SsapClientConnect(setMeb->appId) == NLSTK_ERRCODE_SUCCESS,
        "[CDSM] ssap client connnect failed");
}

static void CdsmReadKeyInfoHandle(CdsmCoopSet_S *coopSet, CdsmCoopSetMeb_S *setMeb, NLSTK_VariableData_S *value)
{
    CP_CHECK_LOG_RETURN_VOID(value->len == CDSM_KEY_INFO_LEN, "[CDSM] key info len error");
    coopSet->keyId = value->data[0];
    coopSet->algoId = value->data[1];
    setMeb->state = CDSM_READ_KEY_INFO_FINISH;
}

static void CdsmReadMemberNumHandle(CdsmCoopSet_S *coopSet, CdsmCoopSetMeb_S *setMeb,
    NLSTK_VariableData_S *value)
{
    CP_CHECK_LOG_RETURN_VOID(value->len == CDSM_MEMBER_NUM_LEN, "[CDSM] member num len error");
    coopSet->num = value->data[0];
    setMeb->state = CDSM_READ_MEMBER_NUM_FINISH;
}

typedef struct {
    uint8_t mebNum;
    SLE_Addr_S *addrList;
} MebList_S;

static bool FindNotMebAddr(void *ptr, void *args)
{
    CdsmCoopSetMeb_S *meb = (CdsmCoopSetMeb_S *)ptr;
    MebList_S *mebList = (MebList_S *)args;
    for (uint8_t i = 0; i < mebList->mebNum; i++) {
        if (memcmp(&meb->addr, &mebList->addrList[i], sizeof(SLE_Addr_S)) == 0) {
            return false;
        }
    }
    return true;
}

static void CdsmReadMemberAddrHandle(CdsmCoopSet_S *coopSet, CdsmCoopSetMeb_S *setMeb,
    NLSTK_VariableData_S *value)
{
    CP_CHECK_LOG_RETURN_VOID(coopSet->num > 0, "[CDSM] meb num is 0");
    CP_CHECK_LOG_RETURN_VOID(value->len == CDSM_MEMBER_ADDR_LEN(coopSet), "[CDSM] member addr len error");
    uint8_t addrNum = value->data[0];
    CP_CHECK_LOG_RETURN_VOID(addrNum == coopSet->num - 1, "[CDSM] addr num error");
    uint8_t mebNum = coopSet->num;
    SLE_Addr_S *addrList = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S) * mebNum);
    CP_CHECK_LOG_RETURN_VOID(addrList != NULL, "[CDSM] mebList malloc fail");
    (void)memcpy_s(&addrList[0], sizeof(SLE_Addr_S), &setMeb->addr, sizeof(SLE_Addr_S));
    for (uint8_t i = 0; i < addrNum; i++) {
        (void)memcpy_s(&addrList[i + 1], sizeof(SLE_Addr_S), value->data + i * sizeof(SLE_Addr_S) + 1,
            sizeof(SLE_Addr_S));
        CdsmCoopSetMeb_S meb = {0};
        (void)memcpy_s(&meb.addr, sizeof(SLE_Addr_S), value->data + i * sizeof(SLE_Addr_S) + 1, sizeof(SLE_Addr_S));
        if (CdsmFindCoopSetMember(coopSet->gid, &meb.addr) == NULL) {
            CdsmAddCoopSetMember(coopSet->gid, &meb);
        }
    }
    MebList_S mebList = {.mebNum = mebNum, .addrList = addrList};
    size_t index = 0;
    while(SDF_VectorFindFirst(coopSet->mebs, FindNotMebAddr, &mebList, &index)) {
        CdsmCoopSetMeb_S *meb = SDF_VectorElementAt(coopSet->mebs, index);
        CdsmRemoveCoopSetMember(coopSet->gid, &meb->addr);
    }
    setMeb->state = CDSM_READ_MEMBER_ADDR_FINISH;
    SDF_MemFree(addrList);
}

static void CdsmReadKeyHandle(CdsmCoopSet_S *coopSet, CdsmCoopSetMeb_S *setMeb, NLSTK_VariableData_S *value)
{
    CP_CHECK_LOG_RETURN_VOID(value->len == CDSM_KEY_LEN, "[CDSM] key len error");
    memcpy_s(coopSet->key, CDSM_KEY_LEN, value->data, CDSM_KEY_LEN);
    setMeb->state = CDSM_CONNECTED;
    CdsmNotifyStateChange(&setMeb->addr, CDSM_EVENT_CONNECT);
}

static void DecodeCdsmProperty(CdsmCoopSet_S *coopSet, CdsmCoopSetMeb_S *setMeb,
    NLSTK_SsapClientReadPropertyInfo_S *property)
{
    CP_CHECK_LOG_RETURN_VOID(property->value.data != NULL, "[CDSM] DecodeCdsmProperty input param null");
    switch (setMeb->state) {
        case CDSM_ACB_CONNECTED:
            CdsmReadKeyInfoHandle(coopSet, setMeb, &property->value);
            break;
        case CDSM_READ_KEY_INFO_FINISH:
            CdsmReadMemberNumHandle(coopSet, setMeb, &property->value);
            break;
        case CDSM_READ_MEMBER_NUM_FINISH:
            CdsmReadMemberAddrHandle(coopSet, setMeb, &property->value);
            break;
        case CDSM_READ_MEMBER_ADDR_FINISH:
            CdsmReadKeyHandle(coopSet, setMeb, &property->value);
            break;
        default:
            CP_LOG_ERROR("[CDSM] state error %u", setMeb->state);
            break;
    }
}

static void CdsmPropertyReadCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
 
{
    CP_LOG_DEBUG("enter CdsmPropertyReadCbk");
    CP_CHECK_LOG_RETURN_VOID(property != NULL, "[CDSM] property is null");
    CP_CHECK_LOG_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[CDSM] read fail, errcode = %u", ret);
    CdsmCoopSetMeb_S *setMeb = CdsmFindCoopSetMemberByAppId(appId);
    CP_CHECK_LOG_RETURN_VOID(setMeb != NULL, "[CDSM] setMeb is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(&setMeb->addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    DecodeCdsmProperty(coopSet, setMeb, property);
}

static void CdsmProperitesReadCbk(int32_t appId, uint8_t num, NLSTK_SsapClientReadPropertyInfo_S *properties,
    NLSTK_Errcode_E ret)
{
    CP_LOG_DEBUG("enter CdsmProperitesReadCbk");
    CP_CHECK_LOG_RETURN_VOID(properties != NULL, "[CDSM] properties is null");
    CP_CHECK_LOG_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[CDSM] read fail, errcode = %u", ret);
    CdsmCoopSetMeb_S *setMeb = CdsmFindCoopSetMemberByAppId(appId);
    CP_CHECK_LOG_RETURN_VOID(setMeb != NULL, "[CDSM] setMeb is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(&setMeb->addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    for (uint8_t i = 0; i < num; i++) {
        DecodeCdsmProperty(coopSet, setMeb, &properties[i]);
    }
}

static bool GetCdsmService(CdsmCoopSetMeb_S *meb, CdsmCacheService_S *cache)
{
    NLSTK_SsapServObtain_S param = {0};
    NLSTK_SsapUuid_S uuid = CdsmConvertUuidToStru(CDSM_SERVICE_UUID);
    NLSTK_SsapServ_S *services = NULL;
    uint16_t servNum = 0;
    NLSTK_SsapClientFreeFunc func = NULL;
    param.appId = meb->appId;
    param.uuid = &uuid;
    param.serv = &services;
    param.num = &servNum;
    param.func = &func;
    SsapcAppGetServ(&param);
    CP_CHECK_LOG_RETURN(services != NULL && servNum > 0, false, "[CDSM] not get cdsm service");

    NLSTK_SsapServ_S *service = &services[0];
    for (uint16_t i = 0; i < service->propertyNum; i++) {
        BuildCdsmServiceCache(cache, &service->properties[i]);
    }
    if (func != NULL) {
        func(services, servNum);
    }
    return true;
}

static void ReadCdsmProp(CdsmCoopSetMeb_S *meb, CdsmCacheService_S *cache)
{
    uint16_t handles[CDSM_READ_PROPERTY_NUM] = {0};
    handles[OCTETS_0] = cache->keyInfoHandle;
    handles[OCTETS_1] = cache->memberNumHandle;
    handles[OCTETS_2] = cache->memberAddrHandle;
    handles[OCTETS_3] = cache->keyHandle;
    if (CfgdbGetManufacturerSupport(&meb->addr, CFGDB_READ_MULTI_HANDLES)) {
        if (NLSTK_SsapClientReadProperties(meb->appId, handles, CDSM_READ_PROPERTY_NUM) != NLSTK_ERRCODE_SUCCESS) {
            CP_LOG_ERROR("[CDSM] ssap client read multi properties failed");
        }
        return;
    }
    CP_LOG_DEBUG("[CDSM] ability map not support multi read, enter single read");
    for (uint8_t i = 0; i < CDSM_READ_PROPERTY_NUM; i++) {
        if (NLSTK_SsapClientReadProperty(meb->appId, handles[i]) != NLSTK_ERRCODE_SUCCESS) {
            CP_LOG_ERROR("[CDSM] ssap client read property failed, handle is %d", handles[i]);
        }
    }
}

static void OnCdsmConnStateChanged(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    CdsmCoopSetMeb_S *meb = CdsmFindCoopSetMemberByAppId(appId);
    CP_CHECK_LOG_RETURN_VOID(meb != NULL, "[CDSM] no app");
    if (state == SSAP_CONNECT_STATE_CONNECTED) {
        CP_CHECK_LOG_RETURN_VOID(meb->state == CDSM_ACB_CONNECTING, "[CDSM] meb not in acb connect");
        meb->state = CDSM_ACB_CONNECTED;
        CdsmCacheService_S cache = {0};
        CP_CHECK_LOG_RETURN_VOID(GetCdsmService(meb, &cache), "[CDSM] get cdsm service failed");
        ReadCdsmProp(meb, &cache);
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        CP_CHECK_LOG_RETURN_VOID(meb->state != CDSM_DISCONNECTED, "[CDSM] meb has disconnected");
        meb->state = CDSM_DISCONNECTED;
        SsapcAppRegParam_S param = {.appId = meb->appId};
        SsapcAppDeregister(&param);
        meb->appId = SSAP_APP_INVALID_ID;
        CdsmNotifyStateChange(&meb->addr, CDSM_EVENT_DISCONNECT);
    }
}

uint32_t CdsmRegisterSsapApp(CdsmCoopSetMeb_S *meb)
{
    CP_CHECK_LOG_RETURN(meb != NULL, NLSTK_ERR, "[CDSM] input meb error");
    NLSTK_SsapAppClientCb_S cb = {
        .onConnectionStateChanged = OnCdsmConnStateChanged,
        .onReadProperty = CdsmPropertyReadCbk,
        .onReadProperties = CdsmProperitesReadCbk
    };
    SsapcAppRegParam_S param = {0};
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &meb->addr, sizeof(SLE_Addr_S));
    param.cb = &cb;
    param.linkState = SSAP_CONNECT_STATE_IDLE;
    param.nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    SsapcAppRegister(&param);
    if (param.appId == SSAP_APP_INVALID_ID) {
        CP_LOG_ERROR("[CDSM] register failed");
        return NLSTK_ERR;
    }
    meb->appId = param.appId;
    return NLSTK_OK;
}