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
#include "gtest/gtest.h"
#include "securec.h"

#include "ssap_link.h"
#include "ssap_type.h"
#include "ssapc_client.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_addr.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "dtap.h"

#include "sdf_string.h"
#include "cpfwk_log.h"

#define TEST_MAX_BUF_CACHE 1024

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};
static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static NLSTK_SsapUuid_S g_uuidService = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuidProperty = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};

static uint8_t reqPktUuidService[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
static uint8_t reqPktUuidProperty[] = {0x06, 0x03, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};

// find回复,op-1,ctrl标准模式-1,{开始句柄-2,结束句柄-2,member-1}
static uint8_t rspPktUuidServiceStandard[] = {0x07, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02};
static uint8_t rspPktUuidServiceStandardErrLen[] = {0x07, 0x03, 0x10, 0x00, 0x11, 0x00};
static uint8_t rspPktUuidServiceCustomize[] = {0x07, 0x07, 0x10, 0x00, 0x11, 0x00, 0x02};
static uint8_t rspPktUuidServiceMix[] = {0x07, 0x0B, 0x01, 0x10, 0x00, 0x11, 0x00, 0x02};
static uint8_t rspPktUuidServiceMixErrCount[] = {0x07, 0x0B, 0x00};
static uint8_t rspPktUuidServiceMixErrSize[] = {0x07, 0x0B, 0x01};
static uint8_t rspPktUuidServiceMixErrLen[] = {0x07, 0x0B};

// find回复,op-1,ctrl标准模式-1,{开始句柄-2,operation-4,count-1,descriptor-x}
static uint8_t rspPktUuidPropertyStandard1[] = {0x07, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t rspPktUuidPropertyStandard2[] = {0x07, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
static uint8_t rspPktUuidPropertyStandard3[] = {0x07, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}; 
static uint8_t rspPktUuidPropertyStandardErrLen[] = {0x07, 0x03};
static uint8_t rspPktUuidPropertyCustomize[] = {0x07, 0x07};
static uint8_t rspPktUuidPropertyMix1[] = {0x07, 0x0B};
static uint8_t rspPktUuidPropertyMix2[] = {0x07, 0x0B, 0x00};
static uint8_t rspPktUuidPropertyMix3[] = {0x07, 0x0B, 0x01};
static uint8_t rspPktUuidPropertyMix4[] = {0x07, 0x0B, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
static uint8_t rspPktUuidPropertyMix5[] = {0x07, 0x0B, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
static uint8_t rspPktUuidPropertyMixErrLen[] = {0x07, 0x0B};
static uint8_t g_recvCbk = -1;
static SSAP_PduErrCode g_errCode = SSAP_ERRCODE_MAX;

class SSAP_CLIENT_HISIOLD_TEST : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
    }
};

static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
}

static SSAP_Link_S *CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

static void StubFindServiceReq()
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&findParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuidService, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPktUuidService));
    EXPECT_EQ(memcmp(g_buffCache, reqPktUuidService, g_buffLen), 0);
}

static void StubFindPropertyReq()
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PROPERTY;
    (void)memcpy_s(&findParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuidProperty, sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPktUuidProperty));
    EXPECT_EQ(memcmp(g_buffCache, reqPktUuidProperty, g_buffLen), 0);
}

// 发现服务，设备为hisiold，仅标准条目
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_001)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceStandard));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceStandard));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceStandard), rspPktUuidServiceStandard,
                    sizeof(rspPktUuidServiceStandard));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，仅标准条目，ErrLen
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_002)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceStandardErrLen));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceStandardErrLen));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceStandardErrLen), rspPktUuidServiceStandardErrLen,
        sizeof(rspPktUuidServiceStandardErrLen));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，混合条目，ErrLen
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_003)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceMixErrLen));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceMixErrLen));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceMixErrLen), rspPktUuidServiceMixErrLen,
                    sizeof(rspPktUuidServiceMixErrLen));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，混合条目，ErrCount
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_004)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceMixErrCount));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceMixErrCount));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceMixErrCount), rspPktUuidServiceMixErrCount,
                    sizeof(rspPktUuidServiceMixErrCount));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，混合条目，ErrSize
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_005)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceMixErrSize));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceMixErrSize));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceMixErrSize), rspPktUuidServiceMixErrSize,
                    sizeof(rspPktUuidServiceMixErrSize));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，混合条目
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_006)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceMix));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceMix));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceMix), rspPktUuidServiceMix,
                    sizeof(rspPktUuidServiceMix));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现服务，设备为hisiold，自定义条目
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_007)
{
    StubFindServiceReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidServiceCustomize));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidServiceCustomize));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidServiceCustomize), rspPktUuidServiceCustomize,
                    sizeof(rspPktUuidServiceCustomize));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，标准条目，无属性描述符
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_008)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyStandard1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyStandard1));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyStandard1), rspPktUuidPropertyStandard1,
                    sizeof(rspPktUuidPropertyStandard1));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，标准条目，有一个属性描述符
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_009)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyStandard2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyStandard2));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyStandard2), rspPktUuidPropertyStandard2,
                    sizeof(rspPktUuidPropertyStandard2));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，标准条目，属性描述符错误
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_010)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyStandard3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyStandard3));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyStandard3), rspPktUuidPropertyStandard3,
                    sizeof(rspPktUuidPropertyStandard3));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，标准条目，报文长度错误
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_011)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyStandardErrLen));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyStandardErrLen));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyStandardErrLen), rspPktUuidPropertyStandardErrLen,
                    sizeof(rspPktUuidPropertyStandardErrLen));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，自定义条目，报文长度错误
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_012)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyCustomize));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyCustomize));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyCustomize), rspPktUuidPropertyCustomize,
                    sizeof(rspPktUuidPropertyCustomize));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，混合条，leftSize为0
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_013)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyMixErrLen));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyMixErrLen));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyMixErrLen), rspPktUuidPropertyMixErrLen,
        sizeof(rspPktUuidPropertyMixErrLen));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，混合条目，leftCount为0
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_014)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyMix2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyMix2));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyMix2), rspPktUuidPropertyMix2, sizeof(rspPktUuidPropertyMix2));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，混合条目，leftSize小于所需长度
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_015)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyMix3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyMix3));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyMix3), rspPktUuidPropertyMix3, sizeof(rspPktUuidPropertyMix3));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，混合条目，leftSize 小于 needSize
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_016)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyMix4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyMix4));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyMix4), rspPktUuidPropertyMix4, sizeof(rspPktUuidPropertyMix4));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

// 发现属性，设备为hisiold，混合条目
TEST_F(SSAP_CLIENT_HISIOLD_TEST, CLIENT_FIND_TEST_017)
{
    StubFindPropertyReq();

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktUuidPropertyMix5));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktUuidPropertyMix5));
    (void)memcpy_s(tmpBuf, sizeof(rspPktUuidPropertyMix5), rspPktUuidPropertyMix5, sizeof(rspPktUuidPropertyMix5));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}