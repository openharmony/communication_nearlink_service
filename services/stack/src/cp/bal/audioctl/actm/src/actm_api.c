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
#include "actm_api.h"
#include "actm_tbl.h"
#include "actm_callback.h"
#include "actm_control.h"
#include "actm_qosm_adapter.h"
#include "actm_ssap.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "nlstk_schedule.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_cfgdb.h"
#include "cdsm_api.h"
#include "securec.h"

#define QOSM_LINK_NUM 2
#define ACTM_SSAP_TIMEOUT 4000

typedef struct {
    SLE_Addr_S addr;
    int32_t appId;
} ReadProp_S;

typedef struct {
    SLE_Addr_S addr;
    uint32_t groupId;
    uint8_t streamId;
    uint8_t pointType;
    NLSTK_ActmConfig_S srcConfig;
    NLSTK_ActmConfig_S sinkConfig;
    bool isImg;
    NLSTK_ActmImgEncpParam_S encp;
} ConfigStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t streamId;
} OpenStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t streamId;
    uint8_t op;
} ChangeStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t streamId;
} ReleaseStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t direction;
} DataPath_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t streamId;
    uint64_t bitrate;
} UpdateBitrate_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t pointType;
    uint8_t commType;
} CreateStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t streamId;
} DeleteStream_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t labelId;
    uint8_t qosIndex;
    uint8_t msgType;
    uint32_t result;
} AutoRateMsg_S;

static void ActmReadRemotePropInner(void *arg)
{
    ReadProp_S *param = (ReadProp_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_DEBUG("[ACTM] not find device");
        NLSTK_CHECK_RETURN_VOID(ActmCreateRemoteDevice(&param->addr) == NLSTK_OK, "[ACTM] create device failed");
        device = ActmFindDeviceByAddr(&param->addr);
        NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    } else {
        NLSTK_SsapClientDeregAppAsync(device->appId);
    }
    device->appId = param->appId;
    NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(device->appId);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_READ, NLSTK_ACTM_SSAP_ERROR, NULL);
    }
}

static uint32_t RegisterSsapApp(SLE_Addr_S *addr, int32_t *appId)
{
    NLSTK_SsapAppClientCb_S cb = {0};
    ActmGetSsapCb(&cb);
    return NLSTK_SsapClientRegApp(appId, &cb, addr);
}

uint32_t NLSTK_ActmReadRemoteProp(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    ReadProp_S *readIn = (ReadProp_S *)SDF_MemZalloc(sizeof(ReadProp_S));
    NLSTK_CHECK_RETURN(readIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] read malloc error");
    (void)memcpy_s(&readIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    int32_t appId = 0;
    uint32_t ret = RegisterSsapApp(&readIn->addr, &appId);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(readIn);
        return NLSTK_ERRCODE_FAIL;
    }
    if (NLSTK_SsapClientSetInteractionTimeout(appId, ACTM_SSAP_TIMEOUT) != NLSTK_ERRCODE_SUCCESS) {
        SDF_MemFree(readIn);
        NLSTK_SsapClientDeregApp(appId);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    readIn->appId = appId;
    if (SchedulePostTask(ActmReadRemotePropInner, readIn, SDF_MemFree) != NLSTK_OK) {
        NLSTK_SsapClientDeregApp(appId);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void ActmDisconnectInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(addr, NLSTK_ACTM_EVENT_DISCONNECTED, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    if (NLSTK_SsapClientDisconnect(device->appId) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] disconnect failed");
        ActmEventCbk(addr, NLSTK_ACTM_EVENT_DISCONNECTED, NLSTK_ACTM_SSAP_ERROR, NULL);
    }
}

uint32_t NLSTK_ActmDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] addr malloc error");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(ActmDisconnectInner, addrIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void CreateStreamInner(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[ACTM] arg is null");
    CreateStream_S *param = (CreateStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_CREATE_STREAM, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmCreateStream(device, param->pointType, param->commType);
    if (stream == NULL) {
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_CREATE_STREAM, NLSTK_ACTM_CREATE_STREAM_FAIL, NULL);
        return;
    }
    NLSTK_ActmStreamInfo_S info = {0};
    info.streamId = stream->streamId;
    info.pointType = stream->pointType;
    info.commType = stream->commType;
    ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_CREATE_STREAM, NLSTK_ACTM_SUCCESS, &info);
}

uint32_t NLSTK_ActmCreateStream(SLE_Addr_S* addr, NLSTK_ActmStreamParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    CreateStream_S *createIn = (CreateStream_S *)SDF_MemZalloc(sizeof(CreateStream_S));
    NLSTK_CHECK_RETURN(createIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] createIn malloc error");
    (void)memcpy_s(&createIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    createIn->pointType = param->pointType;
    createIn->commType = param->commType;
    if (SchedulePostTask(CreateStreamInner, createIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void DeleteStreamInner(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[ACTM] arg is null");
    DeleteStream_S *param = (DeleteStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_DELETE_STREAM, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, param->streamId);
    if (stream == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find stream");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_DELETE_STREAM, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    NLSTK_ActmStreamInfo_S info = {0};
    info.streamId = stream->streamId;
    info.pointType = stream->pointType;
    info.commType = stream->commType;
    ActmDeleteStream(device, param->streamId);
    ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_DELETE_STREAM, NLSTK_ACTM_SUCCESS, &info);
}

uint32_t NLSTK_ActmDeleteStream(SLE_Addr_S* addr, uint8_t streamId)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    DeleteStream_S *deleteIn = (DeleteStream_S *)SDF_MemZalloc(sizeof(DeleteStream_S));
    NLSTK_CHECK_RETURN(deleteIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] deleteIn malloc error");
    (void)memcpy_s(&deleteIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    deleteIn->streamId = streamId;
    if (SchedulePostTask(DeleteStreamInner, deleteIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void RecvAutoRateMsgInner(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[ACTM] arg is null");
    AutoRateMsg_S *param = (AutoRateMsg_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_BITRATE, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmRecvAutoRateMsg(device, param->qosIndex, param->labelId, param->msgType, param->result);
}

uint32_t NLSTK_ActmRecvAutoRateMsg(SLE_Addr_S* addr, NLSTK_ActmAutoRateRecvMsg_S* param)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    AutoRateMsg_S *updateIn = (AutoRateMsg_S *)SDF_MemZalloc(sizeof(AutoRateMsg_S));
    NLSTK_CHECK_RETURN(updateIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] deleteIn malloc error");
    (void)memcpy_s(&updateIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    updateIn->labelId = param->labelId;
    updateIn->qosIndex = param->qosIndex;
    updateIn->msgType = param->msgType;
    updateIn->result = param->result;
    if (SchedulePostTask(RecvAutoRateMsgInner, updateIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint8_t GetConfigPointNum(ActmStream_S *stream)
{
    uint8_t pointNum = 0;
    if ((stream->pointType & NLSTK_ACTM_SOURCE_POINT) != 0) {
        pointNum++;
    }
    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0) {
        pointNum++;
    }
    return pointNum;
}

static void AddDeviceToGroup(ActmRemoteDevice_S *device, uint8_t groupId)
{
    if (device->groupId != groupId) {
        device->groupId = groupId;
        device->mebId = ActmCountGroupSize(device->groupId);
        ActmCreateQosmGroup(device->groupId, QOSM_LINK_NUM);
    }
    ActmQosmAddDevice(device->groupId, &device->addr);
}

static void FreeControlReq(ActmControlReq_S *req)
{
    if (req == NULL) {
        return;
    }
    for (uint8_t i = 0; i < req->pointNum; i++) {
        if (req->param[i].data != NULL) {
            SDF_MemFree(req->param[i].data);
        }
    }
    SDF_MemFree(req);
}

static void SaveImgEncpParam(ActmRemoteDevice_S *device, NLSTK_ActmImgEncpParam_S *param)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    (void)memcpy_s(&group->encp, sizeof(NLSTK_ActmImgEncpParam_S), param, sizeof(NLSTK_ActmImgEncpParam_S));
}

static bool FillPointControlInfo(
    ActmRemoteDevice_S *device, ActmStream_S *stream, ConfigStream_S *config, ActmControlReq_S *req)
{
    ActmAccessPoint_S *point = NULL;
    uint8_t i = 0;
    if ((stream->pointType & NLSTK_ACTM_SOURCE_POINT) != 0) {
        point = ActmFindPointById(device, stream->srcPointId);
        if (point == NULL) {
            // 失败后由外部释放req
            NLSTK_LOG_ERROR("[ACTM] point is null");
            return false;
        }
        ActmConfigPoint(device, stream, point, &config->srcConfig, &req->param[i]);
        i++;
    }
    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0) {
        point = ActmFindPointById(device, stream->sinkPointId);
        if (point == NULL) {
            // 失败后由外部释放req
            NLSTK_LOG_ERROR("[ACTM] point is null");
            return false;
        }
        ActmConfigPoint(device, stream, point, &config->sinkConfig, &req->param[i]);
    }
    return true;
}

static void ConfigAudioStreamInner(void *arg)
{
    ConfigStream_S *config = (ConfigStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&config->addr);
    if (device == NULL) {
        NLSTK_LOG_INFO("[ACTM] not find device");
        ActmEventCbk(&config->addr, NLSTK_ACTM_EVENT_CONFIG, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, config->streamId);
    if (stream == NULL || stream->pointType != config->pointType) {
        NLSTK_LOG_ERROR("[ACTM] not find stream or stream point type error");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_CONFIG, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    AddDeviceToGroup(device, (uint8_t)config->groupId);
    if (config->isImg) {
        SaveImgEncpParam(device, &config->encp);
    }
    uint8_t pointNum = GetConfigPointNum(stream);
    NLSTK_CHECK_RETURN_VOID(pointNum != 0, "[ACTM] point num error");
    ActmControlReq_S *req = (ActmControlReq_S *)SDF_MemZalloc(sizeof(ActmControlReq_S) +
        pointNum * sizeof(ActmPointControlInfo_S));
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] config req malloc error");
    req->appId = device->appId;
    req->handle = device->info.ctrlHandle;
    req->opcode = ACTM_CONTROL_CONFIG;
    req->pointNum = pointNum;
    bool ret = FillPointControlInfo(device, stream, config, req);
    if (!ret) {
        NLSTK_LOG_ERROR("[ACTM] Fill point control info error");
        FreeControlReq(req);
        return;
    }
    ActmControlReqBySsap(req);
    device->ssapState = ACTM_START_CONFIG;
    device->isPriv = false;
    FreeControlReq(req);
}

uint32_t NLSTK_ActmConfigAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    ConfigStream_S *configIn = (ConfigStream_S *)SDF_MemZalloc(sizeof(ConfigStream_S));
    NLSTK_CHECK_RETURN(configIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] config malloc error");
    (void)memcpy_s(&configIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    configIn->groupId = NLSTK_CdsmCreateSet(addr);
    configIn->streamId = param->streamId;
    if (param->srcConfig != NULL) {
        (void)memcpy_s(&configIn->srcConfig, sizeof(NLSTK_ActmConfig_S), param->srcConfig, sizeof(NLSTK_ActmConfig_S));
        configIn->pointType |= NLSTK_ACTM_SOURCE_POINT;
    }
    if (param->sinkConfig != NULL) {
        (void)memcpy_s(&configIn->sinkConfig, sizeof(NLSTK_ActmConfig_S),
            param->sinkConfig, sizeof(NLSTK_ActmConfig_S));
        configIn->pointType |= NLSTK_ACTM_SINK_POINT;
    }
    configIn->isImg = param->isImg;
    (void)memcpy_s(&configIn->encp, sizeof(NLSTK_ActmImgEncpParam_S), &param->encp, sizeof(NLSTK_ActmImgEncpParam_S));
    if (SchedulePostTask(ConfigAudioStreamInner, configIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void StartAudioStreamInner(void *arg)
{
    ConfigStream_S *config = (ConfigStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&config->addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(&config->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, config->streamId);
    if (stream == NULL || stream->pointType != config->pointType) {
        NLSTK_LOG_ERROR("[ACTM] not find stream or stream point type error");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    ConfigAudioStreamInner(config);
    if (CfgdbGetManufacturerSupport(&device->addr, CFGDB_START_PLAYING_MERGE)) {
        device->curStreamId = stream->streamId;
        device->isPriv = true;
        device->ssapState = ACTM_START_TRANS;
        device->qosmState = ACTM_START_QOSM;
        ActmSetQosmParam(device, stream);
    } else {
        // 查询能力协商位图结果不支持起播合并，不会在config阶段进行同步链路建链
        NLSTK_LOG_INFO("[ACTM] remote dev not support startPlaying merge, send config req without add conn");
    }
}

uint32_t NLSTK_ActmStartAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    ConfigStream_S *configIn = (ConfigStream_S *)SDF_MemZalloc(sizeof(ConfigStream_S));
    NLSTK_CHECK_RETURN(configIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] config malloc error");
    (void)memcpy_s(&configIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    configIn->groupId = NLSTK_CdsmCreateSet(addr);
    configIn->streamId = param->streamId;
    if (param->srcConfig != NULL) {
        (void)memcpy_s(&configIn->srcConfig, sizeof(NLSTK_ActmConfig_S), param->srcConfig, sizeof(NLSTK_ActmConfig_S));
        configIn->pointType |= NLSTK_ACTM_SOURCE_POINT;
    }
    if (param->sinkConfig != NULL) {
        (void)memcpy_s(&configIn->sinkConfig, sizeof(NLSTK_ActmConfig_S),
            param->sinkConfig, sizeof(NLSTK_ActmConfig_S));
        configIn->pointType |= NLSTK_ACTM_SINK_POINT;
    }
    configIn->isImg = param->isImg;
    (void)memcpy_s(&configIn->encp, sizeof(NLSTK_ActmImgEncpParam_S), &param->encp, sizeof(NLSTK_ActmImgEncpParam_S));
    if (SchedulePostTask(StartAudioStreamInner, configIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void OpenAudioStreamInner(void *arg)
{
    OpenStream_S *param = (OpenStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_OPEN, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, param->streamId);
    if (stream == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find stream");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_OPEN, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    ActmOpenStream(device, stream);
    device->ssapState = ACTM_START_OPEN;
}

uint32_t NLSTK_ActmOpenAudioStream(SLE_Addr_S *addr, NLSTK_ActmOpenParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    OpenStream_S *openIn = (OpenStream_S *)SDF_MemZalloc(sizeof(OpenStream_S));
    NLSTK_CHECK_RETURN(openIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] addr malloc error");
    (void)memcpy_s(&openIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    openIn->streamId = param->streamId;
    if (SchedulePostTask(OpenAudioStreamInner, openIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void ChangeAudioStreamInner(void *arg)
{
    ChangeStream_S *param = (ChangeStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    uint8_t event = param->op == NLSTK_ACTM_STREAM_TRANS ? NLSTK_ACTM_EVENT_TRANS : NLSTK_ACTM_EVENT_STOP;
    if (device == NULL) {
        NLSTK_LOG_INFO("[ACTM] not find device");
        ActmEventCbk(&param->addr, event, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, param->streamId);
    if (stream == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find stream");
        ActmEventCbk(&device->addr, event, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    ActmChangeStream(device, stream, param->op);
    if (param->op == NLSTK_ACTM_STREAM_STOP) {
        device->ssapState = ACTM_START_STOP;
    } else {
        device->ssapState = ACTM_START_TRANS;
        device->qosmState = ACTM_START_QOSM;
        device->curStreamId = stream->streamId;
        ActmSetQosmParam(device, stream);
    }
}

uint32_t NLSTK_ActmChangeAudioStream(SLE_Addr_S *addr, NLSTK_ActmChangeParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    ChangeStream_S *changeIn = (ChangeStream_S *)SDF_MemZalloc(sizeof(ChangeStream_S));
    NLSTK_CHECK_RETURN(changeIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] change malloc error");
    (void)memcpy_s(&changeIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    changeIn->streamId = param->streamId;
    changeIn->op = param->op;
    if (SchedulePostTask(ChangeAudioStreamInner, changeIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void ReleaseAudioStreamInner(void *arg)
{
    ReleaseStream_S *param = (ReleaseStream_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_INFO("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, param->streamId);
    if (stream == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find stream");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    uint16_t connHandle = ActmGetConnHandle(&device->addr, device->groupId);
    if (connHandle == QOSM_INVALID_HANDLE) {
        NLSTK_LOG_ERROR("[ACTM] get connHandle error");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_RELEASE, NLSTK_ACTM_QOSM_ERROR, NULL);
        return;
    }
    device->ssapState = ACTM_START_RELEASE;
    ActmReleaseStream(device, stream);
    device->qosmState = ACTM_DEL_CONN;
    ActmDelConnection(device, connHandle);
}

uint32_t NLSTK_ActmReleaseAudioStream(SLE_Addr_S *addr, NLSTK_ActmReleaseParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    ReleaseStream_S *releaseIn = (ReleaseStream_S *)SDF_MemZalloc(sizeof(ReleaseStream_S));
    NLSTK_CHECK_RETURN(releaseIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] addr malloc error");
    (void)memcpy_s(&releaseIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    releaseIn->streamId = param->streamId;
    if (SchedulePostTask(ReleaseAudioStreamInner, releaseIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void RegisterCallbackInner(void *arg)
{
    NLSTK_ActmCbk_S *cbk = (NLSTK_ActmCbk_S *)arg;
    ActmSetCallback(cbk);
}

uint32_t NLSTK_ActmRegisterCallback(NLSTK_ActmCbk_S *cbk)
{
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    NLSTK_ActmCbk_S *cbkIn = (NLSTK_ActmCbk_S *)SDF_MemZalloc(sizeof(NLSTK_ActmCbk_S));
    NLSTK_CHECK_RETURN(cbkIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] cbk malloc error");
    (void)memcpy_s(cbkIn, sizeof(NLSTK_ActmCbk_S), cbk, sizeof(NLSTK_ActmCbk_S));
    if (SchedulePostTask(RegisterCallbackInner, cbkIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void SetDirectionInner(void *arg)
{
    DataPath_S *dataPath = (DataPath_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&dataPath->addr);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    ActmSetDataPath(device, dataPath->direction);
}

uint32_t NLSTK_ActmSetDirection(SLE_Addr_S *addr, uint8_t direction)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_PARAM_ERR, "[ACTM] param is null");
    NLSTK_CHECK_RETURN(direction >= NLSTK_ACTM_DIRECTION_DOWN && direction <= NLSTK_ACTM_DIRECTION_BOTH,
        NLSTK_ERRCODE_PARAM_ERR, "[ACTM] direction out of range");
    DataPath_S *dataPath = (DataPath_S *)SDF_MemZalloc(sizeof(DataPath_S));
    NLSTK_CHECK_RETURN(dataPath != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] dataPath malloc error");
    (void)memcpy_s(&dataPath->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    dataPath->direction = direction;
    if (SchedulePostTask(SetDirectionInner, dataPath, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void UpdateBitrateInner(void *arg)
{
    UpdateBitrate_S *param = (UpdateBitrate_S *)arg;
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&param->addr);
    if (device == NULL) {
        NLSTK_LOG_INFO("[ACTM] not find device");
        ActmEventCbk(&param->addr, NLSTK_ACTM_EVENT_BITRATE, NLSTK_ACTM_NOT_FIND_DEVICE, NULL);
        return;
    }
    ActmStream_S *stream = ActmFindStreamById(device, param->streamId);
    if (stream == NULL) {
        NLSTK_LOG_ERROR("[ACTM] not find stream");
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_BITRATE, NLSTK_ACTM_NOT_FIND_STREAM, NULL);
        return;
    }
    ActmChangeBitrate(device, stream, param->bitrate);
}

uint32_t NLSTK_ActmUpdateBitrate(SLE_Addr_S *addr, NLSTK_ActmBitrateParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_POINTER_NULL, "[ACTM] param is null");
    UpdateBitrate_S *bitrateIn = (UpdateBitrate_S *)SDF_MemZalloc(sizeof(UpdateBitrate_S));
    NLSTK_CHECK_RETURN(bitrateIn != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[ACTM] bitrateIn malloc error");
    (void)memcpy_s(&bitrateIn->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    bitrateIn->streamId = param->streamId;
    bitrateIn->bitrate = param->bitrate;
    if (SchedulePostTask(UpdateBitrateInner, bitrateIn, SDF_MemFree) != NLSTK_OK) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}