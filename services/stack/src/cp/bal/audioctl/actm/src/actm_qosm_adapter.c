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
#include "actm_qosm_adapter.h"
#include "actm_callback.h"
#include "actm_l2hc.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "qosm_autorate.h"
#include "qosm_errno.h"
#include "cm_logic_link_api.h"
#include "nlstk_sm_api.h"
#include "securec.h"

#define US_TO_MS 1000

#define QOS_LEVEL_NUM 3

#define QOSM_DOWN 0
#define QOSM_UP   1

#define MAX_BITRATE_LEVEL 14

static SDF_DListHead_S g_groupList;

static void ClearLinkInfo(ActmQosmLink_S *link)
{
    (void)memset_s(&link->addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    link->state = ACTM_QOSM_DISCONNECTED;
    link->used = false;
    link->direction = NLSTK_ACTM_DIRECTION_UNCONFIG;
    link->bitrate = 0;
}

ActmQosmGroup_S *ActmFindQosmGroupById(uint16_t icgId)
{
    NLSTK_CHECK_RETURN(SDF_DListCount(&g_groupList) > 0, NULL, "[ACTM] group list is empty");
    ActmQosmGroup_S *group = NULL;
    SDF_DListElmForeach(group, &g_groupList, entry) {
        if (group->icgId == icgId) {
            return group;
        }
    }
    return NULL;
}

ActmQosmGroup_S *ActmFindQosmGroupByHandle(uint16_t gHandle)
{
    NLSTK_CHECK_RETURN(SDF_DListCount(&g_groupList) > 0, NULL, "[ACTM] group list is empty");
    ActmQosmGroup_S *group = NULL;
    SDF_DListElmForeach(group, &g_groupList, entry) {
        if (group->gHandle == gHandle) {
            return group;
        }
    }
    return NULL;
}

ActmQosmLink_S *ActmFindQosmLinkByHandle(uint16_t icgId, uint16_t connHandle)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN(group != NULL, NULL, "[ACTM] not find group");
    ActmQosmLink_S *link = NULL;
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        link = SDF_VectorElementAt(group->links, i);
        if (link->connHandle == connHandle && link->used) {
            return link;
        }
    }
    return NULL;
}

ActmQosmLink_S *ActmFindQosmLinkByAddr(uint16_t icgId, SLE_Addr_S* addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[ACTM] addr is null");
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN(group != NULL, NULL, "[ACTM] not find group");
    ActmQosmLink_S *link = NULL;
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        link = SDF_VectorElementAt(group->links, i);
        if (link->used && memcmp(&link->addr, addr, sizeof(SLE_Addr_S)) == 0) {
            return link;
        }
    }
    return NULL;
}

uint32_t ActmCreateQosmGroup(uint16_t icgId, uint8_t linkCnt)
{
    NLSTK_CHECK_RETURN(ActmFindQosmGroupById(icgId) == NULL, NLSTK_OK, "[ACTM] group has been created");
    ActmQosmGroup_S *group = (ActmQosmGroup_S *)SDF_MemZalloc(sizeof(ActmQosmGroup_S));
    NLSTK_CHECK_RETURN(group != NULL, NLSTK_ERR, "[ACTM] group malloc error");
    group->icgId = icgId;
    group->state = ACTM_QOSM_PARAM_DELETE;
    group->linkCnt = linkCnt;
    group->gHandle = QOSM_INVALID_HANDLE;
    SDF_Traits traits = {.dtor = SDF_MemFree};
    group->links = SDF_CreateVector(traits);
    if (group->links == NULL) {
        SDF_MemFree(group);
        NLSTK_LOG_ERROR("[ACTM] link vector create failed");
        return NLSTK_ERR;
    }
    group->devices = SDF_CreateVector(traits);
    if (group->devices == NULL) {
        SDF_DestroyVector(group->links);
        SDF_MemFree(group);
        return NLSTK_ERR;
    }
    for (uint8_t i = 0; i < linkCnt; i++) {
        ActmQosmLink_S *link = (ActmQosmLink_S *)SDF_MemZalloc(sizeof(ActmQosmLink_S));
        if (link == NULL) {
            SDF_DestroyVector(group->links);
            SDF_DestroyVector(group->devices);
            SDF_MemFree(group);
            NLSTK_LOG_ERROR("[ACTM] link malloc error");
            return NLSTK_ERR;
        }
        ClearLinkInfo(link);
        if (!SDF_VectorEmplaceBack(group->links, link)) {
            SDF_MemFree(link);
            SDF_DestroyVector(group->links);
            SDF_DestroyVector(group->devices);
            SDF_MemFree(group);
            NLSTK_LOG_ERROR("[ACTM] link emplace back vector failed");
            return NLSTK_ERR;
        }
    }
    NLSTK_LOG_INFO("[ACTM] create qosm group, icgId: %d, linkCnt: %d", group->icgId, group->linkCnt);
    SDF_DListElmTailInsert(&g_groupList, group, entry);
    return NLSTK_OK;
}

void ActmQosmAddDevice(uint16_t icgId, SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[ACTM] addr is null");
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    SLE_Addr_S *tmpAddr = NULL;
    for (size_t i = 0; i < group->devices->size; i++) {
        tmpAddr = SDF_VectorElementAt(group->devices, i);
        if (memcmp(tmpAddr, addr, sizeof(SLE_Addr_S)) == 0) {
            NLSTK_LOG_INFO("[ACTM] device has been added");
            return;
        }
    }
    tmpAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN_VOID(tmpAddr != NULL, "[ACTM] addr malloc failed");
    (void)memcpy_s(tmpAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (!SDF_VectorEmplaceBack(group->devices, tmpAddr)) {
        SDF_MemFree(tmpAddr);
        NLSTK_LOG_ERROR("[ACTM] device emplace back group vector failed");
        return;
    }
}

uint16_t ActmAllocConnHandle(SLE_Addr_S *addr, uint16_t icgId)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN(group != NULL, QOSM_INVALID_HANDLE, "[ACTM] not find group");
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
        if (!link->used) {
            (void)memcpy_s(&link->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
            link->used = true;
            link->state = ACTM_QOSM_DISCONNECTED;
            link->direction = NLSTK_ACTM_DIRECTION_UNCONFIG;
            return link->connHandle;
        }
    }
    return QOSM_INVALID_HANDLE;
}

uint16_t ActmGetConnHandle(SLE_Addr_S *addr, uint16_t icgId)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN(group != NULL, QOSM_INVALID_HANDLE, "[ACTM] not find group");
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
        if (memcmp(&link->addr, addr, sizeof(SLE_Addr_S)) == 0 && link->used) {
            return link->connHandle;
        }
    }
    return QOSM_INVALID_HANDLE;
}

static uint8_t GetSupportedBitrate(uint8_t codecId, uint64_t bps, uint16_t *bitrates)
{
    uint8_t count = 0;
    NLSTK_LOG_DEBUG("[ACTM] bps: 0x%x", bps);
    for (uint8_t i = 0; i < MAX_BITRATE_LEVEL && count < QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT; i++) {
        if ((bps & (1UL << i)) == 0) {
            continue;
        }
        uint16_t bitrate = L2HCGetBps(codecId, i);
        if (bitrate == 0) {
            continue;
        }
        bitrates[count] = bitrate;
        count++;
        NLSTK_LOG_DEBUG("[ACTM] add bitrate: %d", bitrate);
    }
    return count;
}

static bool CheckOtherSetParam(ActmQosmGroup_S *group, uint8_t first, uint8_t second)
{
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *tmpDevice = NULL;
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice != NULL && tmpDevice->qosmState >= first && tmpDevice->qosmState <= second) {
            return true;
        }
    }
    return false;
}

static uint8_t GetLcidParam(ActmQosmGroup_S *group, uint8_t *lcid)
{
    uint8_t lcidCnt = 0;
    for (size_t i = 0; i < group->devices->size && lcidCnt < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        SLE_Addr_S *addr = SDF_VectorElementAt(group->devices, i);
        CM_LogicLink_S logicLink = {0};
        if (CM_GetLogicLinkByAddr(addr, &logicLink) != 0) {
            NLSTK_LOG_ERROR("[ACTM] get lcid failed");
            continue;
        }
        lcid[lcidCnt] = logicLink.lcid;
        lcidCnt++;
    }
    return lcidCnt;
}

static void ActmAddQosmConn(uint16_t icgId, uint8_t num, uint16_t *connHandle)
{
    NLSTK_CHECK_RETURN_VOID(connHandle != NULL, "[ACTM] param is null");
    ActmQosmLink_S *link = NULL;
    QOSM_ConnParam *linkParam = (QOSM_ConnParam *)SDF_MemZalloc(num * sizeof(QOSM_ConnParam));
    NLSTK_CHECK_RETURN_VOID(linkParam != NULL, "[ACTM] linkParam malloc failed");
    for (uint8_t i = 0; i < num; i++) {
        link = ActmFindQosmLinkByHandle(icgId, connHandle[i]);
        if (link == NULL) {
            NLSTK_LOG_ERROR("[ACTM] not find link, connHandle: %d", connHandle[i]);
            SDF_MemFree(linkParam);
            return;
        }
        CM_LogicLink_S logicLink = {0};
        if (CM_GetLogicLinkByAddr(&link->addr, &logicLink) != 0) {
            NLSTK_LOG_ERROR("[ACTM] get lcid failed");
            SDF_MemFree(linkParam);
            return;
        }
        linkParam[i].connHandle = connHandle[i];
        linkParam[i].lcid = logicLink.lcid;
        NLSTK_LOG_INFO("[ACTM] add connection, connHandle: %d, addr: %s", connHandle[i], GET_ENC_ADDR(&link->addr));
        link->state = ACTM_QOSM_CONNECTING;
    }
    QOSM_AutoRateConnParam param = {0};
    param.qosId = icgId;
    param.linkCnt = num;
    param.link = linkParam;
    if (QOSM_AutoRateAddConnection(&param) != QOSM_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] qosm add connection failed");
    }
    SDF_MemFree(linkParam);
}

static void NotifyQosmError(ActmRemoteDevice_S *device)
{
    switch (device->ssapState) {
        case ACTM_START_FAIL:
            ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_TRANS, device->lastErr, NULL);
            break;
        case ACTM_START_TRANS:
        case ACTM_TRANS_COMPL:
            ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_QOSM_ERROR, NULL);
            break;
        default:
            break;
    }
    device->ssapState = ACTM_SSAP_IDLE;
    device->qosmState = ACTM_QOSM_IDLE;
    device->lastErr = NLSTK_ACTM_SUCCESS;
}

static void OpenAudioChannel(ActmQosmGroup_S *group)
{
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *tmpDevice = NULL;
    uint8_t openNum = 0;
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice != NULL && tmpDevice->qosmState == ACTM_SET_PARAM_COMPL) {
            openNum++;
        }
    }
    NLSTK_CHECK_RETURN_VOID(openNum > 0, "[ACTM] no device need to open");
    uint16_t *connHandle = (uint16_t *)SDF_MemZalloc(openNum * sizeof(uint16_t));
    NLSTK_CHECK_RETURN_VOID(connHandle != NULL, "[ACTM] connHandle malloc failed");
    uint8_t j = 0;
    for (size_t i = 0; i < group->devices->size && j < openNum; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->qosmState != ACTM_SET_PARAM_COMPL) {
            continue;
        }
        connHandle[j] = ActmGetConnHandle(&tmpDevice->addr, group->icgId);
        if (connHandle[j] != QOSM_INVALID_HANDLE) {
            tmpDevice->qosmState = ACTM_ADD_CONN;
            j++;
            continue;
        }
        connHandle[j] = ActmAllocConnHandle(&tmpDevice->addr, group->icgId);
        if (connHandle[j] == QOSM_INVALID_HANDLE) {
            NLSTK_LOG_ERROR("[ACTM] get connHandle error");
            NotifyQosmError(tmpDevice);
            connHandle[j] = 0;
            continue;
        }
        tmpDevice->qosmState = ACTM_ADD_CONN;
        tmpDevice->connHandle = connHandle[j];
        j++;
    }

    ActmAddQosmConn(group->icgId, openNum, connHandle);
    SDF_MemFree(connHandle);
}

void ActmSetQosmParam(ActmRemoteDevice_S *device, ActmStream_S *stream)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    if (CheckOtherSetParam(group, ACTM_SET_PARAM_COMPL, ACTM_CONN_COMPL)) {
        NLSTK_LOG_INFO("[ACTM] qosm group param has been added");
        device->qosmState = ACTM_SET_PARAM_COMPL;
        OpenAudioChannel(group);
        return;
    }
    if (CheckOtherSetParam(group, ACTM_SET_PARAM, ACTM_SET_PARAM)) {
        NLSTK_LOG_INFO("[ACTM] qosm group param is adding");
        return;
    }
    QOSM_AutoRateParam param = {0};
    param.qosId = group->icgId;
    param.qosIndex = stream->qosIndex;
    param.linkCnt = group->linkCnt;
    param.bitrate = L2HCGetBps(stream->codec.codecId, stream->codec.l2hc.bpsConf);
    param.supportedBitrateCnt = GetSupportedBitrate(stream->codec.codecId, stream->codec.l2hc.bpsRange,
        param.supportedBitrate);
    param.lcidCnt = GetLcidParam(group, param.lcid);
    NLSTK_LOG_INFO("[ACTM] set qosm param, icgId: %d, qos index: %d, start bitrate: %u", param.qosId, param.qosIndex,
        param.bitrate);
    if (QOSM_AutoRateSetTestParam(&param) != QOSM_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] set qosm param failed");
        NotifyQosmError(device);
        return;
    }
    device->qosmState = ACTM_SET_PARAM;
    group->state = ACTM_QOSM_PARAM_ADDING;
}

void ActmDelQosmParam(uint16_t icgId)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(icgId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    NLSTK_CHECK_RETURN_VOID(group->state == ACTM_QOSM_PARAM_ADD, "[ACTM] qosm group param not add");
    NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateRemoveParam(group->icgId) == QOSM_SUCCESS, "[ACTM] del qosm param failed");
}

static void NotifySetParamResult(ActmQosmGroup_S *group, uint32_t result)
{
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *device = NULL;
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        device = ActmFindDeviceByAddr(addr);
        if (device == NULL || (device->qosmState != ACTM_SET_PARAM && device->qosmState != ACTM_START_QOSM)) {
            continue;
        }
        if (result == QOSM_SUCCESS) {
            device->qosmState = ACTM_SET_PARAM_COMPL;
        } else {
            NotifyQosmError(device);
        }
    }
    OpenAudioChannel(group);
}

static void EncryptImg(ActmQosmGroup_S *group)
{
    NLSTK_SmImgEncpParam_S param = {0};
    param.imgHandle = group->gHandle;
    param.cryptoAlgo = group->encp.cryptAlgo;
    param.giv = group->encp.giv;
    (void)memcpy_s(&param.groupKey, GROUP_KEY_LEN, &group->encp.groupKey, GROUP_KEY_LEN);
    NLSTK_CHECK_RETURN_VOID(NLSTK_SmEnableImgEncp(&param) == NLSTK_ERRCODE_SUCCESS,
        "[ACTM] sm enable encrypt image failed");
}

static void QosmParamChangedCbk(const QOSM_ParamCb *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[ACTM] param is null");
    NLSTK_LOG_INFO("[ACTM] param state: %d, result: %d, linkCnt: %d", param->state, param->result, param->linkCnt);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(param->qosId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    if (param->state == QOSM_PARAM_SETTED) {
        if (param->result != QOSM_SUCCESS || group->linkCnt != param->linkCnt) {
            NLSTK_LOG_ERROR("[ACTM] set qosm param failed, result: %d", param->result);
            group->state = ACTM_QOSM_PARAM_DELETE;
            NotifySetParamResult(group, QOSM_FAIL);
            return;
        }
        group->isImg = param->isIMG;
        group->direction = NLSTK_ACTM_DIRECTION_UNCONFIG;
        NLSTK_CHECK_RETURN_VOID(param->connHandle != NULL, "[ACTM] connHandle is null");
        for (uint8_t i = 0; i < group->linkCnt && i < param->linkCnt; i++) {
            ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
            if (link->state == ACTM_QOSM_DISCONNECTED || link->state == ACTM_QOSM_CONNECTING) {
                ClearLinkInfo(link);
                link->connHandle = param->connHandle[i];
            }
        }
        if (param->isIMG) {
            group->gHandle = param->gHandle;
            NLSTK_LOG_INFO("[ACTM] qosm group param added but need encryption, icgId: %d", group->icgId);
            EncryptImg(group);
            return;
        }
        group->state = ACTM_QOSM_PARAM_ADD;
        NotifySetParamResult(group, QOSM_SUCCESS);
    } else {
        NLSTK_CHECK_RETURN_VOID(param->result == 0, "[ACTM] remove qosm param failed, result: %d", param->result);
        group->state = ACTM_QOSM_PARAM_DELETE;
        group->gHandle = QOSM_INVALID_HANDLE;
        group->isImg = false;
        group->direction = NLSTK_ACTM_DIRECTION_UNCONFIG;
        for (uint8_t i = 0; i < group->linkCnt; i++) {
            ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
            ClearLinkInfo(link);
        }
    }
    NLSTK_LOG_INFO("[ACTM] qosm group param state changed, icgId: %d, state: %d", group->icgId, group->state);
}

static void ImgEncryptComplete(NLSTK_SmImgEncpCmpl_S *params)
{
    NLSTK_CHECK_RETURN_VOID(params != NULL, "[ACTM] params is null");
    ActmQosmGroup_S *group = ActmFindQosmGroupByHandle(params->imgHandle);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] group is null");
    group->state = ACTM_QOSM_PARAM_ADD;
    NLSTK_LOG_INFO("[ACTM] qosm group encrypted, icgId: %d, params: %d", group->icgId, params->encpStatus);
    if (params->encpStatus == SM_IMG_OK) {
        NotifySetParamResult(group, QOSM_SUCCESS);
    } else {
        NotifySetParamResult(group, QOSM_FAIL);
    }
}

static uint8_t GetPathId(uint8_t qosIndex)
{
    if (qosIndex == QOSINDEX_4) {
        return QOSM_PATHID_SENSORHUB;
    }
    return QOSM_PATHID_DSP;
}

static void AddDataPath(ActmRemoteDevice_S *device, uint8_t handle, uint8_t direction)
{
    ActmStream_S *stream = ActmFindStreamById(device, device->curStreamId);
    NLSTK_CHECK_RETURN_VOID(stream != NULL, "[ACTM] not find stream");
    QOSM_AutoRateDataPath param = {0};
    param.qosId = device->groupId;
    param.connHandle = handle;
    param.codec.codecId = stream->codec.codecId;
    param.codec.vendorId = stream->codec.companyId;
    param.codec.vendorCodecId = stream->codec.vendorId;
    param.codecConfigLen = 1;
    uint8_t codecConfigData[1] = {0};
    param.codecConfigData = codecConfigData;
    if ((direction & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
        param.direction = QOSM_DOWN;
        param.pathId = QOSM_PATHID_DSP;
        if (QOSM_AutoRateAddDataPath(&param) != QOSM_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] qosm add down data path failed");
        }
    }
    if ((direction & NLSTK_ACTM_DIRECTION_UP) != 0) {
        param.direction = QOSM_UP;
        param.pathId = GetPathId(stream->qosIndex);
        if (QOSM_AutoRateAddDataPath(&param) != QOSM_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] qosm add up data path failed");
        }
    }
}

static void DelDataPath(ActmRemoteDevice_S *device, uint8_t handle, uint8_t direction)
{
    QOSM_AutoRateDeletedDataPath param = {0};
    param.qosId = device->groupId;
    param.connHandle = handle;
    if ((direction & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
        param.direction = QOSM_DOWN;
        if (QOSM_AutoRateDeleteDataPath(&param) != QOSM_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] qosm del down data path failed");
        }
    }
    if ((direction & NLSTK_ACTM_DIRECTION_UP) != 0) {
        param.direction = QOSM_UP;
        if (QOSM_AutoRateDeleteDataPath(&param) != QOSM_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] qosm del up data path failed");
        }
    }
}

static void SetDownDataPath(ActmRemoteDevice_S *device, ActmQosmLink_S *link)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    if (!group->isImg) {
        AddDataPath(device, link->connHandle, NLSTK_ACTM_DIRECTION_DOWN);
        return;
    }
    if ((group->direction & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
        link->direction |= NLSTK_ACTM_DIRECTION_DOWN;
        return;
    }
    group->direction |= NLSTK_ACTM_DIRECTION_DOWN;
    AddDataPath(device, group->gHandle, NLSTK_ACTM_DIRECTION_DOWN);
}

static void DelDownDataPath(ActmRemoteDevice_S *device, ActmQosmLink_S *link)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    if (!group->isImg) {
        DelDataPath(device, link->connHandle, NLSTK_ACTM_DIRECTION_DOWN);
        return;
    }
    link->direction &= ~NLSTK_ACTM_DIRECTION_DOWN;
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
        if (link->state == ACTM_QOSM_CONNECTED && (link->direction & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
            return;
        }
    }
    if ((group->direction & NLSTK_ACTM_DIRECTION_DOWN) == 0) {
        link->direction &= ~NLSTK_ACTM_DIRECTION_DOWN;
        return;
    }
    group->direction &= ~NLSTK_ACTM_DIRECTION_DOWN;
    DelDataPath(device, group->gHandle, NLSTK_ACTM_DIRECTION_DOWN);
}

void ActmSetDataPath(ActmRemoteDevice_S *device, uint8_t direction)
{
    uint16_t connHandle = ActmGetConnHandle(&device->addr, device->groupId);
    NLSTK_CHECK_RETURN_VOID(connHandle != QOSM_INVALID_HANDLE, "[ACTM] not find connection");
    ActmQosmLink_S *link = ActmFindQosmLinkByHandle(device->groupId, connHandle);
    NLSTK_CHECK_RETURN_VOID(link != NULL, "[ACTM] not find link");
    NLSTK_CHECK_RETURN_VOID(link->direction != direction, "[ACTM] same direction");
    uint8_t delDirection = (link->direction ^ direction) & link->direction;
    uint8_t addDirection = (link->direction ^ direction) & direction;
    if ((delDirection & NLSTK_ACTM_DIRECTION_UP) != 0) {
        DelDataPath(device, link->connHandle, NLSTK_ACTM_DIRECTION_UP);
    }
    if ((delDirection & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
        DelDownDataPath(device, link);
    }
    if ((addDirection & NLSTK_ACTM_DIRECTION_UP) != 0) {
        AddDataPath(device, link->connHandle, NLSTK_ACTM_DIRECTION_UP);
    }
    if ((addDirection & NLSTK_ACTM_DIRECTION_DOWN) != 0) {
        SetDownDataPath(device, link);
    }
}

static void DelQosmConnection(uint16_t icgId, ActmQosmLink_S *link)
{
    CM_LogicLink_S logicLink = {0};
    NLSTK_CHECK_RETURN_VOID(CM_GetLogicLinkByAddr(&link->addr, &logicLink) == 0, "[ACTM] get lcid failed");
    QOSM_ConnParam linkParam = {0};
    QOSM_AutoRateConnParam connParam = {0};
    linkParam.connHandle = link->connHandle;
    linkParam.lcid = logicLink.lcid;
    connParam.qosId = icgId;
    connParam.linkCnt = 1;
    connParam.link = &linkParam;
    NLSTK_LOG_INFO("[ACTM] del connection, connHandle: %d, addr: %s", link->connHandle,
        GET_ENC_ADDR(&link->addr));
    NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateDeleteConnection(&connParam) == QOSM_SUCCESS, "[ACTM] qosm del conn failed!");
}

void ActmDelConnection(ActmRemoteDevice_S *device, uint16_t connHandle)
{
    ActmQosmLink_S *link = ActmFindQosmLinkByHandle(device->groupId, connHandle);
    NLSTK_CHECK_RETURN_VOID(link != NULL, "[ACTM] not find link");
    if (link->direction != NLSTK_ACTM_DIRECTION_UNCONFIG) {
        ActmSetDataPath(device, NLSTK_ACTM_DIRECTION_UNCONFIG);
    }
    DelQosmConnection(device->groupId, link);
}

void ActmRecvAutoRateMsg(ActmRemoteDevice_S *device, uint8_t qosIndex, uint8_t labelId,
    uint8_t msgType, uint32_t result)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    QOSM_AutoRateRecvMsgParam param = {0};
    param.qosId = group->icgId;
    param.qosIndex = qosIndex;
    param.labelId = labelId;
    param.msgType = msgType;
    param.result = result;
    NLSTK_LOG_INFO("qosId = %d, qosIndex = %d, label id = %d, msgType = %d, result = %d",
        group->icgId, qosIndex, labelId, msgType, result);
    NLSTK_CHECK_RETURN_VOID(QOSM_RecvAutoRateMsg(&param) == QOSM_SUCCESS, "[ACTM] upgrade bitrate failed!");
}

static uint8_t CountConnNum(ActmQosmGroup_S *group)
{
    ActmQosmLink_S *link = NULL;
    uint8_t count = 0;
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        link = SDF_VectorElementAt(group->links, i);
        if (link->used && link->state == ACTM_QOSM_CONNECTED) {
            count++;
        }
    }
    return count;
}

void ActmNotifyConnState(ActmRemoteDevice_S *device)
{
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] device is null");
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(device->groupId, &device->addr);
    NLSTK_CHECK_RETURN_VOID(link != NULL, "[ACTM] not find device");
    ActmStream_S *stream = ActmFindStreamById(device, device->curStreamId);
    NLSTK_CHECK_RETURN_VOID(stream != NULL, "[ACTM] not find stream");
    QOSM_ICBParam param = {0};
    if (stream->qosIndex == QOSINDEX_5) {
        NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateGetICGT2GParam(stream->qosIndex, &param) == QOSM_SUCCESS,
            "[ACTM] qosm get icg t2g param failed");
    } else {
        NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateGetICGG2TParam(stream->qosIndex, &param) == QOSM_SUCCESS,
            "[ACTM] qosm get icg g2t param failed");
    }
    NLSTK_ActmQosmInfo_S info = {0};
    info.qosIndex = stream->qosIndex;
    info.gHandle = group->gHandle;
    info.connHandle = link->connHandle;
    info.codecId = stream->codec.codecId;
    info.version = stream->codec.l2hc.version;
    info.sduInterval = param.sduInterval / US_TO_MS;
    info.maxSdu = param.maxSdu;
    info.bufNum = param.icbNum;
    info.channelMode = stream->codec.l2hc.channelConf;
    info.linkCnt = CountConnNum(group);
    info.frame = L2HCGetFrame(stream->codec.l2hc.frameConf);
    info.bitSamp = L2HCGetDepth(stream->codec.l2hc.depthConf);
    info.rate = L2HCGetRate(stream->codec.l2hc.rateConf);
    info.companyId = stream->codec.companyId;
    info.vendorId = stream->codec.vendorId;
    info.bps = link->bitrate;
    info.sca = param.sca;
    info.packing = param.packing;
    info.framing = param.framing;
    info.ft = param.ft;
    info.rtn = param.rtn;
    info.nse = param.nse;
    info.bn = param.bn;
    info.phy = param.phy;
    info.mcs = param.mcs;
    info.pilot = param.pilot;
    info.frameType = param.frame;
    info.maxPdu = param.maxPdu;
    info.maxLatency = param.maxLatency;
    info.icbInterval = param.icbInterval;
    ActmEventCbk(&link->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_SUCCESS, &info);
    device->ssapState = ACTM_START_PLAY_COMPL;
}

static void ProcessConnect(ActmRemoteDevice_S *device)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *tmpDevice = NULL;
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL) {
            continue;
        }
        if (tmpDevice->ssapState >= ACTM_START_TRANS && tmpDevice->ssapState < ACTM_TRANS_COMPL) {
            return;
        }
    }
    ActmNotifyConnState(device);
}

void ActmNotifyDisconnState(ActmRemoteDevice_S *device)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *tmpDevice = NULL;
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL) {
            continue;
        }
        if (tmpDevice->qosmState == ACTM_DEL_CONN || tmpDevice->ssapState == ACTM_START_RELEASE) {
            return;
        }
    }
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->qosmState != ACTM_DEL_COMPL ||
            tmpDevice->ssapState != ACTM_RELEASE_COMPL) {
            continue;
        }
        ActmEventCbk(&tmpDevice->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_SUCCESS, &tmpDevice->connHandle);
        tmpDevice->ssapState = ACTM_SSAP_IDLE;
        tmpDevice->qosmState = ACTM_QOSM_IDLE;
        ActmQosmLink_S *tmpLink = ActmFindQosmLinkByHandle(tmpDevice->groupId, tmpDevice->connHandle);
        tmpDevice->connHandle = 0;
        if (tmpLink != NULL) {
            ClearLinkInfo(tmpLink);
        }
    }
}

static void NotifyDisconnState(ActmRemoteDevice_S *device, ActmQosmLink_S *link)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    if (device->ssapState == ACTM_SSAP_IDLE || device->ssapState == ACTM_START_TRANS ||
        device->ssapState == ACTM_TRANS_COMPL) {
        if (device->ssapState == ACTM_SSAP_IDLE) {
            device->qosmState = ACTM_QOSM_IDLE;
        } else if (device->ssapState == ACTM_START_TRANS || device->ssapState == ACTM_TRANS_COMPL) {
            device->qosmState = ACTM_QOSM_FAIL;
        }
        device->connHandle = 0;
        ClearLinkInfo(link);
        return;
    }
    if (device->qosmState != ACTM_DEL_CONN) {
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_SUCCESS, &link->connHandle);
        device->ssapState = ACTM_SSAP_IDLE;
        device->qosmState = ACTM_QOSM_IDLE;
        device->connHandle = 0;
        ClearLinkInfo(link);
        return;
    }
    device->qosmState = ACTM_DEL_COMPL;
    ActmNotifyDisconnState(device);
}

static void ProcessAddConnError(ActmRemoteDevice_S *device, ActmQosmLink_S *link)
{
    if (device->ssapState == ACTM_TRANS_COMPL) {
        NotifyQosmError(device);
        return;
    } else if (device->ssapState == ACTM_START_FAIL) {
        NotifyQosmError(device);
        return;
    } else if (device->ssapState == ACTM_START_PLAY_COMPL) {
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_SUCCESS, &link->connHandle);
        device->ssapState = ACTM_SSAP_IDLE;
    }
    device->qosmState = ACTM_QOSM_FAIL;
}

static void QosmConnChangedCbk(const QOSM_ConnParamCb *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[ACTM] param is null");
    NLSTK_CHECK_RETURN_VOID(param->link != NULL, "[ACTM] param link is null");
    ActmQosmLink_S *link = NULL;
    ActmRemoteDevice_S *device = NULL;
    for (uint8_t i = 0; i < param->linkCnt; i++) {
        link = ActmFindQosmLinkByHandle(param->qosId, param->link[i].connHandle);
        if (link == NULL) {
            continue;
        }
        device = ActmFindDeviceByAddr(&link->addr);
        if (device == NULL) {
            continue;
        }
        if (param->state == QOSM_CONNECTION_ADDED) {
            if (param->result == QOSM_SUCCESS) {
                link->state = ACTM_QOSM_CONNECTED;
                link->bitrate = (uint16_t)param->bitrate;
                device->qosmState = ACTM_CONN_COMPL;
                ProcessConnect(device);
            } else {
                NLSTK_LOG_ERROR("[ACTM] qosm link add failed, result: %d", param->result);
                ProcessAddConnError(device, link);
                device->connHandle = 0;
                ClearLinkInfo(link);
                continue;
            }
        } else {
            link->state = ACTM_QOSM_DISCONNECTED;
            NotifyDisconnState(device, link);
        }
        NLSTK_LOG_INFO("[ACTM] qosm link state changed, connHandle: %d, state: %d", link->connHandle, link->state);
    }
}

static uint8_t GetDirectionByQosm(uint8_t direction)
{
    if (direction == QOSM_DOWN) {
        return NLSTK_ACTM_DIRECTION_DOWN;
    }
    if (direction == QOSM_UP) {
        return NLSTK_ACTM_DIRECTION_UP;
    }
    return 0;
}

static void IcgDataPathChanged(const QOSM_DataPathParamCb *param)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupByHandle(param->connHandle);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] qosm group not find");
    uint8_t direction = GetDirectionByQosm(param->direction);
    NLSTK_CHECK_RETURN_VOID(direction == NLSTK_ACTM_DIRECTION_DOWN, "[ACTM] param direction error");
    if (param->result != QOSM_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] data path changed error");
        if (param->state == QOSM_DATAPATH_ADDED) {
            group->direction &= ~direction;
        } else if (param->state == QOSM_DATAPATH_DELETED) {
            group->direction |= direction;
        }
        return;
    }
    NLSTK_LOG_INFO("[ACTM] group datapath changed, direction: %d, handle: 0x%x", group->direction, group->gHandle);
    for (uint8_t i = 0; i < group->linkCnt; i++) {
        ActmQosmLink_S *link = SDF_VectorElementAt(group->links, i);
        if (link->state != ACTM_QOSM_CONNECTED) {
            continue;
        }
        if (param->state == QOSM_DATAPATH_ADDED) {
            link->direction |= direction;
        } else if (param->state == QOSM_DATAPATH_DELETED) {
            link->direction &= ~direction;
        }
        NLSTK_LOG_INFO("[ACTM] link datapath changed, direction: %d, handle: 0x%x", link->direction, link->connHandle);
    }
}

static void QosmDataPathChangedCbk(const QOSM_DataPathParamCb *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[ACTM] param is null");
    ActmQosmLink_S *link = ActmFindQosmLinkByHandle(param->qosId, param->connHandle);
    if (link == NULL) {
        IcgDataPathChanged(param);
        return;
    }
    uint8_t direction = GetDirectionByQosm(param->direction);
    NLSTK_CHECK_RETURN_VOID(direction != 0, "[ACTM] param direction error");
    if (param->state == QOSM_DATAPATH_DELETED) {
        NLSTK_CHECK_RETURN_VOID((link->direction & direction) != 0, "[ACTM] no direction: %d", direction);
        link->direction &= ~direction;
    } else if (param->state == QOSM_DATAPATH_ADDED) {
        NLSTK_CHECK_RETURN_VOID((link->direction & direction) == 0, "[ACTM] exist direction: %d", direction);
        link->direction |= direction;
    }
    NLSTK_LOG_INFO("[ACTM] link datapath changed, direction: %d, handle: 0x%x", link->direction, link->connHandle);
}

static void QosmRateChangedCbk(const QOSM_BitrateParamCb *param, uint8_t paramCnt)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[ACTM] param is null");
    NLSTK_ActmBitrateChange_S bitrate = {0};
    bitrate.groupId = param->qosId;
    bitrate.direction = param->direction;
    bitrate.labelId = param->labelId;
    bitrate.downBitrate = param->downwardBitrate;
    bitrate.upBitrate = param->upwardBitrate;
    bitrate.qosIndex = param->qosIndex;
    bitrate.qosLevel = param->qosLevel;
    bitrate.dutyCycle = param->dutyCycle;
    bitrate.availableBitratesCnt = param->availableBitratesCnt;
    for (uint8_t i = 0; i < param->availableBitratesCnt && i < ACTM_AVAILABLE_BITRATE_MAX; i++) {
        bitrate.availableBitrates[i] = param->availableBitrates[i];
    }
    ActmBitCbk(&bitrate);
}

static void QosmAutoRateMsgCbk(const QOSM_AutoRateSendMsgCb *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[ACTM] param is null");
    NLSTK_ActmAutoRateSendMsg_S autoRateMsg = {0};
    autoRateMsg.qosId = param->qosId;
    autoRateMsg.linkCnt = param->linkCnt;
    for (uint8_t i = 0; i < param->linkCnt && i < NLSTK_MAX_LINK_CNT && i < QOSM_AUTORATE_MAX_LINK_CNT; i++) {
        CM_LogicLink_S logicLink = {0};
        if (CM_GetLogicLinkByLcid(param->lcid[i], &logicLink) == QOSM_SUCCESS) {
            autoRateMsg.addr[i] = logicLink.addr;
        }
    }
    autoRateMsg.labelId = param->labelId;
    autoRateMsg.qosIndex = param->qosIndex;
    autoRateMsg.direction = param->direction;
    autoRateMsg.upwardBitrate = param->upwardBitrate;
    autoRateMsg.downwardBitrate = param->downwardBitrate;
    autoRateMsg.msgType = param->msgType;
    autoRateMsg.result = param->result;
    ActmCallBitUpDownCbk(&autoRateMsg);
}

void ActmUpdateBitRate(ActmRemoteDevice_S *device, uint64_t bps)
{
    ActmStream_S *stream = ActmFindStreamById(device, device->curStreamId);
    NLSTK_CHECK_RETURN_VOID(stream != NULL, "[ACTM] not find stream");
    QOSM_AutoRateEarphoneFeedbackParam param = {0};
    param.supportedBitrateCnt = GetSupportedBitrate(stream->codec.codecId, bps, param.supportedBitrate);
    NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateSetEarphoneFeedback(&param) == QOSM_SUCCESS,
        "[ACTM] qosm set support bitrate failed");
}

void ActmGroupListInit(void)
{
    SDF_DListHeadInit(&g_groupList);
}

uint32_t ActmRegisterQosmCbk(void)
{
    QOSM_AutoRateCallback cbk = {
        .paramChangedCbk = QosmParamChangedCbk,
        .connChangedCbk = QosmConnChangedCbk,
        .dataPathChangedCbk = QosmDataPathChangedCbk,
        .bitrateChangedCbk = QosmRateChangedCbk,
        .callBitrateUpDownCbk = QosmAutoRateMsgCbk
    };
    if (QOSM_AutoRateRegisterCallback(&cbk) != QOSM_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] register qosm cbk failed");
        return NLSTK_ERR;
    }
    return NLSTK_OK;
}

uint32_t ActmRegisterSmCbk(void)
{
    NLSTK_SmImgCallbacks_S cbk = {
        .imgEncpCbk = ImgEncryptComplete
    };
    if (NLSTK_SmRegImgCbks(&cbk) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] register sm cbk failed");
        return NLSTK_ERR;
    }
    return NLSTK_OK;
}

void ActmDeregisterQosmCbk(void)
{
    NLSTK_CHECK_RETURN_VOID(QOSM_AutoRateUnregisterCallback() == QOSM_SUCCESS, "[ACTM] deregister qosm cbk failed");
}

static void FreeGroup(ActmQosmGroup_S *group)
{
    SDF_DestroyVector(group->links);
    SDF_DestroyVector(group->devices);
    SDF_MemFree(group);
}

void ActmCleanGroupList(void)
{
    ActmQosmGroup_S *group = NULL;
    ActmQosmGroup_S *tmpGroup = NULL;
    SDF_DListElmAllFree(group, tmpGroup, &g_groupList, entry, FreeGroup);
    SDF_DListHeadInit(&g_groupList);
}