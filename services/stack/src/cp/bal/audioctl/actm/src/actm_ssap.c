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
#include "actm_ssap.h"
#include "actm_control.h"
#include "actm_callback.h"
#include "actm_l2hc.h"
#include "actm_qosm_adapter.h"
#include "ssapc_app.h"
#include "ssap_pkt.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "nlstk_cfgdb.h"
#include "sdf_mem.h"
#include "cm_logic_link_api.h"
#include "securec.h"

#define OPCODE_LEN    1
#define POINT_NUM_LEN 1
#define POINT_ID_LEN  1
#define RESCODE_LEN   1
#define BITRATE_UPDATE_LEN 6

#define OCTETS_14 14
#define OCTETS_15 15
#define SHIFT_8_BITS 8

#define AUDIO_POINT_LEN 4

#define PROP_ID_LEN 1
#define PROP_TYPE_LEN 1

#define MAX_PROP_NUM 6
#define HANDLE_LEN 2
#define TYPE_AUDIO_ABLITITY 0x01
#define TYPE_STREAM_TYPE    0x20

#define MAX_SERVICE_PROPERTY_NUM 255

#define AUDIO_LOCATION_LEN 10
#define MULTI_CHANNEL_SYSTEM_TYPE 0x00
#define LOCATION_LEFT  0x08
#define LOCATION_RIGHT 0x10

#define AVAILABLE_STREAM_TYPE_LEN 4

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static NLSTK_SsapUuid_S ConvertUuidTo128Bits(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < OCTETS_14; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[OCTETS_14] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[OCTETS_15] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

static uint16_t ConvertUuidTo16Bits(NLSTK_SsapUuid_S *uuid)
{
    return (uint16_t)(uuid->uuid[OCTETS_14]) << SHIFT_8_BITS | uuid->uuid[OCTETS_15];
}

static bool IsUuidEqual(uint16_t uuid, NLSTK_SsapPrty_S *property)
{
    return uuid == ConvertUuidTo16Bits(&property->uuid);
}

static void ReadDeviceProperties(ActmRemoteDevice_S *device, uint16_t *handles, uint8_t num)
{
    if (CfgdbGetManufacturerSupport(&device->addr, CFGDB_READ_MULTI_HANDLES)) {
        if (NLSTK_SsapClientReadProperties(device->appId, handles, num) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] read multi handles error");
        }
        return;
    }
    for (uint8_t i = 0; i < num; i++) {
        if (NLSTK_SsapClientReadProperty(device->appId, handles[i]) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[ACTM] read handle error");
        }
    }
}

static void GetStreamManagementInfo(ActmRemoteDevice_S *device, NLSTK_SsapServ_S *service)
{
    if (service->propertyNum > MAX_SERVICE_PROPERTY_NUM) {
        NLSTK_LOG_ERROR("[ACTM] service property num exceed max num");
        return;
    }
    for (uint16_t i = 0; i < service->methodNum; i++) {
        if (IsUuidEqual(AUDIO_CONTROL_POINT_UUID, &service->methods[i])) {
            device->info.ctrlHandle = service->methods[i].handle;
            break;
        }
    }
    for (uint16_t i = 0; i < service->eventNum; i++) {
        if (IsUuidEqual(AUDIO_STREAM_STATE_CHANGE_UUID, &service->events[i])) {
            device->info.eventHandle = service->events[i].handle;
            break;
        }
    }
    uint16_t handles[MAX_SERVICE_PROPERTY_NUM] = {0};
    uint8_t num = 0;
    for (uint16_t i = 0; i < service->propertyNum; i++) {
        if (IsUuidEqual(AUDIO_SOURCE_POINT_UUID, &service->properties[i]) ||
            IsUuidEqual(AUDIO_SINK_POINT_UUID, &service->properties[i])) {
            handles[num++] = service->properties[i].handle;
        }
    }
    ReadDeviceProperties(device, handles, num);
}

static void GetPublicPropInfo(ActmRemoteDevice_S *device, NLSTK_SsapServ_S *service)
{
    if (service->propertyNum > MAX_SERVICE_PROPERTY_NUM) {
        NLSTK_LOG_ERROR("[ACTM] service property num exceed max num");
        return;
    }
    uint16_t handles[MAX_SERVICE_PROPERTY_NUM] = {0};
    uint8_t num = 0;
    for (uint16_t i = 0; i < service->propertyNum; i++) {
        if (IsUuidEqual(AUDIO_SOURCE_PROPERTY_UUID, &service->properties[i]) ||
            IsUuidEqual(AUDIO_SINK_PROPERTY_UUID, &service->properties[i])) {
            handles[num++] = service->properties[i].handle;
        }
    }
    for (uint16_t i = 0; i < service->propertyNum; i++) {
        if (IsUuidEqual(AUDIO_SOURCE_ABILITY_UUID, &service->properties[i]) ||
            IsUuidEqual(AUDIO_SOURCE_STREAM_TYPE_UUID, &service->properties[i]) ||
            IsUuidEqual(AUDIO_SINK_ABILITY_UUID, &service->properties[i])) {
            handles[num++] = service->properties[i].handle;
        } else if (IsUuidEqual(AUDIO_SINK_LOCATION_UUID, &service->properties[i])) {
            handles[num++] = service->properties[i].handle;
            if (device->info.locationHandle != 0) {
                continue;
            }
            device->info.locationHandle = service->properties[i].handle;
            if (NLSTK_SsapClientSetPropertyNtf(device->appId, device->info.locationHandle, true) !=
                    NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[ACTM] ssap client set property notify error");
            }
        } else if (IsUuidEqual(AUDIO_SINK_STREAM_TYPE_UUID, &service->properties[i])) {
            handles[num++] = service->properties[i].handle;
            if (device->info.availableStreamTypeHandle != 0) {
                continue;
            }
            device->info.availableStreamTypeHandle = service->properties[i].handle;
            if (NLSTK_SsapClientSetPropertyNtf(device->appId, device->info.availableStreamTypeHandle, true) !=
                    NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[ACTM] ssap client set property notify error");
            }
        }
    }
    ReadDeviceProperties(device, handles, num);
}

bool ActmGetService(ActmRemoteDevice_S *device)
{
    NLSTK_SsapServObtain_S param = {0};
    NLSTK_SsapUuid_S uuid = ConvertUuidTo128Bits(AUDIO_STREAM_MANAGEMENT_UUID);
    NLSTK_SsapServ_S *services = NULL;
    uint16_t servNum = 0;
    NLSTK_SsapClientFreeFunc func = NULL;
    param.appId = device->appId;
    param.uuid = &uuid;
    param.serv = &services;
    param.num = &servNum;
    param.func = &func;
    SsapcAppGetServ(&param);
    NLSTK_CHECK_RETURN(services != NULL && servNum > 0, false, "[CDSM] not get stream manage service");
    NLSTK_SsapServ_S *service = &services[0];
    GetStreamManagementInfo(device, service);
    if (func != NULL) {
        func(services, servNum);
    }
    uuid = ConvertUuidTo128Bits(AUDIO_PUBLIC_PROPERTY_UUID);
    services = NULL;
    servNum = 0;
    func = NULL;
    SsapcAppGetServ(&param);
    NLSTK_CHECK_RETURN(services != NULL && servNum > 0, false, "[CDSM] not get public property service");
    service = &services[0];
    GetPublicPropInfo(device, service);
    if (func != NULL) {
        func(services, servNum);
    }
    return true;
}

void ActmControlReqBySsap(ActmControlReq_S *req)
{
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] req is null");
    uint16_t bufLen = OPCODE_LEN + POINT_NUM_LEN + POINT_ID_LEN * req->pointNum;
    for (uint8_t i = 0; i < req->pointNum; i++) {
        bufLen += req->param[i].dataLen;
    }
    NLSTK_VariableData_S value = {0};
    value.len = bufLen;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[ACTM] data malloc error");
    uint8_t *buf = value.data;
    buf[0] = req->opcode;
    buf[1] = req->pointNum;
    buf += OPCODE_LEN + POINT_NUM_LEN;
    for (uint8_t i = 0; i < req->pointNum; i++) {
        buf[0] = req->param[i].pointId;
        if (req->param[i].dataLen > 0) {
            (void)memcpy_s(buf + POINT_ID_LEN, req->param[i].dataLen, req->param[i].data, req->param[i].dataLen);
        }
        buf += POINT_ID_LEN + req->param[i].dataLen;
    }
    NLSTK_LOG_INFO("[ACTM] audio control req, opcode:0x%x, point num: %d", req->opcode, req->pointNum);
    if (NLSTK_SsapClientCallMethod(req->appId, req->handle, &value, false) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] ssap control req call method error");
    }
    SDF_MemFree(value.data);
}

static void NotifySsapError(int32_t appId)
{
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    uint8_t event = 0;
    switch (device->ssapState) {
        case ACTM_START_CONFIG:
            event = NLSTK_ACTM_EVENT_CONFIG;
            break;
        case ACTM_START_OPEN:
            event = NLSTK_ACTM_EVENT_OPEN;
            break;
        case ACTM_START_TRANS:
            event = NLSTK_ACTM_EVENT_TRANS;
            break;
        case ACTM_START_STOP:
            event = NLSTK_ACTM_EVENT_STOP;
            break;
        case ACTM_START_RELEASE:
            event = NLSTK_ACTM_EVENT_RELEASE;
            break;
        default:
            NLSTK_LOG_ERROR("[ACTM] not ssap control state");
            return;
    }
    ActmEventCbk(&device->addr, event, NLSTK_ACTM_SSAP_ERROR, NULL);
    ActmDeviceOffline(device, false);
}

static void FreeControlRsp(ActmControlRsp_S *rsp)
{
    if (rsp == NULL) {
        return;
    }
    for (uint8_t i = 0; i < rsp->pointNum; i++) {
        if (rsp->param[i].data != NULL) {
            SDF_MemFree(rsp->param[i].data);
        }
    }
    SDF_MemFree(rsp);
}

static void RecvControlRspBySsap(int32_t appId, NLSTK_SsapClientCallMethodResult_S *result, NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(result != NULL, "[ACTM] result is null");
    if (result->errorCode != NLSTK_ERRCODE_SUCCESS || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[ACTM] ssap control ret error");
        NotifySsapError(appId);
        return;
    }
    NLSTK_CHECK_RETURN_VOID(result->value.len > OPCODE_LEN + POINT_NUM_LEN, "[ACTM] value length error");
    NLSTK_CHECK_RETURN_VOID(result->value.data != NULL, "[ACTM] value data is null");
    uint8_t *resBuf = result->value.data;
    uint8_t opcode = resBuf[0];
    uint8_t pointNum = resBuf[1];
    NLSTK_CHECK_RETURN_VOID(result->value.len >=
        OPCODE_LEN + POINT_NUM_LEN + pointNum * (POINT_ID_LEN + RESCODE_LEN), "[ACTM] value length error");
    NLSTK_LOG_INFO("[ACTM] audio control rsp, opcode: 0x%x, point num: %d", opcode, pointNum);
    resBuf += OPCODE_LEN + POINT_NUM_LEN;
    ActmControlRsp_S *rspInfo =
        (ActmControlRsp_S *)SDF_MemZalloc(sizeof(ActmControlRsp_S) + pointNum * sizeof(ActmPointControlRes_S));
    NLSTK_CHECK_RETURN_VOID(rspInfo != NULL, "rspInfo malloc error");
    rspInfo->appId = appId;
    rspInfo->opcode = opcode;
    rspInfo->pointNum = pointNum;
    for (uint8_t i = 0; i < pointNum; i++) {
        rspInfo->param[i].pointId = resBuf[0];
        rspInfo->param[i].rescode = resBuf[1];
        rspInfo->param[i].dataLen = 0;
        rspInfo->param[i].data = NULL;
        resBuf += POINT_ID_LEN + RESCODE_LEN;
    }
    ActmControlStreamRes(rspInfo);
    FreeControlRsp(rspInfo);
}

static void ParseAccessPnt(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint8_t pointType)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt->len >= AUDIO_POINT_LEN, "[ACTM] src point len error");
    NLSTK_CHECK_RETURN_VOID(valuePkt->data != NULL, "[ACTM] src point value null");
    uint8_t *buf = valuePkt->data;
    uint8_t propId = *buf++;
    uint8_t pointId = *buf++;
    ActmAccessPoint_S *point = ActmFindPointById(device, pointId);
    if (point == NULL) {
        point = (ActmAccessPoint_S *)SDF_MemZalloc(sizeof(ActmAccessPoint_S));
        NLSTK_CHECK_RETURN_VOID(point != NULL, "[ACTM] point malloc error");
        if (!SDF_VectorEmplaceBack(device->points, point)) {
            NLSTK_LOG_ERROR("[ACTM] point vector emplace back error");
            SDF_MemFree(point);
            return;
        }
    }
    point->pointId = pointId;
    point->propId = propId;
    point->type = pointType;
    point->used = false;
    NLSTK_LOG_DEBUG("[ACTM] read access point, pointId: %d, propId: %d, pointType: %d", pointId, propId, pointType);
}

static void ParseAudioPropHandle(ActmProp_S *prop, uint8_t type, uint16_t handle)
{
    switch (type) {
        case TYPE_AUDIO_ABLITITY:
            prop->abilityHandle = handle;
            break;
        case TYPE_STREAM_TYPE:
            prop->acceptTypeHandle = handle;
            break;
        default:
            break;
    }
}

static uint8_t CountTypeNum(uint8_t type)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_PROP_NUM; i++) {
        if ((type & (1 << i)) != 0) {
            count++;
        }
    }
    return count;
}

static void ParsePropGroup(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint8_t propType)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt->len >= PROP_ID_LEN + PROP_TYPE_LEN, "[ACTM] prop group len error");
    NLSTK_CHECK_RETURN_VOID(valuePkt->data != NULL, "[ACTM] prop group value null");
    uint8_t *buf = valuePkt->data;
    uint8_t propId = *buf++;
    ActmProp_S *prop = ActmFindPropById(device, propId);
    if (prop == NULL) {
        prop = (ActmProp_S *)SDF_MemZalloc(sizeof(ActmProp_S));
        NLSTK_CHECK_RETURN_VOID(prop != NULL, "[ACTM] prop malloc error");
        if (!SDF_VectorEmplaceBack(device->props, prop)) {
            NLSTK_LOG_ERROR("[ACTM] prop vector emplace back error");
            SDF_MemFree(prop);
            return;
        }
    }
    prop->propId = propId;
    prop->propType = propType;
    uint8_t type = *buf++;
    NLSTK_CHECK_RETURN_VOID(valuePkt->len >= PROP_ID_LEN + PROP_TYPE_LEN + CountTypeNum(type) * HANDLE_LEN,
        "[ACTM] prop group len error");
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_PROP_NUM; i++) {
        if ((type & (1 << i)) != 0) {
            uint16_t handle = 0;
            (void)memcpy_s(&handle, sizeof(uint16_t), buf + count * HANDLE_LEN, sizeof(uint16_t));
            ParseAudioPropHandle(prop, 1 << i, handle);
            count++;
        }
    }
    NLSTK_LOG_DEBUG("[ACTM] read source property, propId: %d, propType: %d", propId, propType);
}

static uint32_t ParseCodecType(NLSTK_ActmCodecParam_S *codec, uint8_t *buf, uint8_t type, uint8_t paramLen)
{
    switch (type) {
        case L2HC_TYPE_VERSION:
            NLSTK_CHECK_RETURN(paramLen == sizeof(uint8_t), NLSTK_ERRCODE_FAIL, "[ACTM] version len error");
            (void)memcpy_s(&codec->l2hc.version, paramLen, buf, paramLen);
            break;
        case L2HC_TYPE_RATE:
            NLSTK_CHECK_RETURN(paramLen == sizeof(uint16_t), NLSTK_ERRCODE_FAIL, "[ACTM] rate len error");
            (void)memcpy_s(&codec->l2hc.rate, paramLen, buf, paramLen);
            break;
        case L2HC_TYPE_DEPTH:
            NLSTK_CHECK_RETURN(paramLen == sizeof(uint8_t), NLSTK_ERRCODE_FAIL, "[ACTM] depth len error");
            (void)memcpy_s(&codec->l2hc.depth, paramLen, buf, paramLen);
            break;
        case L2HC_TYPE_CHANNEL:
            NLSTK_CHECK_RETURN(paramLen == sizeof(uint8_t), NLSTK_ERRCODE_FAIL, "[ACTM] channel len error");
            (void)memcpy_s(&codec->l2hc.channel, paramLen, buf, paramLen);
            break;
        case L2HC_TYPE_FRAME:
            NLSTK_CHECK_RETURN(paramLen == sizeof(uint16_t), NLSTK_ERRCODE_FAIL, "[ACTM] frame len error");
            (void)memcpy_s(&codec->l2hc.frame, paramLen, buf, paramLen);
            break;
        case L2HC_TYPE_BPS:
            NLSTK_CHECK_RETURN(paramLen == L2HC_BPS_LEN, NLSTK_ERRCODE_FAIL, "[ACTM] bps len error");
            (void)memcpy_s(&codec->l2hc.bps, paramLen, buf, paramLen);
            break;
        default:
            NLSTK_LOG_ERROR("[ACTM] unknown codec param type: 0x%x", type);
            break;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint16_t ParseCodec(NLSTK_ActmCodecParam_S *codec, uint8_t *buf, uint16_t bufLen)
{
    uint16_t leftLen = bufLen;
    NLSTK_CHECK_RETURN(leftLen >= CODEC_ID_LEN + CODEC_PARAM_LENGTH_LEN, 0, "[ACTM] parse codec no enough len");
    codec->codecId = *buf++;
    (void)memcpy_s(&codec->companyId, sizeof(uint16_t), buf, sizeof(uint16_t));
    buf += sizeof(uint16_t);
    (void)memcpy_s(&codec->vendorId, sizeof(uint16_t), buf, sizeof(uint16_t));
    buf += sizeof(uint16_t);
    leftLen -= CODEC_ID_LEN;
    uint8_t codecLen = *buf++;
    leftLen -= CODEC_PARAM_LENGTH_LEN;
    NLSTK_CHECK_RETURN(leftLen >= (uint16_t)codecLen, 0, "[ACTM] parse codec no enough len");
    uint8_t len = 0;
    while (codecLen > len && codecLen - len >= CODEC_PARAM_TYPE_AND_PARAM_LENGTH_LEN) {
        uint8_t type = buf[0];
        uint8_t paramLen = buf[1];
        len += CODEC_PARAM_TYPE_AND_PARAM_LENGTH_LEN;
        buf += CODEC_PARAM_TYPE_AND_PARAM_LENGTH_LEN;
        NLSTK_CHECK_RETURN(codecLen > len && codecLen - len >= paramLen, 0, "[ACTM] parse codec no enough len");
        ParseCodecType(codec, buf, type, paramLen);
        len += paramLen;
        buf += paramLen;
    }
    NLSTK_LOG_DEBUG("codec id: 0x%x, company id: 0x%x, vendor id: 0x%x",
        codec->codecId, codec->companyId, codec->vendorId);
    NLSTK_LOG_DEBUG("codec version: 0x%x, rate: 0x%x, depth: 0x%x, channel: 0x%x, frame: 0x%x, bps: 0x%x",
        codec->l2hc.version, codec->l2hc.rate, codec->l2hc.depth, codec->l2hc.channel, codec->l2hc.frame,
        codec->l2hc.bps);
    return CODEC_ID_LEN + CODEC_PARAM_LENGTH_LEN + codecLen;
}

static void ParseAudioAbl(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint16_t handle)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt != NULL && valuePkt->data != NULL, "[ACTM] audio abl value null");
    uint16_t leftLen = valuePkt->len;
    uint8_t *buf = valuePkt->data;
    ActmProp_S *prop = ActmFindPropByAblHandle(device, handle);
    NLSTK_CHECK_RETURN_VOID(prop != NULL, "[ACTM] not find prop");
    NLSTK_ActmAbility_S *abl = &prop->ability;
    NLSTK_CHECK_RETURN_VOID(leftLen >= sizeof(uint8_t), "[ACTM] parse audio tbl no enough len");
    abl->codecNum = *buf++;
    leftLen -= sizeof(uint8_t);
    if (abl->codec != NULL) {
        SDF_MemFree(abl->codec);
        abl->codec = NULL;
    }

    abl->codec = (NLSTK_ActmCodecParam_S *)SDF_MemZalloc(abl->codecNum * sizeof(NLSTK_ActmCodecParam_S));
    NLSTK_CHECK_RETURN_VOID(abl->codec != NULL, "[ACTM] codec malloc error");
    for (uint8_t i = 0; i < abl->codecNum; i++) {
        uint16_t codecLen = ParseCodec(&abl->codec[i], buf, leftLen);
        buf += codecLen;
        leftLen -= codecLen;
    }
    if (leftLen < sizeof(uint8_t) + sizeof(uint32_t)) {
        NLSTK_LOG_ERROR("[ACTM] parse audio tbl no enough len for comm and supportType");
        SDF_MemFree(abl->codec);
        abl->codec = NULL;
        return;
    }
    abl->comm = *buf++;
    (void)memcpy_s(&abl->supportType, sizeof(uint32_t), buf, sizeof(uint32_t));
    NLSTK_LOG_DEBUG("comm mode: 0x%x, supportType: 0x%x", abl->comm, abl->supportType);
}

static void ParseAudioType(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint16_t handle)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt != NULL && valuePkt->data != NULL, "[ACTM] audio type value null");
    NLSTK_CHECK_RETURN_VOID(valuePkt->len == sizeof(uint32_t), "[ACTM] audio type len error");
    uint8_t *buf = valuePkt->data;
    ActmProp_S *prop = ActmFindPropByTypeHandle(device, handle);
    NLSTK_CHECK_RETURN_VOID(prop != NULL, "[ACTM] not find prop");
    (void)memcpy_s(&prop->acceptType, sizeof(uint32_t), buf, sizeof(uint32_t));
    NLSTK_LOG_DEBUG("acceptType: 0x%x", prop->acceptType);
}

static void ParseAudioLocation(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint16_t handle)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt->len == AUDIO_LOCATION_LEN, "[ACTM] audio location len error");
    uint8_t *buf = valuePkt->data;
    uint8_t type = buf[0];
    uint8_t len = buf[1];
    NLSTK_CHECK_RETURN_VOID(type == MULTI_CHANNEL_SYSTEM_TYPE && len == sizeof(uint64_t),
        "[ACTM] audio location type or len error");
    uint64_t location = 0;
    (void)memcpy_s(&location, sizeof(uint64_t), buf + sizeof(uint16_t), sizeof(uint64_t));
    if ((location & LOCATION_LEFT) != 0) {
        device->isLeft = true;
    } else if ((location & LOCATION_RIGHT) != 0) {
        device->isLeft = false;
    }
}

static void ParseAvailableStreamType(ActmRemoteDevice_S *device, NLSTK_VariableData_S *valuePkt, uint16_t handle)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt->len == AVAILABLE_STREAM_TYPE_LEN, "[ACTM] available stream type len error");
    uint32_t availableStreamType = 0;
    uint8_t *buf = valuePkt->data;
    (void)memcpy_s(&availableStreamType, sizeof(uint32_t), buf, AVAILABLE_STREAM_TYPE_LEN);
    device->availableStreamType = availableStreamType;
    NLSTK_LOG_INFO("[ACTM] available stream type update: 0x%x, addr: %s", availableStreamType,
        GET_ENC_ADDR(&device->addr));
    ActmReportAvailableStreamType(device, availableStreamType);
}

static void DecodeActmProperty(ActmRemoteDevice_S *device, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    uint16_t uuid = ConvertUuidTo16Bits(&property->uuid);
    switch (uuid) {
        case AUDIO_SOURCE_POINT_UUID:
            ParseAccessPnt(device, &property->value, NLSTK_ACTM_SOURCE_POINT);
            break;
        case AUDIO_SINK_POINT_UUID:
            ParseAccessPnt(device, &property->value, NLSTK_ACTM_SINK_POINT);
            break;
        case AUDIO_SOURCE_PROPERTY_UUID:
            ParsePropGroup(device, &property->value, NLSTK_ACTM_SOURCE_POINT);
            break;
        case AUDIO_SINK_PROPERTY_UUID:
            ParsePropGroup(device, &property->value, NLSTK_ACTM_SINK_POINT);
            break;
        case AUDIO_SOURCE_ABILITY_UUID:
            ParseAudioAbl(device, &property->value, property->handle);
            break;
        case AUDIO_SINK_ABILITY_UUID:
            ParseAudioAbl(device, &property->value, property->handle);
            break;
        case AUDIO_SOURCE_STREAM_TYPE_UUID:
            ParseAudioType(device, &property->value, property->handle);
            break;
        case AUDIO_SINK_STREAM_TYPE_UUID:
            ParseAudioType(device, &property->value, property->handle);
            break;
        case AUDIO_SINK_LOCATION_UUID:
            ParseAudioLocation(device, &property->value, property->handle);
            break;
        default:
            break;
    }
}

static void ReadRspBySsap(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[ACTM] property is null");
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        if (property->errorCode == SSAP_ERRCODE_INVALID_HANDLE) {
            NLSTK_LOG_DEBUG("[ACTM] read audio prop over");
            NotifyAudioProp(device);
        } else {
            NLSTK_LOG_ERROR("[ACTM] ssap read error, errcode: 0x%x", property->errorCode);
            ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_READ, NLSTK_ACTM_SSAP_ERROR, NULL);
        }
        return;
    }
    DecodeActmProperty(device, property);
}

static void ReadPropsRspBySsap(int32_t appId, uint8_t num, NLSTK_SsapClientReadPropertyInfo_S *properties,
    NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(properties != NULL, "[ACTM] properties is null");
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_READ, NLSTK_ACTM_SSAP_ERROR, NULL);
        return;
    }
    for (uint8_t i = 0; i < num; i++) {
        if (properties[i].errorCode != SSAP_ERRCODE_SUCCESS) {
            continue;
        }
        DecodeActmProperty(device, &properties[i]);
    }
    if (device->points->size != 0 && device->props->size != 0) {
        NotifyAudioProp(device);
    }
}

static void OnActmConnStateChanged(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] no app");
    if (state == SSAP_CONNECT_STATE_CONNECTED) {
        NLSTK_LOG_DEBUG("[ACTM] remote device connected, addr: %s", GET_ENC_ADDR(&device->addr));
        ActmReadProp(device);
    } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        NLSTK_LOG_DEBUG("[ACTM] remote device disconnected, addr: %s", GET_ENC_ADDR(&device->addr));
        SsapcAppRegParam_S param = {.appId = device->appId};
        SsapcAppDeregister(&param);
        device->appId = SSAP_APP_INVALID_ID;
        ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_DISCONNECTED, NLSTK_ACTM_SUCCESS, NULL);
        ActmDeviceOffline(device, true);
        ActmDestroyRemoteDevice(&device->addr);
    }
}

static void BitrateUpdate(ActmRemoteDevice_S *device, uint8_t *data, uint8_t pointNum)
{
    ActmAccessPoint_S *point = NULL;
    for (uint8_t i = 0; i < pointNum; i++) {
        uint8_t pointId = *data++;
        uint8_t len = *data++;
        if (len != L2HC_BPS_LEN) {
            NLSTK_LOG_ERROR("[ACTM] not l2hc bps len");
            return;
        }
        point = ActmFindPointById(device, pointId);
        if (point == NULL || point->type != NLSTK_ACTM_SINK_POINT) {
            NLSTK_LOG_ERROR("[ACTM] point error");
            data += len;
            continue;
        }
        uint64_t bps = 0;
        (void)memcpy_s(&bps, L2HC_BPS_LEN, data, L2HC_BPS_LEN);
        ActmUpdateBitRate(device, bps);
        return;
    }
}

static void RecvPropertyNtf(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_CHECK_RETURN_VOID(property != NULL && property->errorCode == NLSTK_ERRCODE_SUCCESS &&
        property->value.data != NULL, "[ACTM] param error");
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] no app");
    if (property->handle == device->info.locationHandle) {
        ParseAudioLocation(device, &property->value, property->handle);
        NLSTK_LOG_INFO("[ACTM] location changed, addr: %s, isLeft: %d", GET_ENC_ADDR(&device->addr), device->isLeft);
        ActmLocationCbk(&device->addr, device->isLeft);
    } else if (property->handle == device->info.availableStreamTypeHandle) {
        ParseAvailableStreamType(device, &property->value, property->handle);
    }
}

static void RecvEventNtf(int32_t appId, NLSTK_SsapClientEventInfo_S *event)
{
    NLSTK_CHECK_RETURN_VOID(event != NULL && event->errorCode == NLSTK_ERRCODE_SUCCESS &&
        event->value.data != NULL, "[ACTM] param error");
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] no app");
    NLSTK_CHECK_RETURN_VOID(device->info.eventHandle == event->handle, "[ACTM] event handle error");
    uint16_t dataLen = event->value.len;
    uint8_t *data = event->value.data;
    NLSTK_CHECK_RETURN_VOID(dataLen > OPCODE_LEN + POINT_NUM_LEN, "[ACTM] data len error");
    uint8_t opcode = data[0];
    uint8_t pointNum = data[1];
    uint8_t pointLen = POINT_ID_LEN;
    if (opcode == ACTM_EVENT_BITRATE_UPDATE) {
        pointLen += BITRATE_UPDATE_LEN;
    }
    NLSTK_LOG_INFO("[ACTM] recv event notify, opcode: 0x%x, addr: %s", opcode, GET_ENC_ADDR(&device->addr));
    NLSTK_CHECK_RETURN_VOID(dataLen >= OPCODE_LEN + POINT_NUM_LEN + pointNum * pointLen, "[ACTM] data len error");
    switch (opcode) {
        case ACTM_EVENT_BITRATE_UPDATE:
            BitrateUpdate(device, data + OPCODE_LEN + POINT_NUM_LEN, pointNum);
            break;
        case ACTM_EVENT_TRANS:
            NLSTK_CHECK_RETURN_VOID(device->ssapState != ACTM_TRANS_COMPL, "[ACTM] already trans");
            device->ssapState = ACTM_TRANS_COMPL;
            ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_SUCCESS, NULL);
            break;
        case ACTM_EVENT_STOP:
            NLSTK_CHECK_RETURN_VOID(device->ssapState != ACTM_STOP_COMPL, "[ACTM] already stop");
            device->ssapState = ACTM_STOP_COMPL;
            ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_STOP, NLSTK_ACTM_SUCCESS, NULL);
            break;
        case ACTM_EVENT_RELEASE:
            NLSTK_CHECK_RETURN_VOID(device->ssapState != ACTM_RELEASE_COMPL, "[ACTM] already release");
            break;
        default:
            NLSTK_LOG_ERROR("[ACTM] unsupport event opcode: 0x%x", opcode);
            break;
    }
}

void ActmGetSsapCb(NLSTK_SsapAppClientCb_S *cb)
{
    NLSTK_CHECK_RETURN_VOID(cb != NULL, "[ACTM] cb is null");
    cb->onConnectionStateChanged = OnActmConnStateChanged;
    cb->onReadProperty = ReadRspBySsap;
    cb->onReadProperties = ReadPropsRspBySsap;
    cb->onPropertyChanged = RecvPropertyNtf;
    cb->onCallMethod = RecvControlRspBySsap;
    cb->onEvent = RecvEventNtf;
}