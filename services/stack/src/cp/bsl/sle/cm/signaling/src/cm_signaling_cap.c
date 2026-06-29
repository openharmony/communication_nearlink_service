/****************************************************************************
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
****************************************************************************/

#include "cm_signaling_cap.h"

#include <netinet/in.h>
#include "securec.h"

#include "byte_codec.h"
#include "cm_log.h"
#include "cm_errno.h"
#include "dli.h"
#include "dli_cmd_struct.h"
#include "sdf_mem.h"
#include "nbc_api.h"
#include "cm_signaling_version.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_internal.h"

#define CM_INVALID_RET (-1)
#define CM_DEFAULT_VERSION 0
#define CM_CAPABILITY_RSP_LEN 100
#define CM_16BIT_CNT 3
#define CM_48BIT_LEN (sizeof(uint16_t) * CM_16BIT_CNT)

#define CM_SIGNALING_HDR_LEN_OFFSET 2U

#define CM_CAP_DEFAULT_TRANS_MODE 1
#define CM_CAP_DEFAULT_VALUE ((1 << CM_SIGNAL_TRANS_MODE) | (1 << CM_SIGNAL_ACCESS_SLE) | \
    (1 << CM_SIGNAL_MTU) | (1 << CM_SIGNAL_MPS) | (1 << CM_SIGNAL_VERSION) | (1 << CM_SIGNAL_WND))

typedef struct CM_LocalSupportInfo {
    uint32_t cap;
    uint8_t measureEnable;
    uint8_t identifier;
    CM_TransMode transMode;
    uint16_t defaultVeraion;
    uint16_t mtu;
    uint16_t mps;
    uint8_t wnd;
} CM_LocalSupportInfo;

static CM_LocalSupportInfo g_localInfo = {
    .cap = CM_CAP_DEFAULT_VALUE,
    .measureEnable = CM_CAP_DISABLE,
    .transMode.byPassMode = CM_CAP_DISABLE,
    .transMode.flowMode = CM_CAP_ENABLE,
    .transMode.reliableMode = CM_CAP_ENABLE,
    .transMode.reverse = 0,
    .defaultVeraion = CM_CAP_DEFAULT_VERSION,
    .mtu = CM_CAP_MTU,
    .mps = CM_CAP_MPS,
    .wnd = CM_CAP_WND,
};

uint32_t g_localSupportCapability =
    (1 << CM_SIGNAL_TRANS_MODE) |
    (1 << CM_SIGNAL_ACCESS_SLE) |
    (1 << CM_SIGNAL_MTU) |
    (1 << CM_SIGNAL_MPS) |
    (1 << CM_SIGNAL_VERSION);

SDF_Buff_S *CM_CreateSignalingBuff(uint8_t code, uint8_t identifier,
    uint8_t *data, uint16_t length)
{
    uint32_t dataLen = (uint32_t)sizeof(CM_SignalingHead_S) + length;
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(dataLen);
    CM_CHECK_RETURN_RET(buf != NULL, NULL, "CM_CreateSignalingBuff create buf error");
    CM_SignalingHead_S *head = (CM_SignalingHead_S*)SDF_BuffAppend(buf, dataLen);
    if (head == NULL) {
        CM_LOGE("CM_CreateSignalingBuff create head error");
        SDF_BuffFree(buf);
        return NULL;
    }
    head->code = code;
    head->identifier = identifier;
    ENCODE2BYTE_LITTLE(&head->length, length);
    CM_LOGI("CM_CreateSignalingBuff code %02x id %hhu create head len %hu dataLen %hu",
            code, identifier, length, dataLen);
    (void)memcpy_s(head->data, length, data, length);
    return buf;
}

// dtap侧过来的数据，剥离dtap的head，在此需要解析ControlHead
CM_SignalingHead_S *CM_ParseSignalingBuff(SDF_Buff_S *buf)
{
    uint64_t dataLen = SDF_DataLenGet(buf);
    if (dataLen < sizeof(CM_SignalingHead_S)) {
        CM_LOGE("CM_ParseSignalingBuff buf len %lu is error", dataLen);
        return NULL;
    }
    CM_SignalingHead_S *head = CM_GET_SIGNALING_HEAD(buf);
    if (head == NULL) {
        CM_LOGE("CM_ParseSignalingBuff create head error");
        return NULL;
    }

    head->length = DECODE2BYTE_LITTLE((uint8_t *)SDF_DataOffset(buf) + CM_SIGNALING_HDR_LEN_OFFSET);
    if (head->length != dataLen - sizeof(CM_SignalingHead_S)) {
        CM_LOGE("CM_ParseSignalingBuff length %lu is no match dataLen %hu", head->length, dataLen);
        return NULL;
    }
    CM_LOGD("length %u", head->length);
    return head;
}

static inline int SetRelayCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    (void)data;
    (void)len;
    (void)value;
    return 0;
}

static int SetTransModeCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t tmpLen = sizeof(uint16_t);
    if (len < tmpLen) {
        return CM_INVALID_RET;
    }
    uint16_t tmp = DECODE2BYTE_LITTLE(data);
    if (memcpy_s(&value->transMode, sizeof(value->transMode), &tmp, tmpLen) != 0) {
        CM_LOGE("SetTransModeCapability memcpy failed\r\n");
        return CM_INVALID_RET;
    }
    CM_LOGI("SetVersionCapability set capInfo transMode 0x%04x", tmp);
    return (int)tmpLen;
}

static inline int SetMeasurementCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    (void)data;
    (void)len;
    (void)value;
    CM_LOGI("SetVersionCapability set enable Measurement capability");
    return 0;
}

static int SetAccessSleCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t aceLen = sizeof(value->accessSle);
    if (len < aceLen) {
        return CM_INVALID_RET;
    }
    if (memcpy_s(value->accessSle, aceLen, data, aceLen) != 0) {
        CM_LOGE("SetAccessSleCapability memcpy failed\r\n");
        return CM_INVALID_RET;
    }
    CM_LOGI("SetVersionCapability set capInfo AccessSle %02X:%02X:**:**:**:%02X",
        value->accessSle[SLE_INDEX_0], value->accessSle[SLE_INDEX_1], value->accessSle[SLE_INDEX_5]);
    return (int)aceLen;
}

static int SetAccessSlbCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t acbLen = sizeof(value->accessSlb);
    if (len < acbLen) {
        return CM_INVALID_RET;
    }
    if (memcpy_s(value->accessSlb, acbLen, data, acbLen) != 0) {
        CM_LOGE("SetAccessSlbCapability memcpy failed\r\n");
        return CM_INVALID_RET;
    }
    CM_LOGI("SetVersionCapability set capInfo AccessSlb %02X:%02X:**:**:**:%02X",
        value->accessSlb[SLE_INDEX_0], value->accessSlb[SLE_INDEX_1], value->accessSlb[SLE_INDEX_5]);
    return (int)acbLen;
}

static inline int SetMtuCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t mtuLen = sizeof(value->mtu);
    if (len < mtuLen) {
        return CM_INVALID_RET;
    }
    value->mtu = DECODE2BYTE_LITTLE(data);
    CM_LOGD("SetVersionCapability set capInfo mtu %hu", value->mtu);
    return (int)mtuLen;
}

static inline int SetMpsCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t mpsLen = (uint32_t)sizeof(value->mps);
    if (len < mpsLen) {
        return CM_INVALID_RET;
    }
    value->mps = DECODE2BYTE_LITTLE(data);
    CM_LOGI("SetMpsCapability set capInfo mps %hu", value->mps);
    return (int)mpsLen;
}

static inline int SetVersionCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t verLen = sizeof(value->version);
    if (len < verLen) {
        return CM_INVALID_RET;
    }
    value->version = DECODE2BYTE_LITTLE(data);
    CM_LOGI("SetVersionCapability set capInfo version 0x%04x", value->version);
    return (int)verLen;
}

static inline int SetRxWindowCapability(const uint8_t *data, uint32_t len,
    CM_CapabilityValue_S *value)
{
    size_t wndLen = sizeof(value->wnd);
    if (len < wndLen) {
        return CM_INVALID_RET;
    }
    value->wnd = *data;
    CM_LOGI("SetRxWindowCapability set capInfo wnd %hhu", value->wnd);
    return (int)wndLen;
}

struct CM_LocalSetCapStru {
    uint32_t bitFlag;
    int (*setCap)(const uint8_t *data, uint32_t len, CM_CapabilityValue_S *value);
};

const struct CM_LocalSetCapStru g_localSetCap[] = {
    {1 << CM_SIGNAL_RELAY_CAPABILITY, SetRelayCapability},
    {1 << CM_SIGNAL_TRANS_MODE, SetTransModeCapability},
    {1 << CM_SIGNAL_MEASURE_CAPABILITY, SetMeasurementCapability},
    {1 << CM_SIGNAL_ACCESS_SLE, SetAccessSleCapability},
    {1 << CM_SIGNAL_ACCESS_SLB, SetAccessSlbCapability},
    {1 << CM_SIGNAL_MTU, SetMtuCapability},
    {1 << CM_SIGNAL_MPS, SetMpsCapability},
    {1 << CM_SIGNAL_VERSION, SetVersionCapability},
    {1 << CM_SIGNAL_WND, SetRxWindowCapability},
};

#define CM_LOCAL_SET_CAPABILITY_NUM (sizeof(g_localSetCap) / sizeof(struct CM_LocalSetCapStru))

static uint32_t SetCapabilityInfo(uint8_t *data,
    uint32_t len, CM_CapabilityValue_S *value)
{
    uint32_t cap = DECODE4BYTE_LITTLE(data);
    CM_LOGI("ack cap %x", cap);
    uint32_t totalLen = (uint32_t)sizeof(uint32_t);

    // CM_ProcessRspSignalingCapability中已判断len至少为4字节
    uint32_t remainingLen = len - (uint32_t)sizeof(uint32_t);
    uint32_t bitMap = 0;
    for (uint8_t i = 0; i < CM_LOCAL_SET_CAPABILITY_NUM; i++) {
        if ((g_localSetCap[i].bitFlag & cap) != 0) {
            int retLen = g_localSetCap[i].setCap(&data[totalLen], remainingLen, value);
            if (retLen == CM_INVALID_RET || remainingLen < (uint32_t)retLen) {
                CM_LOGE("SetCapabilityInfo bitFlag 0x%08x failed", g_localSetCap[i].bitFlag);
                return 0;
            }
            bitMap |= g_localSetCap[i].bitFlag;
            remainingLen -= (uint32_t)retLen;
            totalLen += (uint32_t)retLen;
        }
    }

    if (memcpy_s(&value->capBitMap, sizeof(value->capBitMap), &bitMap, sizeof(bitMap)) != 0) {
        CM_LOGE("memcpy_s capBitMap failed");
        return 0;
    }
    return totalLen;
}

uint32_t CM_ProcessRspSignalingCapability(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    CM_CapabilityValue_S value = {0};
    if (pkt == NULL || pkt->length < sizeof(CM_CapabilityBitmap_S)) {
        CM_LOGE("CM_ProcessRspSignalingCapability params is error");
        return CM_INVALID_PARAM_ERR;
    }

    uint32_t ret = SetCapabilityInfo(pkt->data, pkt->length, &value);
    if (ret == 0) {
        CM_LOGE("CM_ProcessRspSignalingCapability set cap info failed");
        return CM_FAIL;
    }

    if (value.version == 0) {
        CM_SetDeviceLinkDeviceType(lcid, false);
    } else {
        CM_SetLinkProtocolVersion(lcid, value.version);
        CM_SetDeviceLinkDeviceType(lcid, true);
    }

    CM_SetLinkTransMode(lcid, value.transMode);

    if (value.wnd != 0) {
        CM_SetLinkRxWindow(lcid, value.wnd);
    }

    if (value.mtu != 0) {
        CM_SetLinkMtu(lcid, value.mtu);
    }

    CM_LOGI("CM_ProcessRspSignalingCapability AckCap %08x, Tm %hu, Ver %hu, Mtu %hu,"
            " Mps %hu, Wnd %hhu, length %hu, ret %u", value.capBitMap, value.transMode,
            value.version, value.mtu, value.mps, value.wnd, pkt->length, ret);
    return 0;
}

static inline uint32_t GetRelayCapability(uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;
    return 0;
}

static inline uint32_t GetTransModeCapability(uint8_t *data, uint32_t len)
{
    uint32_t transModeLen = (uint32_t)sizeof(g_localInfo.transMode);
    if (len < transModeLen) {
        return 0;
    }
    uint16_t value = *(uint16_t *)&g_localInfo.transMode;
    ENCODE2BYTE_LITTLE(data, value);
    CM_LOGI("local capInfo transMode 0x%04x", value);
    return transModeLen;
}

static inline uint32_t GetMeasurementCapability(uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;
    return 0;
}

static inline uint32_t GetAccessSleCapability(uint8_t *data, uint32_t len)
{
    uint32_t addrLen = SLE_ADDR_LEN * (uint32_t)sizeof(uint8_t);
    SLE_Addr_S *addr = NBC_GetPublicAddress();
    CM_CHECK_RETURN_RET(addr != NULL, 0, "public address is null");
    if (memcpy_s(data, len, addr->addr, addrLen) != 0) {
        CM_LOGE("GetAccessSleCapability memcpy failed");
        return 0;
    }

    CM_LOGI("local capInfo AccessSle %02X:%02X:**:**:**:%02X",
        data[SLE_INDEX_0], data[SLE_INDEX_1], data[SLE_INDEX_5]);
    return addrLen;
}

static inline uint32_t GetAccessSlbCapability(uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;
    return 0;
}

static inline uint32_t GetMtuCapability(uint8_t *data, uint32_t len)
{
    uint32_t mtuLen = (uint32_t)sizeof(g_localInfo.mtu);
    if (len < mtuLen) {
        return 0;
    }
    CM_LOGI("local capInfo mtu %hu", g_localInfo.mtu);
    ENCODE2BYTE_LITTLE(data, g_localInfo.mtu);
    return mtuLen;
}

static inline uint32_t GetMpsCapability(uint8_t *data, uint32_t len)
{
    uint32_t mpsLen = (uint32_t)sizeof(g_localInfo.mps);
    if (len < mpsLen) {
        return 0;
    }
    CM_LOGI("local capInfo mps %hu", g_localInfo.mps);
    ENCODE2BYTE_LITTLE(data, g_localInfo.mps);
    return mpsLen;
}

static inline uint32_t GetVersionCapability(uint8_t *data, uint32_t len)
{
    uint32_t verLen = (uint32_t)sizeof(g_localInfo.defaultVeraion);
    if (len < verLen) {
        return 0;
    }
    CM_LOGI("local capInfo version 0x%04x", g_localInfo.defaultVeraion);
    ENCODE2BYTE_LITTLE(data, g_localInfo.defaultVeraion);
    return verLen;
}

static inline uint32_t GetRxWindowCapability(uint8_t *data, uint32_t len)
{
    uint32_t wndLen = (uint32_t)sizeof(g_localInfo.wnd);
    if (len < wndLen) {
        return 0;
    }
    CM_LOGI("local capInfo wnd %hhu", g_localInfo.wnd);
    *data = g_localInfo.wnd;
    return wndLen;
}

struct CM_LocalGetCapStru {
    uint32_t bitFlag;
    uint32_t (*getCap)(uint8_t *data, uint32_t len);
};

const struct CM_LocalGetCapStru g_localGetCap[] = {
    {1 << CM_SIGNAL_RELAY_CAPABILITY, GetRelayCapability},
    {1 << CM_SIGNAL_TRANS_MODE, GetTransModeCapability},
    {1 << CM_SIGNAL_MEASURE_CAPABILITY, GetMeasurementCapability},
    {1 << CM_SIGNAL_ACCESS_SLE, GetAccessSleCapability},
    {1 << CM_SIGNAL_ACCESS_SLB, GetAccessSlbCapability},
    {1 << CM_SIGNAL_MTU, GetMtuCapability},
    {1 << CM_SIGNAL_MPS, GetMpsCapability},
    {1 << CM_SIGNAL_VERSION, GetVersionCapability},
    {1 << CM_SIGNAL_WND, GetRxWindowCapability},
};

#define CM_LOCAL_CAPABILITY_NUM (sizeof(g_localGetCap) / sizeof(struct CM_LocalGetCapStru))

static uint32_t GetCapabilityInfo(uint32_t cap, uint8_t *data, uint32_t len)
{
    uint32_t rspCap = cap & g_localInfo.cap;
    ENCODE4BYTE_LITTLE(data, rspCap);
    CM_LOGI("GetCapabilityInfo cap 0x%08x, rsp Cap 0x%08x", cap, rspCap);
    uint32_t totalLen = (uint32_t)sizeof(rspCap);
    uint32_t retLen = (uint32_t)sizeof(rspCap);
    uint32_t remainingLen = len - (uint32_t)sizeof(rspCap);
    for (uint8_t i = 0; i < CM_LOCAL_CAPABILITY_NUM; i++) {
        if ((g_localGetCap[i].bitFlag & rspCap) != 0) {
            retLen = g_localGetCap[i].getCap(&data[totalLen], remainingLen);
            if (remainingLen < retLen || retLen == 0) {
                CM_LOGE("GetCapabilityInfo bitFlag 0x%08x failed", g_localGetCap[i].bitFlag);
                return 0;
            }
            remainingLen -= retLen;
            totalLen += retLen;
        }
    }

    return totalLen;
}

uint32_t CM_ProcessReqSignalingCapability(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    if (pkt == NULL || pkt->length < sizeof(uint32_t)) {
        CM_LOGE("CM_ProcessReqSignalingCapability params is error");
        return CM_INVALID_PARAM_ERR;
    }

    uint32_t cap = DECODE4BYTE_LITTLE((uint8_t *)pkt->data);
    if (((1 << CM_SIGNAL_VERSION) & cap) != 0) {
        /* 请求的version置位，确认新设备 */
        CM_SetDeviceLinkDeviceType(lcid, true);
    }

    // 后续可能有其他能力值及其capInfos，需要检查tmp长度是否足够
    uint8_t tmp[CM_CAPABILITY_RSP_LEN] = {0};
    uint32_t len = GetCapabilityInfo(cap, tmp, (uint32_t)sizeof(tmp));
    if (len == 0) {
        CM_LOGE("GetCapabilityInfo failed");
        return CM_FAIL;
    }

    SDF_Buff_S *buf = CM_CreateSignalingBuff(CAPABILITY_RSP, pkt->identifier,
        (uint8_t*)tmp, (uint16_t)len);
    CM_CHECK_RETURN_RET(buf != NULL, CM_MEM_ERR, "CM_ProcessReqSignalingCapability create buf failed");
    uint32_t ret = CM_SendBuffToDtap(lcid, buf);
    if (ret != 0) {
        SDF_BuffFree(buf);
    }
    CM_LOGI("CM_ProcessReqSignalingCapability cap %08x capRsp %08x, size %hu ret %u", cap,
        *(uint32_t *)(pkt->data), len, ret);
    return ret;
}

uint32_t CM_SendReqSignalingCapability(uint16_t lcid, CM_CapabilityBitmap_S *cap)
{
    uint32_t capBitMap = 0;
    ENCODE4BYTE_LITTLE((uint8_t *)&capBitMap, *(uint32_t *)cap);
    SDF_Buff_S *buf = CM_CreateSignalingBuff(CAPABILITY_REQ, CM_GetIdentifier(),
        (uint8_t*)&capBitMap, sizeof(uint32_t));
    CM_CHECK_RETURN_RET(buf != NULL, CM_MEM_ERR, "CM_SendReqSignalingCapability create buf failed");
    uint32_t ret = CM_SendBuffToDtap(lcid, buf);
    if (ret != 0) {
        SDF_BuffFree(buf);
    }
    return ret;
}