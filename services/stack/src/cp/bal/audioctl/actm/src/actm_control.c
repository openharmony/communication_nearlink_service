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
#include "actm_control.h"
#include <stdbool.h>
#include "actm_tbl.h"
#include "actm_callback.h"
#include "actm_qosm_adapter.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_cfgdb.h"
#include "qosm_autorate.h"
#include "securec.h"

#define INVALID_SSAP_HANDLE 0xFFFF

#define PARAM_TYPE_LEN 1
#define CODEC_L2HC_CONFIG_LEN 30
#define CHANNEL_REQ_LEN 3
#define CHANNEL_REQ_WITH_QOS_LEN 15
#define META_INFO_LEN 13
#define PORT_LEN 4

#define CONFIG_TYPE_CODEC           0x01
#define CONFIG_TYPE_CHANNEL         0X02
#define CONFIG_TYPE_META_INFO       0x03
#define CONFIG_TYPE_PORT            0x04

#define META_TYPE_LEN 1
#define META_TYPE_STREAM_TYPE     0x01
#define META_TYPE_STREAM_DURATION 0x02
#define META_TYPE_MEDIA_ID        0x03

#define CONFIG_POINT_MAX_LEN 255

#define SDU_INTERVAL_LENGTH 3

#define MULTI_CHANNEL_TYPE 0x01
#define MULTI_CHANNEL_LEN 8

#define MAX_POINT_NUM 2

void NotifyAudioProp(ActmRemoteDevice_S *device)
{
    uint8_t pointNum = (uint8_t)device->points->size;
    ActmAccessPoint_S *point = NULL;
    ActmProp_S *prop = NULL;
    NLSTK_ActmProp_S *props = (NLSTK_ActmProp_S *)SDF_MemZalloc(pointNum * sizeof(NLSTK_ActmProp_S));
    NLSTK_CHECK_RETURN_VOID(props != NULL, "[ACTM] props malloc error");
    for (uint8_t i = 0; i < pointNum; i++) {
        point = SDF_VectorElementAt(device->points, i);
        prop = ActmFindPropById(device, point->propId);
        props[i].pointType = point->type;
        if (prop == NULL || point->type != prop->propType) {
            NLSTK_LOG_ERROR("[ACTM] point has wrong prop");
            continue;
        }
        (void)memcpy_s(&props[i].ability, sizeof(NLSTK_ActmAbility_S), &prop->ability, sizeof(NLSTK_ActmAbility_S));
        props[i].acceptType = prop->acceptType;
    }
    ActmPropCbk(&device->addr, pointNum, props);
    if (device->info.locationHandle != 0) {
        ActmLocationCbk(&device->addr, device->isLeft);
    }
    SDF_MemFree(props);
}

void ActmReadProp(ActmRemoteDevice_S *device)
{
    NLSTK_CHECK_RETURN_VOID(ActmGetService(device), "[ACTM] get service failed");
    if (!CfgdbGetManufacturerSupport(&device->addr, CFGDB_READ_MULTI_HANDLES)) {
        NLSTK_SsapClientReadProperty(device->appId, INVALID_SSAP_HANDLE);
    }
}

static void ConfigL2HCParam(uint8_t *data, NLSTK_ActmCodecConfig_S *param)
{
    *data++ = CONFIG_TYPE_CODEC;
    *data++ = param->codecId;
    (void)memcpy_s(data, sizeof(uint16_t), &param->companyId, sizeof(uint16_t));
    data += sizeof(uint16_t);
    (void)memcpy_s(data, sizeof(uint16_t), &param->vendorId, sizeof(uint16_t));
    data += sizeof(uint16_t);
    *data++ = CODEC_L2HC_CONFIG_LEN;

    *data++ = L2HC_TYPE_VERSION;
    *data++ = sizeof(uint8_t);
    *data++ = param->l2hc.version;

    *data++ = L2HC_TYPE_RATE;
    *data++ = sizeof(uint8_t);
    *data++ = param->l2hc.rateConf;

    *data++ = L2HC_TYPE_DEPTH;
    *data++ = sizeof(uint8_t);
    *data++ = param->l2hc.depthConf;

    *data++ = L2HC_TYPE_CHANNEL;
    *data++ = sizeof(uint8_t) + MULTI_CHANNEL_LEN;
    *data++ = MULTI_CHANNEL_TYPE;
    (void)memset_s(data, MULTI_CHANNEL_LEN, 0, MULTI_CHANNEL_LEN);
    data += MULTI_CHANNEL_LEN;

    *data++ = L2HC_TYPE_FRAME;
    *data++ = sizeof(uint8_t);
    *data++ = param->l2hc.frameConf;

    *data++ = L2HC_TYPE_BPS;
    *data++ = L2HC_BPS_LEN;
    (void)memcpy_s(data, L2HC_BPS_LEN, &param->l2hc.bpsRange, L2HC_BPS_LEN);
}

static uint8_t ConfigChannelReq(uint8_t *data, NLSTK_ActmChannelConfig_S *channel, ActmRemoteDevice_S *device,
    ActmAccessPoint_S *point)
{
    *data++ = CONFIG_TYPE_CHANNEL;
    *data++ = channel->comm;
    *data++ = channel->trans;
    if (channel->trans == 0) {
        *data++ = channel->qosId;
        return PARAM_TYPE_LEN + CHANNEL_REQ_LEN;
    }
    uint8_t qosIdLen = 0;
    if (CfgdbGetManufacturerSupport(&device->addr, CFGDB_QOS_ID_CONFIG)) {
        *data++ = channel->qosId;
        qosIdLen = 1;
    }
    *data++ = device->groupId; // 事件组集合标识
    *data++ = device->mebId; // 事件组标识

    QOSM_ICBParam param = {0};
    if (point->type == NLSTK_ACTM_SOURCE_POINT) {
        QOSM_AutoRateGetICGT2GParam(channel->qosId, &param);
    } else {
        QOSM_AutoRateGetICGG2TParam(channel->qosId, &param);
    }
    *data++ = 0; // 业务适配方式
    (void)memcpy_s(data, SDU_INTERVAL_LENGTH, &param.sduInterval, SDU_INTERVAL_LENGTH); // SDU间隔
    data += SDU_INTERVAL_LENGTH;
    (void)memcpy_s(data, sizeof(uint16_t), &param.maxSdu, sizeof(uint16_t)); // 最大SDU长度
    data += sizeof(uint16_t);
    *data++ = param.rtn; // 最大重传次数
    (void)memcpy_s(data, sizeof(uint16_t), &param.maxLatency, sizeof(uint16_t)); // 最大传输时延
    data += sizeof(uint16_t);
    (void)memset_s(data, sizeof(uint16_t), 0, sizeof(uint16_t)); // 缓存时延

    return PARAM_TYPE_LEN + CHANNEL_REQ_WITH_QOS_LEN + qosIdLen;
}

static void ConfigMetaInfo(uint8_t *data, NLSTK_ActmConfig_S *config)
{
    *data++ = CONFIG_TYPE_META_INFO;
    *data++ = META_TYPE_STREAM_TYPE;
    *data++ = sizeof(uint32_t);
    (void)memcpy_s(data, sizeof(uint32_t), &config->streamType, sizeof(uint32_t));
    data += sizeof(uint32_t);
    *data++ = META_TYPE_STREAM_DURATION;
    *data++ = sizeof(uint8_t);
    *data++ = config->duration;
    *data++ = META_TYPE_MEDIA_ID;
    *data++ = sizeof(uint8_t) + sizeof(uint8_t);
    *data++ = 1; // 目前只有一个通用媒体实例标识
    *data++ = config->mediaId;
}

static void SaveStreamConfig(ActmStream_S *stream, NLSTK_ActmConfig_S *config)
{
    stream->qosIndex = config->channel.qosId;
    (void)memcpy_s(&stream->codec, sizeof(NLSTK_ActmCodecConfig_S), &config->codec, sizeof(NLSTK_ActmCodecConfig_S));
}

void ActmConfigPoint(ActmRemoteDevice_S *device, ActmStream_S *stream, ActmAccessPoint_S *point,
    NLSTK_ActmConfig_S *config, ActmPointControlInfo_S *info)
{
    info->pointId = point->pointId;
    uint8_t buf[CONFIG_POINT_MAX_LEN] = {0};
    uint8_t *data = &buf[0];
    uint8_t dataLen = sizeof(uint16_t);

    data += sizeof(uint16_t); // 配置参数总长度最后再填充

    ConfigL2HCParam(data, &config->codec);
    data += PARAM_TYPE_LEN + CODEC_ID_LEN + CODEC_PARAM_LENGTH_LEN + CODEC_L2HC_CONFIG_LEN;
    dataLen += PARAM_TYPE_LEN + CODEC_ID_LEN + CODEC_PARAM_LENGTH_LEN + CODEC_L2HC_CONFIG_LEN;

    uint8_t channelLen = ConfigChannelReq(data, &config->channel, device, point);
    data += channelLen;
    dataLen += channelLen;

    ConfigMetaInfo(data, config);
    data += PARAM_TYPE_LEN + META_INFO_LEN;
    dataLen += PARAM_TYPE_LEN + META_INFO_LEN;

    if (config->channel.trans == 0) { // 透传方式为不透传时配置
        *data++ = CONFIG_TYPE_PORT;
        (void)memcpy_s(data, sizeof(uint16_t), &config->src, sizeof(uint16_t));
        data += sizeof(uint16_t);
        (void)memcpy_s(data, sizeof(uint16_t), &config->dst, sizeof(uint16_t));
        dataLen += PARAM_TYPE_LEN + PORT_LEN;
    }

    uint16_t paramLen = dataLen - sizeof(uint16_t);
    data = &buf[0];
    (void)memcpy_s(data, sizeof(uint16_t), &paramLen, sizeof(uint16_t));

    info->data = (uint8_t *)SDF_MemZalloc(dataLen);
    NLSTK_CHECK_RETURN_VOID(info->data != NULL, "[ACTM] config data malloc error");
    info->dataLen = dataLen;
    (void)memcpy_s(info->data, dataLen, buf, dataLen);
    NLSTK_LOG_INFO("[ACTM] config point, addr: %s, id: %d, param: %s", GET_ENC_ADDR(&device->addr), point->pointId,
        SDF_GET_UINT8_STR(info->data, info->dataLen));

    if (point->type == NLSTK_ACTM_SINK_POINT || (point->type == NLSTK_ACTM_SOURCE_POINT &&
        config->channel.qosId == QOSINDEX_5)) {
        SaveStreamConfig(stream, config);
    }
}

static uint8_t CountPointNum(ActmStream_S *stream, uint8_t *points)
{
    uint8_t pointNum = 0;
    if ((stream->pointType & NLSTK_ACTM_SOURCE_POINT) != 0) {
        points[pointNum] = stream->srcPointId;
        pointNum++;
    }
    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0) {
        points[pointNum] = stream->sinkPointId;
        pointNum++;
    }
    return pointNum;
}

void ActmOpenStream(ActmRemoteDevice_S *device, ActmStream_S *stream)
{
    uint8_t points[MAX_POINT_NUM] = {0};
    uint8_t pointNum = CountPointNum(stream, points);
    NLSTK_CHECK_RETURN_VOID(pointNum != 0, "[ACTM] pointNum error");
    ActmControlReq_S *req = (ActmControlReq_S *)SDF_MemZalloc(sizeof(ActmControlReq_S) +
        pointNum * sizeof(ActmPointControlInfo_S));
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] open req malloc error");
    req->appId = device->appId;
    req->handle = device->info.ctrlHandle;
    req->opcode = ACTM_CONTROL_OPEN_PATH;
    req->pointNum = pointNum;

    for (uint8_t i = 0; i < pointNum; i++) {
        req->param[i].pointId = points[i];
        req->param[i].dataLen = 0;
        req->param[i].data = NULL;
    }
    ActmControlReqBySsap(req);
    SDF_MemFree(req);
}

static uint8_t GetOpCode(uint8_t op)
{
    switch (op) {
        case NLSTK_ACTM_STREAM_TRANS:
            return ACTM_CONTROL_TRANS;
        case NLSTK_ACTM_STREAM_STOP:
            return ACTM_CONTROL_STOP;
        default:
            return 0;
    }
}

void ActmChangeStream(ActmRemoteDevice_S *device, ActmStream_S *stream, uint8_t op)
{
    uint8_t points[MAX_POINT_NUM] = {0};
    uint8_t pointNum = CountPointNum(stream, points);
    NLSTK_CHECK_RETURN_VOID(pointNum != 0, "[ACTM] pointNum error");
    ActmControlReq_S *req = (ActmControlReq_S *)SDF_MemZalloc(sizeof(ActmControlReq_S) +
        pointNum * sizeof(ActmPointControlInfo_S));
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] change req malloc error");
    req->appId = device->appId;
    req->handle = device->info.ctrlHandle;
    req->opcode = GetOpCode(op);
    req->pointNum = pointNum;

    for (uint8_t i = 0; i < pointNum; i++) {
        req->param[i].pointId = points[i];
        req->param[i].dataLen = 0;
        req->param[i].data = NULL;
    }
    ActmControlReqBySsap(req);
    SDF_MemFree(req);
}

void ActmReleaseStream(ActmRemoteDevice_S *device, ActmStream_S *stream)
{
    uint8_t points[MAX_POINT_NUM] = {0};
    uint8_t pointNum = CountPointNum(stream, points);
    NLSTK_CHECK_RETURN_VOID(pointNum != 0, "[ACTM] pointNum error");
    ActmControlReq_S *req = (ActmControlReq_S *)SDF_MemZalloc(sizeof(ActmControlReq_S) +
        pointNum * sizeof(ActmPointControlInfo_S));
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] release req malloc error");
    req->appId = device->appId;
    req->handle = device->info.ctrlHandle;
    req->opcode = ACTM_CONTROL_RELEASE;
    req->pointNum = pointNum;

    uint8_t ind[1] = {0x00}; // 释放指示默认为0x00
    for (uint8_t i = 0; i < pointNum; i++) {
        req->param[i].pointId = points[i];
        req->param[i].dataLen = sizeof(uint8_t);
        req->param[i].data = ind;
    }
    ActmControlReqBySsap(req);
    SDF_MemFree(req);
}

void ActmChangeBitrate(ActmRemoteDevice_S *device, ActmStream_S *stream, uint64_t bitrate)
{
    uint8_t points[MAX_POINT_NUM] = {0};
    uint8_t pointNum = CountPointNum(stream, points);
    NLSTK_CHECK_RETURN_VOID(pointNum != 0, "[ACTM] pointNum error");
    ActmControlReq_S *req = (ActmControlReq_S *)SDF_MemZalloc(sizeof(ActmControlReq_S) +
        pointNum * (sizeof(ActmPointControlInfo_S) + sizeof(uint8_t) + L2HC_BPS_LEN));
    NLSTK_CHECK_RETURN_VOID(req != NULL, "[ACTM] bitrate req malloc error");
    req->appId = device->appId;
    req->handle = device->info.ctrlHandle;
    req->opcode = ACTM_CONTROL_BITRATE_UPDATE;
    req->pointNum = pointNum;

    uint8_t bps[L2HC_BPS_LEN + 1] = {0};
    bps[0] = L2HC_BPS_LEN;
    (void)memcpy_s(bps + sizeof(uint8_t), L2HC_BPS_LEN, &bitrate, L2HC_BPS_LEN);
    for (uint8_t i = 0; i < pointNum; i++) {
        req->param[i].pointId = points[i];
        req->param[i].dataLen = sizeof(uint8_t) + L2HC_BPS_LEN;
        req->param[i].data = bps;
    }

    if ((stream->pointType & NLSTK_ACTM_SINK_POINT) != 0 && stream->qosIndex != QOSINDEX_3) {
        (void)memcpy_s(&stream->codec.l2hc.bpsRange, L2HC_BPS_LEN, &bitrate, L2HC_BPS_LEN);
    }
    ActmControlReqBySsap(req);
    SDF_MemFree(req);
}

static void PrintDeviceState(ActmRemoteDevice_S *device)
{
    if (device == NULL) {
        return;
    }
    NLSTK_LOG_DEBUG("[ACTM] device addr: %s, ssap state: %d, qosm state: %d", GET_ENC_ADDR(&device->addr),
        device->ssapState, device->qosmState);
}

static void ProcessConfig(ActmRemoteDevice_S *device)
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
        if (tmpDevice->ssapState >= ACTM_START_CONFIG && tmpDevice->ssapState < ACTM_CONFIG_COMPL) {
            NLSTK_LOG_INFO("[ACTM] wait other device config, addr: %s", GET_ENC_ADDR(&device->addr));
            return;
        }
    }
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->ssapState != ACTM_CONFIG_COMPL) {
            PrintDeviceState(tmpDevice);
            continue;
        }
        ActmEventCbk(&tmpDevice->addr, NLSTK_ACTM_EVENT_CONFIG, NLSTK_ACTM_SUCCESS, NULL);
    }
}

static void ProcessOpen(ActmRemoteDevice_S *device)
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
        if (tmpDevice->ssapState >= ACTM_START_CONFIG && tmpDevice->ssapState < ACTM_OPEN_COMPL) {
            NLSTK_LOG_INFO("[ACTM] wait other device open, addr: %s", GET_ENC_ADDR(&device->addr));
            return;
        }
    }
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->ssapState != ACTM_OPEN_COMPL) {
            PrintDeviceState(tmpDevice);
            continue;
        }
        ActmEventCbk(&tmpDevice->addr, NLSTK_ACTM_EVENT_OPEN, NLSTK_ACTM_SUCCESS, NULL);
    }
}

static void ProcessStart(ActmRemoteDevice_S *device)
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
        if (tmpDevice->ssapState >= ACTM_START_CONFIG && tmpDevice->ssapState < ACTM_TRANS_COMPL) {
            NLSTK_LOG_INFO("[ACTM] wait other device start, addr: %s", GET_ENC_ADDR(&device->addr));
            return;
        }
    }
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->ssapState != ACTM_TRANS_COMPL ||
            (tmpDevice->qosmState != ACTM_CONN_COMPL && tmpDevice->qosmState != ACTM_QOSM_FAIL)) {
            PrintDeviceState(tmpDevice);
            continue;
        }
        if (tmpDevice->qosmState == ACTM_QOSM_FAIL) {
            ActmEventCbk(&tmpDevice->addr, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_QOSM_ERROR, NULL);
            tmpDevice->ssapState = ACTM_SSAP_IDLE;
            continue;
        }
        ActmNotifyConnState(tmpDevice);
    }
}

static void ProcessStop(ActmRemoteDevice_S *device)
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
        if (tmpDevice->ssapState >= ACTM_START_STOP && tmpDevice->ssapState < ACTM_STOP_COMPL) {
            NLSTK_LOG_INFO("[ACTM] wait other device stop, addr: %s", GET_ENC_ADDR(&device->addr));
            return;
        }
    }
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL || tmpDevice->ssapState != ACTM_STOP_COMPL) {
            PrintDeviceState(tmpDevice);
            continue;
        }
        ActmEventCbk(&tmpDevice->addr, NLSTK_ACTM_EVENT_STOP, NLSTK_ACTM_SUCCESS, NULL);
    }
}

static void ProcessRelease(ActmRemoteDevice_S *device)
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
        if (tmpDevice->ssapState >= ACTM_START_RELEASE && tmpDevice->ssapState < ACTM_RELEASE_COMPL) {
            NLSTK_LOG_INFO("[ACTM] wait other device release, addr: %s", GET_ENC_ADDR(&device->addr));
            return;
        }
    }
    ActmNotifyDisconnState(device);
}

static void ProcessBitrate(ActmRemoteDevice_S *device)
{
    ActmStream_S *stream = ActmFindStreamById(device, device->curStreamId);
    NLSTK_CHECK_RETURN_VOID(stream != NULL, "[ACTM] not find stream");
    ActmUpdateBitRate(device, stream->codec.l2hc.bpsRange);
    ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_BITRATE, NLSTK_ACTM_SUCCESS, NULL);
}

static void ControlResChangeQosm(uint8_t event, ActmRemoteDevice_S *device)
{
    switch (event) {
        case NLSTK_ACTM_EVENT_CONFIG:
            device->ssapState = ACTM_CONFIG_COMPL;
            ProcessConfig(device);
            break;
        case NLSTK_ACTM_EVENT_OPEN:
            device->ssapState = ACTM_OPEN_COMPL;
            ProcessOpen(device);
            break;
        case NLSTK_ACTM_EVENT_TRANS:
            device->ssapState = ACTM_TRANS_COMPL;
            ProcessStart(device);
            break;
        case NLSTK_ACTM_EVENT_STOP:
            device->ssapState = ACTM_STOP_COMPL;
            ProcessStop(device);
            break;
        case NLSTK_ACTM_EVENT_RELEASE:
            device->ssapState = ACTM_RELEASE_COMPL;
            ProcessRelease(device);
            break;
        case NLSTK_ACTM_EVENT_BITRATE:
            ProcessBitrate(device);
            break;
        default:
            break;
    }
}

static uint8_t GetEventByOpcode(uint8_t opcode)
{
    uint8_t event = 0;
    switch (opcode) {
        case ACTM_CONTROL_CONFIG:
            event = NLSTK_ACTM_EVENT_CONFIG;
            break;
        case ACTM_CONTROL_OPEN_PATH:
            event = NLSTK_ACTM_EVENT_OPEN;
            break;
        case ACTM_CONTROL_TRANS:
            event = NLSTK_ACTM_EVENT_TRANS;
            break;
        case ACTM_CONTROL_STOP:
            event = NLSTK_ACTM_EVENT_STOP;
            break;
        case ACTM_CONTROL_RELEASE:
            event = NLSTK_ACTM_EVENT_RELEASE;
            break;
        case ACTM_CONTROL_BITRATE_UPDATE:
            event = NLSTK_ACTM_EVENT_BITRATE;
            break;
        default:
            break;
    }
    return event;
}

static void ProcessStartError(ActmRemoteDevice_S *device)
{
    if (device->qosmState == ACTM_ADD_CONN) {
        return;
    }
    if (device->qosmState == ACTM_CONN_COMPL) {
        ActmDelConnection(device, ActmGetConnHandle(&device->addr, device->groupId));
    }
    ActmEventCbk(&device->addr, NLSTK_ACTM_EVENT_TRANS, device->lastErr, NULL);
    ActmDeviceOffline(device, false);
}

static void ControlResError(ActmRemoteDevice_S *device, uint8_t event, uint8_t errcode)
{
    switch (event) {
        case NLSTK_ACTM_EVENT_CONFIG:
        case NLSTK_ACTM_EVENT_OPEN:
        case NLSTK_ACTM_EVENT_STOP:
            ActmEventCbk(&device->addr, event, errcode, NULL);
            ActmDeviceOffline(device, false);
            break;
        case NLSTK_ACTM_EVENT_TRANS:
            device->ssapState = ACTM_START_FAIL;
            ProcessStartError(device);
            break;
        case NLSTK_ACTM_EVENT_RELEASE:
            device->ssapState = ACTM_RELEASE_COMPL;
            ProcessRelease(device);
            break;
        case NLSTK_ACTM_EVENT_BITRATE:
            ActmEventCbk(&device->addr, event, errcode, NULL);
            break;
        default:
            break;
    }
}

void ActmControlStreamRes(ActmControlRsp_S *rsp)
{
    ActmRemoteDevice_S *device = ActmFindDeviceByAppId(rsp->appId);
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[ACTM] not find device");
    uint8_t errcode = 0;
    for (uint8_t i = 0; i < rsp->pointNum; i++) {
        if (rsp->param[i].rescode != 0) {
            errcode = rsp->param[i].rescode;
        }
    }
    uint8_t event = GetEventByOpcode(rsp->opcode);
    if (event == NLSTK_ACTM_EVENT_CONFIG && device->isPriv) {
        event = NLSTK_ACTM_EVENT_TRANS;
    }
    if (errcode != 0) {
        NLSTK_LOG_ERROR("[ACTM] control stream res error, opcode: %d", rsp->opcode);
        device->lastErr = errcode;
        ControlResError(device, event, errcode);
        return;
    }
    ControlResChangeQosm(event, device);
}

static bool CompDevice(void *ptr, void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)ptr;
    ActmRemoteDevice_S *device = (ActmRemoteDevice_S *)arg;
    return (memcmp(addr, &device->addr, sizeof(SLE_Addr_S)) == 0);
}

void ActmReportAvailableStreamType(ActmRemoteDevice_S *device, uint32_t streamType)
{
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    SLE_Addr_S *addr = NULL;
    ActmRemoteDevice_S *tmpDevice = NULL;
    /* 检查双耳streamType，等到双耳streamType都更新一致后再上报，保证双耳同步 */
    for (size_t i = 0; i < group->devices->size; i++) {
        addr = SDF_VectorElementAt(group->devices, i);
        tmpDevice = ActmFindDeviceByAddr(addr);
        if (tmpDevice == NULL) {
            continue;
        }
        if (tmpDevice->availableStreamType != streamType) {
            return;
        }
    }
    if (tmpDevice != NULL) {
        ActmStreamTypeCbk(&tmpDevice->addr, streamType);
    }
}

void ActmDeviceOffline(ActmRemoteDevice_S *device, bool remove)
{
    device->ssapState = ACTM_SSAP_IDLE;
    device->lastErr = NLSTK_ACTM_SUCCESS;
    ProcessConfig(device);
    ProcessOpen(device);
    ProcessStart(device);
    ProcessStop(device);
    ProcessRelease(device);
    if (!remove) {
        return;
    }
    ActmQosmGroup_S *group = ActmFindQosmGroupById(device->groupId);
    NLSTK_CHECK_RETURN_VOID(group != NULL, "[ACTM] not find group");
    size_t index = 0;
    if (!SDF_VectorFindFirst(group->devices, CompDevice, device, &index)) {
        NLSTK_LOG_ERROR("[ACTM] not find device");
        return;
    }
    SDF_VectorRemove(group->devices, index);
}