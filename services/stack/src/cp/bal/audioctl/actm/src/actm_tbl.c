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
#include "actm_tbl.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "nlstk_mcp_volume.h"
#include "sdf_mem.h"
#include "securec.h"

#define UNDEFINE_POINT_ID 0xff

SDF_DListHead_S g_DeviceList;

ActmRemoteDevice_S *ActmFindDeviceByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[ACTM] addr is null");
    NLSTK_CHECK_LOGD_RETURN(SDF_DListCount(&g_DeviceList) > 0, NULL, "[ACTM] device list is empty");
    ActmRemoteDevice_S *device = NULL;
    ActmDeviceNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_DeviceList, entry) {
        if (memcmp(&node->device.addr, addr, sizeof(SLE_Addr_S)) == 0) {
            device = &node->device;
            break;
        }
    }
    return device;
}

ActmRemoteDevice_S *ActmFindDeviceByAppId(int32_t appId)
{
    NLSTK_CHECK_RETURN(SDF_DListCount(&g_DeviceList) > 0, NULL, "[ACTM] device list is empty");
    ActmRemoteDevice_S *device = NULL;
    ActmDeviceNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_DeviceList, entry) {
        if (node->device.appId == appId) {
            device = &node->device;
            break;
        }
    }
    return device;
}

static void FreeProp(void *arg)
{
    if (arg == NULL) {
        return;
    }
    ActmProp_S *prop = (ActmProp_S *)arg;
    SDF_MemFree(prop->ability.codec);
    SDF_MemFree(prop);
}

static void FreeDeviceNode(ActmDeviceNode_S *node)
{
    SDF_DestroyVector(node->device.points);
    SDF_DestroyVector(node->device.props);
    SDF_DestroyVector(node->device.streams);
    SDF_MemFree(node);
}

uint32_t ActmCreateRemoteDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(ActmFindDeviceByAddr(addr) == NULL, NLSTK_OK, "[ACTM] device has been created");
    ActmDeviceNode_S *node = (ActmDeviceNode_S *)SDF_MemZalloc(sizeof(ActmDeviceNode_S));
    NLSTK_CHECK_RETURN(node != NULL, NLSTK_ERR, "[ACTM] node malloc error");
    ActmRemoteDevice_S *device = &node->device;
    (void)memcpy_s(&device->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    device->groupId = 0xFF;
    device->curStreamId = INVALID_STREAM_ID;
    SDF_Traits pointTraits = {.dtor = SDF_MemFree};
    device->points = SDF_CreateVector(pointTraits);
    if (device->points == NULL) {
        FreeDeviceNode(node);
        NLSTK_LOG_ERROR("[ACTM] point vector create failed");
        return NLSTK_ERR;
    }
    SDF_Traits propTraits = {.dtor = FreeProp};
    device->props = SDF_CreateVector(propTraits);
    if (device->props == NULL) {
        FreeDeviceNode(node);
        NLSTK_LOG_ERROR("[ACTM] prop vector create failed");
        return NLSTK_ERR;
    }
    SDF_Traits streamTraits = {.dtor = SDF_MemFree};
    device->streams = SDF_CreateVector(streamTraits);
    if (device->streams == NULL) {
        FreeDeviceNode(node);
        NLSTK_LOG_ERROR("[ACTM] stream vector create failed");
        return NLSTK_ERR;
    }
    NLSTK_LOG_DEBUG("[ACTM] create remote device, addr: %s", GET_ENC_ADDR(addr));
    SDF_DListElmTailInsert(&g_DeviceList, node, entry);
    return NLSTK_OK;
}

void ActmDestroyRemoteDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(SDF_DListCount(&g_DeviceList) > 0, "[ACTM] device list is empty");
    ActmDeviceNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_DeviceList, entry) {
        if (memcmp(&node->device.addr, addr, sizeof(SLE_Addr_S)) == 0) {
            SDF_DListElmDel(&g_DeviceList, node, entry);
            FreeDeviceNode(node);
            return;
        }
    }
}

ActmAccessPoint_S *ActmFindPointById(ActmRemoteDevice_S *device, uint8_t pointId)
{
    NLSTK_CHECK_RETURN(device != NULL, NULL, "[ACTM] device is null");
    ActmAccessPoint_S *point = NULL;
    for (uint8_t i = 0; i < device->points->size; i++) {
        point = SDF_VectorElementAt(device->points, i);
        if (point->pointId == pointId) {
            return point;
        }
    }
    return NULL;
}

ActmProp_S *ActmFindPropById(ActmRemoteDevice_S *device, uint8_t propId)
{
    NLSTK_CHECK_RETURN(device != NULL, NULL, "[ACTM] device is null");
    ActmProp_S *prop = NULL;
    for (uint8_t i = 0; i < device->props->size; i++) {
        prop = SDF_VectorElementAt(device->props, i);
        if (prop->propId == propId) {
            return prop;
        }
    }
    return NULL;
}

ActmProp_S *ActmFindPropByAblHandle(ActmRemoteDevice_S *device, uint32_t handle)
{
    NLSTK_CHECK_RETURN(device != NULL, NULL, "[ACTM] device is null");
    ActmProp_S *prop = NULL;
    for (uint8_t i = 0; i < device->props->size; i++) {
        prop = SDF_VectorElementAt(device->props, i);
        if (prop->abilityHandle == handle) {
            return prop;
        }
    }
    return NULL;
}

ActmProp_S *ActmFindPropByTypeHandle(ActmRemoteDevice_S *device, uint32_t handle)
{
    NLSTK_CHECK_RETURN(device != NULL, NULL, "[ACTM] device is null");
    ActmProp_S *prop = NULL;
    for (uint8_t i = 0; i < device->props->size; i++) {
        prop = SDF_VectorElementAt(device->props, i);
        if (prop->acceptTypeHandle == handle) {
            return prop;
        }
    }
    return NULL;
}

ActmStream_S *ActmFindStreamById(ActmRemoteDevice_S *device, uint8_t streamId)
{
    NLSTK_CHECK_RETURN(device != NULL, NULL, "[ACTM] device is null");
    ActmStream_S *stream = NULL;
    for (uint8_t i = 0; i < device->streams->size; i++) {
        stream = SDF_VectorElementAt(device->streams, i);
        if (stream->streamId == streamId) {
            return stream;
        }
    }
    return NULL;
}

uint8_t ActmCountGroupSize(uint16_t groupId)
{
    uint8_t count = 0;
    ActmDeviceNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_DeviceList, entry) {
        if (node->device.groupId == groupId) {
            count++;
        }
    }
    return count;
}

static bool MatchCommType(ActmRemoteDevice_S *device, ActmAccessPoint_S *point, uint8_t commType)
{
    ActmProp_S *prop = ActmFindPropById(device, point->propId);
    NLSTK_CHECK_RETURN(prop != NULL, false, "[ACTM] not find prop");
    return ((1 << commType) & prop->ability.comm) != 0;
}

static bool FindUnusedPoint(ActmRemoteDevice_S *device, ActmStream_S *stream)
{
    uint8_t srcPointId = UNDEFINE_POINT_ID;
    uint8_t sinkPointId = UNDEFINE_POINT_ID;
    if ((stream->pointType & NLSTK_ACTM_SOURCE_POINT) != 0) {
        srcPointId = INVALID_POINT_ID;
        for (size_t i = 0; i < device->points->size; i++) {
            ActmAccessPoint_S *point = SDF_VectorElementAt(device->points, i);
            if (point->type == NLSTK_ACTM_SOURCE_POINT && !point->used &&
                MatchCommType(device, point, stream->commType)) {
                srcPointId = point->pointId;
                break;
            }
        }
    }
    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0) {
        sinkPointId = INVALID_POINT_ID;
        for (size_t i = 0; i < device->points->size; i++) {
            ActmAccessPoint_S *point = SDF_VectorElementAt(device->points, i);
            if (point->type == NLSTK_ACTM_SINK_POINT && !point->used &&
                MatchCommType(device, point, stream->commType)) {
                sinkPointId = point->pointId;
                break;
            }
        }
    }
    if (sinkPointId == INVALID_POINT_ID || srcPointId == INVALID_POINT_ID) {
        NLSTK_LOG_INFO("[ACTM] no enough access point");
        return false;
    }
    if ((stream->pointType & NLSTK_ACTM_SOURCE_POINT) != 0) {
        stream->srcPointId = srcPointId;
        ActmAccessPoint_S *point = ActmFindPointById(device, srcPointId);
        point->used = true;
    }
    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0) {
        stream->sinkPointId = sinkPointId;
        ActmAccessPoint_S *point = ActmFindPointById(device, sinkPointId);
        point->used = true;
    }
    return true;
}

static uint8_t GenerateStreamId(ActmRemoteDevice_S *device)
{
    for (uint8_t i = 0; i < INVALID_STREAM_ID; i++) {
        if (ActmFindStreamById(device, i) == NULL) {
            return i;
        }
    }
    return INVALID_STREAM_ID;
}

/* 该函数是为了FindUnusedPoint函数使用后出现异常，需要把已经置为used的point状态回滚到false */
static void FreePointUse(ActmStream_S *stream, ActmRemoteDevice_S *device)
{
    ActmAccessPoint_S *srcPoint = ActmFindPointById(device, stream->srcPointId);
    if (srcPoint != NULL) {
        srcPoint->used = false;
    }
    ActmAccessPoint_S *sinkPoint = ActmFindPointById(device, stream->sinkPointId);
    if (sinkPoint != NULL) {
        sinkPoint->used = false;
    }
}

ActmStream_S *ActmCreateStream(ActmRemoteDevice_S *device, uint8_t pointType, uint8_t commType)
{
    NLSTK_CHECK_RETURN(device != NULL && pointType >= NLSTK_ACTM_SOURCE_POINT && pointType <= NLSTK_ACTM_ALL_POINT,
        NULL, "[ACTM] param error");
    ActmStream_S *stream = (ActmStream_S *)SDF_MemZalloc(sizeof(ActmStream_S));
    NLSTK_CHECK_RETURN(stream != NULL, NULL, "[ACTM] stream malloc error");
    stream->pointType = pointType;
    stream->commType = commType;
    stream->srcPointId = INVALID_POINT_ID;
    stream->sinkPointId = INVALID_POINT_ID;
    if (!FindUnusedPoint(device, stream)) {
        SDF_MemFree(stream);
        return NULL;
    }
    stream->streamId = GenerateStreamId(device);
    if (stream->streamId == INVALID_STREAM_ID) {
        NLSTK_LOG_ERROR("[ACTM] stream id is invalid");
        // 上面的FindUnusedPoint函数会标记一些point为used，如果生成streamId失败，需要将这些used状态的point恢复
        FreePointUse(stream, device);
        SDF_MemFree(stream);
        return NULL;
    }
    if (!SDF_VectorEmplaceBack(device->streams, stream)) {
        NLSTK_LOG_ERROR("[ACTM] streams vector emplace back error");
        FreePointUse(stream, device);
        SDF_MemFree(stream);
        return NULL;
    }
    NLSTK_LOG_INFO("[ACTM] create stream, id: %d, pointType: %d, commType: %d",
        stream->streamId, stream->pointType, stream->commType);
    if (commType == NLSTK_ACTM_UNICAST) {
        McpVolumeNotifyAccessPoint(&device->addr, stream->sinkPointId, INVALID_POINT_ID);
    } else if (commType == NLSTK_ACTM_MULTICAST) {
        McpVolumeNotifyAccessPoint(&device->addr, INVALID_POINT_ID, stream->sinkPointId);
    }
    return stream;
}

static bool FindStream(void *ptr, void *args)
{
    ActmStream_S *stream = (ActmStream_S *)ptr;
    uint8_t *streamId = (uint8_t *)args;
    return stream->streamId == *streamId;
}

void ActmDeleteStream(ActmRemoteDevice_S *device, uint8_t streamId)
{
    size_t index = 0;
    bool ret = SDF_VectorFindFirst(device->streams, FindStream, &streamId, &index);
    NLSTK_CHECK_RETURN_VOID(ret, "[ACTM] not find stream");
    ActmStream_S *stream = (ActmStream_S *)SDF_VectorElementAt(device->streams, index);
    NLSTK_CHECK_RETURN_VOID(stream != NULL, "[ACTM] not find stream");
    ActmAccessPoint_S *point = ActmFindPointById(device, stream->srcPointId);
    if (point != NULL) {
        point->used = false;
    }
    point = ActmFindPointById(device, stream->sinkPointId);
    if (point != NULL) {
        point->used = false;
    }
    NLSTK_LOG_INFO("[ACTM] delete stream, id: %d", stream->streamId);
    SDF_VectorRemove(device->streams, index);
}

void ActmTblInit(void)
{
    SDF_DListHeadInit(&g_DeviceList);
}

void ActmCleanTbl(void)
{
    ActmDeviceNode_S *node = NULL;
    ActmDeviceNode_S *tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &g_DeviceList, entry, FreeDeviceNode);
    SDF_DListHeadInit(&g_DeviceList);
}