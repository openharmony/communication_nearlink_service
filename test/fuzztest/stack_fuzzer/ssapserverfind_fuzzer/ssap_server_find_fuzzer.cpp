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
#include "sdf_mem.h"
#include "cm_def.h"

#include "ssap_server_find_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssap_utils.h"

extern "C" void CM_StubDeviceType(uint8_t type);

// STD 形态 UUID：前 14 字节为标准基（SSAP_CheckUuidStd 仅比对前 14 字节，ssap_utils.c:27/:101-110），尾 2 字节区分条目
// SVC_STD 服务（句柄域 [0x10,0x12]）
static NLSTK_SsapUuid_S g_stdSvcUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02};
// SVC_STD 属性1
static NLSTK_SsapUuid_S g_stdPropUuid1 = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x04};
// SVC_STD 属性2
static NLSTK_SsapUuid_S g_stdPropUuid2 = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x05, 0x06};
// SVC_MIX 服务（STD 形态，句柄域 [0x15,0x17]）
static NLSTK_SsapUuid_S g_mixSvcUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x08};
// SVC_MIX 的 STD 形态属性
static NLSTK_SsapUuid_S g_mixPropStdUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x09, 0x0A};
// SVC_MIX 的 CUS 属性（非 STD）
static NLSTK_SsapUuid_S g_uuid2 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10};
// SVC_CUS 服务（沿用，非 STD，句柄域 [0x13,0x14]）
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11};
// SVC_CUS 属性（沿用，非 STD）
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e};

// SVC_STD：服务与属性均为 STD 形态 UUID（响应中按 2 字节短 UUID 编码，显著短于 CUS）
static void AddService1()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_stdSvcUuid, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_stdPropUuid1, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0x00] = 0xEE;
    descParam->permission.permissionValue = 0x04;
    descParam->operation.operationValue = 0x07;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_stdPropUuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0x00] = 0xAA;
    propertyParam1->operation.operationValue = 0x73F;
    propertyParam1->permission.permissionValue = 0x04;
    propertyParam1->operation.operationValue = 0x19;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);
    SSAP_ParamAddDescriptor_S *descParam1 =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 0x02);
    descParam1->type = DESC_TYPE_CLIENT_CONFIG;
    descParam1->val.len = 0x02;
    descParam1->val.value[0] = 0x01;
    descParam1->val.value[1] = 0x00;
    descParam1->operation.operationValue = 0x73F;
    SSAP_CacheDescriptor(descParam1);
    SDF_MemFree(descParam1);
    SSAP_StartService(NULL);
}

// SVC_CUS：服务与属性均为非 STD UUID（响应中按 16 字节 UUID 编码）
static void AddService2()
{
    SSAP_ParamAddService_S *serviceParam1 = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam1->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam1);
    SDF_MemFree(serviceParam1);
    SSAP_ParamAddProperty_S *propertyParam2 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 0x02);
    memcpy_s(&propertyParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    propertyParam2->val.len = 0x02;
    propertyParam2->val.value[0] = 0xAB;
    propertyParam2->val.value[1] = 0xCD;
    propertyParam2->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam2);
    SDF_MemFree(propertyParam2);
    SSAP_StartService(NULL);
}

// SVC_MIX：服务 uuid 为 STD 形态，属性一半 STD 一半 CUS（请求 itemType=MIX 时命中 FillMix* 混排组包路径）
static void AddService3()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_mixSvcUuid, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_mixPropStdUuid, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xCC;
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0x00] = 0xDD;
    propertyParam1->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);
    SSAP_StartService(NULL);
}

namespace OHOS {

    // 最小观测记录（不做断言）：响应到达与长度差异留作外部核对——STD 条目响应按 2 字节 UUID 编码，显著短于 CUS 的 16 字节
    static uint8_t g_lastOutOpcode = 0;
    static uint32_t g_lastOutLen = 0;
    static uint32_t g_outPkts = 0;

    void SendFunc(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        g_lastOutOpcode = opcode;
        g_lastOutLen = (buff != NULL) ? (uint32_t)SDF_DataLenGet(buff) : 0;
        g_outPkts++;
    }

    void FuzzDliSsapRecvPkt(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);

        SSAP_LinkInit();
        SSAPS_ServiceInit();
        AddService1();
        AddService2();
        AddService3();

        SLE_Addr_S sleAddr = {0};
        sleAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            sleAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        (void)SSAP_CreateSsapLink(&sleAddr, 0, SendFunc);

        if (fdp.ConsumeBool()) {
            CM_StubDeviceType(CM_DEVTYPE_NEW);
        } else {
            CM_StubDeviceType(CM_DEVTYPE_OLD);
        }

        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size + 1);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size + 1);
        uint8_t reqOpcode = fdp.ConsumeBool() ? SSAP_FIND_STRUCTURE_REQ : SSAP_FIND_STRUCTURE_BY_UUID_REQ;
        // ——以下为新增消费：追加在既有序列（type→addr→devType→reqOpcode）之后、payload 循环之前。
        //   FDP 整数自尾部取字节，payload 循环会吞掉剩余全部字节，新字段若排在其后只能取到空 FDP 默认值 0——
        bool useStructPdu = fdp.ConsumeBool();                     // 结构化 PDU 构造/原始报文路径选择
        uint8_t findType = fdp.ConsumeIntegral<uint8_t>() % 6;     // SS-C1：全 6 种 findType（0x02/0x05 拒绝分支显式驱动）
        uint8_t itemType = fdp.ConsumeIntegral<uint8_t>() % 3;     // SS-C2：STD/CUS/MIX（MIX 命中 FillMix* 组包）
        uint8_t rangeSel = fdp.ConsumeIntegral<uint8_t>() % 3;     // 句柄域三态：空范围/恰好单服务域/交叉跨服务
        uint8_t svcIdx = fdp.ConsumeIntegral<uint8_t>() % 3;       // 目标服务（by-uuid 的 uuid 与边界句柄域）
        uint8_t uuidShort = fdp.ConsumeIntegral<uint8_t>() % 2;    // by-uuid 变体：2 字节/16 字节 uuid
        if (!useStructPdu) {
            // 原始路径：保留 opcode 二选一 + 逐字节随机载荷
            buf[0] = reqOpcode;
            for (size_t i = 0; i < size; i++) {
                buf[i + 1] = fdp.ConsumeIntegral<uint8_t>();
            }
        } else {
            // 结构化路径：组装合法 PDU 放入同一 buff，仍经 SSAP_Recv 入口进入 SSAPS_FindReqHandle。
            // 句柄域推导（遗留路径：每输入 ServiceDeInit 复位 g_curHandle=0x10，ssaps_service.c:38/:45/:387；
            // SSAPS_StartService 按每服务 1 句柄+每属性 1 句柄连续分配，描述符不占句柄，ssaps_service.c:281-328。
            // 布局确定；以实施 SHA 为准）：
            //   SVC_STD 域 [0x10,0x12]（svc=0x10，属性=0x11/0x12）
            //   SVC_CUS 域 [0x13,0x14]（svc=0x13，属性=0x14）
            //   SVC_MIX 域 [0x15,0x17]（svc=0x15，STD 属性=0x16，CUS 属性=0x17）
            static const uint16_t svcStart[3] = {0x10, 0x13, 0x15};
            static const uint16_t svcEnd[3] = {0x12, 0x14, 0x17};
            uint16_t startH;
            uint16_t endH;
            if (rangeSel == 0) {
                startH = 0x14;                      // 空范围：start>end → INVALID_HANDLE（start==0 由原始路径随机覆盖）
                endH = 0x13;
            } else if (rangeSel == 1) {
                startH = svcStart[svcIdx];          // 边界：恰好选中服务的句柄域
                endH = svcEnd[svcIdx];
            } else {
                startH = 0x12;                      // 交叉：跨三服务（STD 属性2/CUS 服务+属性/MIX 服务）
                endH = 0x15;
            }
            uint32_t uuidLen = 0;                   // FIND_STRUCTURE_REQ 无 uuid；BY_UUID_REQ 追加 2/16 字节
            if (reqOpcode == SSAP_FIND_STRUCTURE_BY_UUID_REQ) {
                uuidLen = uuidShort ? SSAP_UUID128_LEN : SSAP_UUID16_LEN;
            }
            uint32_t pduLen = sizeof(SSAP_PduFindStructReq_S) + uuidLen;
            /* 容量不足（小输入 × by-uuid 16 字节）时结构化填充会越界写
             * （ASAN 设备轮实锤：req->uuid[i] heap-buffer-overflow）——
             * 退化为原始路径（opcode+随机载荷），只写 size+1 已分配区 */
            bool structFit = (size + 1 >= pduLen);
            if (!structFit) {
                buf[0] = reqOpcode;
                for (size_t i = 0; i < size; i++) {
                    buf[i + 1] = fdp.ConsumeIntegral<uint8_t>();
                }
            } else {
                (void)SDF_BuffTrimSuffix(sdfBuff, size + 1 - pduLen);  // 收缩 dataLen 到合法 PDU 长度
                SSAP_PduFindStructReq_S *req = (SSAP_PduFindStructReq_S *)buf;
                req->msgCode = reqOpcode;
                req->ctrl.findType = findType;
                req->ctrl.itemType = itemType;
                req->ctrl.rspMode = FIND_RSP_MODE_SINGLE_RSP;  // 多响应模式需对端支持分包（fragCtx.fragment 恒 0），恒取单响应
                req->ctrl.resv = 0;
                req->startHandle = startH;
                req->endHandle = endH;
                if (uuidLen != 0) {
                    static NLSTK_SsapUuid_S *svcUuid[3] = {&g_stdSvcUuid, &g_uuid3, &g_mixSvcUuid};
                    NLSTK_SsapUuid_S *uuid = svcUuid[svcIdx];
                    if (uuidLen == SSAP_UUID128_LEN) {
                        for (uint32_t i = 0; i < SSAP_UUID128_LEN; i++) {
                            req->uuid[i] = uuid->uuid[SSAP_UUID128_LEN - 1 - i];  // 16 字节全量网络序（对应 SSAP_HostToNetUuid）
                        }
                    } else if (SSAP_CheckUuidStd(uuid)) {
                        req->uuid[0] = uuid->uuid[15];  // STD 形态短 uuid：网络序（SSAP_GetUuidFromPktBuf 回填 uuid[14]/[15]）
                        req->uuid[1] = uuid->uuid[14];
                    } else {
                        req->uuid[0] = 0xAA;            // 非 STD 形态无法以 2 字节表达：发不匹配短 uuid，驱动 by-uuid ITEM_INEXIST
                        req->uuid[1] = 0xBB;
                    }
                }
                }  // structFit 路径结束：以上结构化填充仅在容量足够时执行
        }

        DTAP_Data_Info_S info = {0};
        SSAP_Recv(&info, sdfBuff);

        SDF_BuffFree(sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);

        SSAP_LinkDeInit();
        SSAPS_ServiceDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzDliSsapRecvPkt(fuzzData, fuzzSize);
    return 0;
}