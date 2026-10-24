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
#include "sdf_addr.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "sdf_vector.h"
#include "nlstk_log.h"
#include "devd_local.h"
#include "nlstk_schedule.h"
#include "nlstk_init_api.h"
#include "nlstk_devd.h"
#include "nlstk_devd_api.h"
#include "nlstk_scan_api.h"
#include "devd_scan_manager.h"
#include "devd_scan_stm.h"
#include "devd_scan_util.h"
#include "devd_scan_api.h"
#include "devd_scan_type.h"
#include "devd_scan_filter.h"
#include "devd_report_parse.h"
#include "devd_cbk.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "multiscan_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "SleDliThreadUtil.h"

#define STACK_MAX_THREAD 5
#define SERVICE_UUID_LEN_128 16
#define MAX_ADV_DATA_LEN 250

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN   = 7,
    FUZZ_THIRTYTWO = 32,
} FUZZ_NUM_E;

static inline void FuzzGetVariableData(FuzzedDataProvider &fdp, NLSTK_VariableData_S *varData, uint8_t maxLen)
{
    uint8_t len = fdp.ConsumeIntegral<uint8_t>() % (maxLen + 1);
    varData->len = len;
    if (len > 0) {
        varData->data = (uint8_t *)SDF_MemZalloc(len);
        if (varData->data != NULL) {
            for (uint8_t i = 0; i < len; i++) {
                varData->data[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }
    } else {
        varData->data = NULL;
    }
}

static inline void FuzzFreeVariableData(NLSTK_VariableData_S *varData)
{
    if (varData->data != NULL) {
        SDF_MemFree(varData->data);
        varData->data = NULL;
    }
    varData->len = 0;
}

typedef struct {
    uint8_t *data;
    size_t size;
} FuzzAdvReportArg_S;

static void FreeAdvReportArg(void *arg)
{
    FuzzAdvReportArg_S *reportArg = (FuzzAdvReportArg_S *)arg;
    if (reportArg != NULL) {
        SDF_MemFree(reportArg->data);
        SDF_MemFree(reportArg);
    }
}

static uint32_t SdfInit(void)
{
    uint32_t ret = SDF_ThreadInit(STACK_MAX_THREAD);
    if (ret != NLSTK_OK) {
        return ret;
    }
    ret = SDF_EvcInit();
    if (ret != NLSTK_OK) {
        SDF_ThreadDeinit();
        return ret;
    }
    return NLSTK_OK;
}

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();
    static uint8_t g_advHandle = 0;

    static void DevdAdvEventCbkStub(NLSTK_DevdAdvCbkParam_S *param)
    {
        (void)param;
    }

    /* ponytail: 不再用 stub 经 NLSTK_DevdRegScanEventCbk 抢注 SCAN_*_CBK 全局单点：
     * 那会覆盖 DevdScanManagerInit 注册的真实 ScanCbk/ReportCbk（devd_scan_manager.c:112-115），
     * 截断 STM 回调链使 STARTED 不可达。用户层观察改走 NLSTK_DevdRegScanModule 的模块级回调。 */

    static void FuzzNlstkDevdSetAdvData(uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        NLSTK_DevdSetAdvData_S setAdvData = {0};
        setAdvData.advHandle = fdp.ConsumeIntegral<uint8_t>();

        uint16_t advDataLen = fdp.ConsumeIntegral<uint16_t>() % (MAX_ADV_DATA_LEN + 1);
        setAdvData.data.advDataLen = advDataLen;
        if (advDataLen > 0) {
            setAdvData.data.advData = (uint8_t *)SDF_MemZalloc(advDataLen);
            if (setAdvData.data.advData == NULL) {
                return;
            }
            for (uint16_t i = 0; i < advDataLen; i++) {
                setAdvData.data.advData[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        uint16_t scanRspDataLen = fdp.ConsumeIntegral<uint16_t>() % (MAX_ADV_DATA_LEN + 1);
        setAdvData.data.scanRspDataLen = scanRspDataLen;
        if (scanRspDataLen > 0) {
            setAdvData.data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
            if (setAdvData.data.scanRspData == NULL) {
                SDF_MemFree(setAdvData.data.advData);
                return;
            }
            for (uint16_t i = 0; i < scanRspDataLen; i++) {
                setAdvData.data.scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        (void)NLSTK_DevdSetAdvData(&setAdvData);

        SDF_MemFree(setAdvData.data.advData);
        SDF_MemFree(setAdvData.data.scanRspData);
    }

    static void FuzzNlstkDevdStartAdv(uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        NLSTK_DevdSetAdvParams_S advParams = {0};
        advParams.accessMode = fdp.ConsumeIntegral<uint8_t>() % (ADV_ACCESS_MODE_DEFAULT + 1);
        advParams.discoveryLevel = fdp.ConsumeIntegral<uint8_t>() % (ADV_DISCOVERY_LEVEL_SPECIAL + 1);
        advParams.param.advHandle = fdp.ConsumeIntegral<uint8_t>();
        advParams.param.advMode = fdp.ConsumeIntegral<uint8_t>() % (ADV_MODE_CONNECTABLE_DIRECTED + 1);
        if (advParams.param.advMode == ADV_MODE_CONNECTABLE_DIRECTED) {
            advParams.param.advMode = ADV_MODE_CONNECTABLE_SCANABLE;
        }
        advParams.param.advGtRole = fdp.ConsumeIntegral<uint8_t>() % (ADV_GT_ROLE_G_NO_NEGO + 1);
        advParams.param.advIntervalMin = fdp.ConsumeIntegral<uint32_t>();
        if (advParams.param.advIntervalMin < DEVD_ADV_INTERVAL_MIN) {
            advParams.param.advIntervalMin = DEVD_ADV_INTERVAL_MIN;
        }
        advParams.param.advIntervalMax = fdp.ConsumeIntegral<uint32_t>();
        if (advParams.param.advIntervalMax < advParams.param.advIntervalMin) {
            advParams.param.advIntervalMax = advParams.param.advIntervalMin;
        }
        if (advParams.param.advIntervalMax > DEVD_ADV_INTERVAL_MAX) {
            advParams.param.advIntervalMax = DEVD_ADV_INTERVAL_MAX;
        }
        advParams.param.advChannelMap = fdp.ConsumeIntegral<uint8_t>() % (ADV_CHANNEL_MAP_DEFAULT + 1);
        advParams.param.ownAddr.type = fdp.ConsumeIntegral<uint8_t>() % ADDR_TYPE_END;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParams.param.ownAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        advParams.param.peerAddr.type = fdp.ConsumeIntegral<uint8_t>() % ADDR_TYPE_END;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParams.param.peerAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        advParams.param.primaryFrameType = fdp.ConsumeIntegral<uint8_t>() % (ADV_FRAME_TYPE_4 + 1);
        advParams.param.extParam = NULL;
        advParams.param.connParam = NULL;

        uint16_t advDataLen = fdp.ConsumeIntegral<uint16_t>() % (MAX_ADV_DATA_LEN + 1);
        advParams.data.advDataLen = advDataLen;
        if (advDataLen > 0) {
            advParams.data.advData = (uint8_t *)SDF_MemZalloc(advDataLen);
            if (advParams.data.advData == NULL) {
                return;
            }
            for (uint16_t i = 0; i < advDataLen; i++) {
                advParams.data.advData[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }
        uint16_t scanRspDataLen = fdp.ConsumeIntegral<uint16_t>() % (MAX_ADV_DATA_LEN + 1);
        advParams.data.scanRspDataLen = scanRspDataLen;
        if (scanRspDataLen > 0) {
            advParams.data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
            if (advParams.data.scanRspData == NULL) {
                SDF_MemFree(advParams.data.advData);
                return;
            }
            for (uint16_t i = 0; i < scanRspDataLen; i++) {
                advParams.data.scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        (void)NLSTK_DevdStartAdv(&advParams);

        SDF_MemFree(advParams.data.advData);
        SDF_MemFree(advParams.data.scanRspData);
    }

    static void FuzzNlstkDevdEnableAdv(uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        NLSTK_DevdSetAdvEnable_S setEnable = {0};
        setEnable.advHandle = g_advHandle;
        setEnable.enable = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
        setEnable.duration = fdp.ConsumeIntegral<uint16_t>();
        setEnable.maxAdvEvent = fdp.ConsumeIntegral<uint8_t>();
        (void)NLSTK_DevdEnableAdv(&setEnable);
    }

    static void FuzzNlstkDevdRemoveAdv(void)
    {
        uint8_t removeHandle = g_advHandle;
        (void)NLSTK_DevdRemoveAdv(&removeHandle);
    }

    static void FuzzNlstkDevdCreateAdvHandle(void)
    {
        uint8_t handle = NLSTK_DevdCreateAdvHandle(DevdAdvEventCbkStub);
        g_advHandle = handle;
    }

    static uint8_t FuzzGetPhyCountByFrameType(uint8_t frameType)
    {
        uint8_t phyCount = 0;
        if ((frameType & 0x01) != 0) { // FRAME_TYPE_1
            phyCount++;
        }
        if ((frameType & 0x02) != 0) { // FRAME_TYPE_4
            phyCount++;
        }
        return phyCount;
    }

    static void FuzzNlstkDevdSleStartScan(uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint8_t frameType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR;
        uint8_t phyCount = FuzzGetPhyCountByFrameType(frameType);
        size_t totalSize = sizeof(NLSTK_DevdSleScanParams_S) + phyCount * sizeof(NLSTK_DevdSleScanParamsNoPhy_S);
        NLSTK_DevdSleScanParams_S *sleScanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(totalSize);
        if (sleScanParams == NULL) {
            return;
        }

        sleScanParams->localAddrType = fdp.ConsumeIntegral<uint8_t>() % ADDR_TYPE_END;
        sleScanParams->scanFilterPolicy = fdp.ConsumeIntegral<uint8_t>() % (SCAN_FLT_EXTEND + 1);
        sleScanParams->frameType = frameType;
        for (uint8_t i = 0; i < phyCount; i++) {
            sleScanParams->params[i].scanType = fdp.ConsumeIntegral<uint8_t>() % (SCAN_TYPE_ACTIVE + 1);
            uint16_t scanInterval = fdp.ConsumeIntegral<uint16_t>();
            sleScanParams->params[i].scanInterval =
                (scanInterval < SCAN_INTERVAL_MIN) ? SCAN_INTERVAL_MIN : scanInterval;
            uint16_t scanWindow = fdp.ConsumeIntegral<uint16_t>();
            sleScanParams->params[i].scanWindow = (scanWindow < SCAN_WINDOW_MIN) ? SCAN_WINDOW_MIN : scanWindow;
        }

        (void)NLSTK_DevdSleStartScan(sleScanParams);
        SDF_MemFree(sleScanParams);
    }

    static void FuzzNlstkDevdSleEnableScan(uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        NLSTK_DevdSleScanEnable_S sleScanEnable = {0};
        sleScanEnable.scanEnable = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
        sleScanEnable.scanFilterDuplicates = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
        (void)NLSTK_DevdSleEnableScan(&sleScanEnable);
    }

    void FuzzDevdApi(uint8_t *data, size_t size)
    {
        if (data == NULL || size < 1) {
            return;
        }
        g_advHandle = 0;

        FuzzNlstkDevdCreateAdvHandle();

        FuzzNlstkDevdSetAdvData(data, size);

        FuzzNlstkDevdStartAdv(data, size);

        FuzzNlstkDevdEnableAdv(data, size);

        FuzzNlstkDevdRemoveAdv();

        FuzzNlstkDevdCreateAdvHandle();

        FuzzNlstkDevdSleStartScan(data, size);

        FuzzNlstkDevdSleEnableScan(data, size);
    }

    struct ScanFuzzState {
        bool moduleRegistered;
        uint8_t moduleId;
        bool scannerAllocated;
        uint32_t scannerId;
    };

    void ResetScanState(ScanFuzzState* state)
    {
        state->moduleRegistered = false;
        state->moduleId = 0;
        state->scannerAllocated = false;
        state->scannerId = 0;
    }

    void FuzzDevdScanInit(void *arg)
    {
        (void)arg;
        DevdScanInit();
    }

    void FuzzDevdScanDeInit(void *arg)
    {
        (void)arg;
        DevdScanDeInit();
    }

    void FuzzDevdAdvDataReport(void *arg)
    {
        FuzzAdvReportArg_S *reportArg = (FuzzAdvReportArg_S *)arg;
        if (reportArg == NULL || reportArg->data == NULL || reportArg->size == 0) {
            return;
        }
        FuzzedDataProvider fdp(reportArg->data, reportArg->size);
        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t *advDataBuf = (uint8_t *)SDF_MemZalloc(advDataLen);
        if (advDataBuf == NULL) {
            return;
        }
        for (uint8_t i = 0; i < advDataLen; i++) {
            advDataBuf[i] = fdp.ConsumeIntegral<uint8_t>();
        }        NLSTK_DevdScanFilter_S *filters = (NLSTK_DevdScanFilter_S *)
            SDF_MemZalloc(sizeof(NLSTK_DevdScanFilter_S) * 0x05);
        uint16_t filtersNum = 0x05;
        filters[0].hasServiceUuid = true;
        filters[1].hasSolicitationUuid = true;
        filters[2].hasRssiThreshold = true;
        filters[3].meshInfoReport = true;
        filters[4].isNoFilter = true;
        SDF_Vector_S *filtersVec = DevdConvertFiltersToVector(filters, filtersNum);

        NLSTK_DevdAdvReportInfo_S *report = (NLSTK_DevdAdvReportInfo_S *)
            SDF_MemZalloc(sizeof(NLSTK_DevdAdvReportInfo_S) + advDataLen);
        if (report == NULL) {
            SDF_MemFree(advDataBuf);
            SDF_DestroyVector(filtersVec);
            return;
        }
        report->addrType = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            report->addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        /* S8: extendParams 11 字段全部喂实 */
        report->extendParams.eventType = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.dataStatus = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.directAddrType = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            report->extendParams.directAddr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        report->extendParams.primFrameType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
        report->extendParams.primPhy = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.secondPhy = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.secondFrameType = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.secondMod = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.secondPilotRatio = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.secondPolar = fdp.ConsumeIntegral<uint8_t>();
        report->rssi = fdp.ConsumeIntegral<uint8_t>();
        report->dataLength = advDataLen;
        (void)memcpy_s(report->data, report->dataLength, advDataBuf, report->dataLength);
        NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
        if (result != NULL) {
            DevdIsMatchFilters(result, filtersVec);
            FreeAdvResult(result);
        }

        for (uint8_t i = 0; i < advDataLen; i++) {
            report->data[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        result = DevdAdvDataReport(report);
        if (result != NULL) {
            DevdIsMatchFilters(result, filtersVec);
            FreeAdvResult(result);
        }

        SDF_DestroyVector(filtersVec);
        SDF_MemFree(report);
        SDF_MemFree(advDataBuf);
    }

    /* ========== PR-5 C1: 扫描生命周期闭环（直调 DLI 回调补全链路） ========== */
    /* S3 DIS 名称夹具：服务数据 uuid 末两字节 06 09 + 本地名称 ABCD */
    static const uint8_t g_fuzzDisNameAdv[] = {0x03, 0x08, 0x09, 0x06, 0x06, 0x04, 0x41, 0x42, 0x43, 0x44};
    /* S1 旧格式 LTV 夹具：铅笔签名，触发 DevdParseAdvReportOld 路径 */
    static const uint8_t g_fuzzOldLtvAdv[] = {0x03, 0x19, 0xC7, 0x03, 0x0F, 0x16, 0xEE, 0xFD};
    static const uint8_t g_fuzzAddrA[SLE_ADDR_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    static uint32_t g_moduleStartCnt = 0;
    static uint32_t g_moduleStopCnt = 0;
    static uint32_t g_moduleReportCnt = 0;
    static uint32_t g_cacheEvictCnt = 0;
    static uint32_t g_filterMaskMatchCnt = 0;
    static bool g_moduleCbkLogged = false;
    static bool g_cacheEvictLogged = false;
    static bool g_filterMaskLogged = false;

    /* 模块级回调记录桩：计数 + 首次打印一条 */
    static void FuzzModuleScanEvent(NLSTK_Errcode_E resultCode, bool isStartScan)
    {
        if (isStartScan) {
            g_moduleStartCnt++;
        } else {
            g_moduleStopCnt++;
        }
        if (!g_moduleCbkLogged) {
            NLSTK_LOG_INFO("[FUZZ] module scan cbk first reach, code:%d isStart:%u", resultCode, isStartScan);
            g_moduleCbkLogged = true;
        }
    }

    static void FuzzModuleScanReport(NLSTK_DevdAdvResult_S *result)
    {
        g_moduleReportCnt++;
        if (g_moduleReportCnt == 1) {
            NLSTK_LOG_INFO("[FUZZ] module report cbk first reach, nameLen:%u uuidCnt:%u",
                result->localName.len, result->serviceUuids->size);
        }
    }

    static NLSTK_DevdAdvReportInfo_S *FuzzBuildAdvReport(const uint8_t *addr, uint8_t eventType,
        const uint8_t *data, uint8_t dataLen)
    {
        NLSTK_DevdAdvReportInfo_S *report = (NLSTK_DevdAdvReportInfo_S *)SDF_MemZalloc(
            sizeof(NLSTK_DevdAdvReportInfo_S) + dataLen);
        if (report == NULL) {
            return NULL;
        }
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            report->addr[i] = addr[i];
        }
        report->addrType = 0;
        report->extendParams.eventType = eventType;
        report->extendParams.primFrameType = 0; /* 帧一 */
        report->rssi = 0xB0;
        report->dataLength = dataLen;
        if (dataLen > 0) {
            (void)memcpy_s(report->data, dataLen, data, dataLen);
        }
        return report;
    }

    static void FuzzFeedAdvReport(const uint8_t *addr, uint8_t eventType, const uint8_t *data, uint8_t dataLen)
    {
        NLSTK_DevdAdvReportInfo_S *report = FuzzBuildAdvReport(addr, eventType, data, dataLen);
        if (report == NULL) {
            return;
        }
        NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
        if (result != NULL) {
            FreeAdvResult(result);
        }
        SDF_MemFree(report);
    }

    /* S6: 缓存逐出（MAX_DEVICE_CACHE=7，devd_report_parse.c）：7 址 + 1 逐出 + 被逐地址重发现 */
    static void FuzzCacheEviction(void)
    {
        static const uint8_t rspData[] = {0x0B, 0x02, 0x41, 0x42};
        uint8_t addr[SLE_ADDR_LEN] = {0};
        addr[5] = 0xC1;
        for (uint8_t i = 0; i < FUZZ_SEVEN; i++) {
            addr[0] = i;
            FuzzFeedAdvReport(addr, 0x02, g_fuzzDisNameAdv, sizeof(g_fuzzDisNameAdv)); /* scannable 先缓存 */
        }
        addr[0] = FUZZ_SEVEN;
        FuzzFeedAdvReport(addr, 0x02, g_fuzzDisNameAdv, sizeof(g_fuzzDisNameAdv)); /* 第 8 址触发逐出最老缓存 */
        addr[0] = 0; /* 被逐出的最老地址直接送 scanRsp：缓存未命中 -> 重加缓存并上报（重发现） */
        NLSTK_DevdAdvReportInfo_S *report = FuzzBuildAdvReport(addr, 0x08, rspData, sizeof(rspData));
        if (report == NULL) {
            return;
        }
        NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
        if (result != NULL) {
            g_cacheEvictCnt++;
            if (!g_cacheEvictLogged) {
                NLSTK_LOG_INFO("[FUZZ] cache eviction redelivery first reach");
                g_cacheEvictLogged = true;
            }
            FreeAdvResult(result);
        }
        SDF_MemFree(report);
    }

    /* S7: 过滤 mask 等长/不等长/全零/全一/逐字节差异，多组过滤器配置（等价多扫描器） */
    static void FuzzFilterMaskVariants(void)
    {
        static const uint8_t advData[] = {
            0x0B, 0x03, 0x41, 0x42, 0x43,           /* 本地名称 ABC */
            0x05, 0x02, 0x06, 0x09,                 /* 16 位服务 UUID，展开后末两字节 06 09 */
            0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD,     /* 服务数据 uuid AABB + 载荷 CCDD */
            0xFF, 0x04, 0x01, 0x02, 0x03, 0x04,     /* 厂商数据 id 0x0201 + 载荷 0304 */
        };
        uint8_t advLen = sizeof(advData);
        NLSTK_DevdAdvReportInfo_S *report = FuzzBuildAdvReport(g_fuzzAddrA, 0x00, advData, advLen);
        if (report == NULL) {
            return;
        }
        NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
        SDF_MemFree(report);
        if (result == NULL) {
            return;
        }
        NLSTK_DevdScanFilter_S filters[FUZZ_FIVE] = {};
        /* 0: uuid 全一 mask 精确匹配；1: uuid 全零 mask 放行；
         * 2: 服务数据等长 mask 逐字节差异；3: 厂商数据不等长 mask 退化为精确比较；4: 通配 */
        filters[FUZZ_ZERO].hasServiceUuid = true;
        filters[FUZZ_ZERO].hasServiceUuidMask = true;
        filters[FUZZ_ONE].hasServiceUuid = true;
        filters[FUZZ_ONE].hasServiceUuidMask = true;
        filters[FUZZ_TWO].serviceData.len = FUZZ_FOUR;
        filters[FUZZ_TWO].serviceDataMask.len = FUZZ_FOUR;
        filters[FUZZ_THREE].manufacturerId = 0x0201;
        filters[FUZZ_THREE].manufacturerData.len = FUZZ_TWO;
        filters[FUZZ_THREE].manufacturerDataMask.len = FUZZ_ONE;
        filters[FUZZ_FOUR].isNoFilter = true;
        /* 过滤 UUID 取 DIS 完整 128 位值（baseUuid + 末两字节 06 09），全一 mask 走精确比较 */
        static const uint8_t disUuid[SERVICE_UUID_LEN_128] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
            0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x09};
        static const uint8_t svcData[FUZZ_FOUR] = {0xAA, 0xBB, 0xCC, 0xDD};
        static const uint8_t svcMask[FUZZ_FOUR] = {0xFF, 0x00, 0xFF, 0x00}; /* 逐字节差异 */
        static const uint8_t mfgData[FUZZ_TWO] = {0x03, 0x04};
        static const uint8_t mfgMask[FUZZ_ONE] = {0xFF};
        for (uint8_t f = 0; f < FUZZ_FIVE; f++) {
            if (!filters[f].hasServiceUuid) {
                continue;
            }
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filters[f].serviceUuid[i] = disUuid[i];
                filters[f].serviceUuidMask[i] = (f == FUZZ_ZERO) ? 0xFF : 0x00; /* 全一/全零两种变体 */
            }
        }
        filters[FUZZ_TWO].serviceData.data = (uint8_t *)svcData;
        filters[FUZZ_TWO].serviceDataMask.data = (uint8_t *)svcMask;
        filters[FUZZ_THREE].manufacturerData.data = (uint8_t *)mfgData;
        filters[FUZZ_THREE].manufacturerDataMask.data = (uint8_t *)mfgMask;
        for (uint8_t f = 0; f < FUZZ_FIVE; f++) {
            SDF_Vector_S *filtersVec = DevdConvertFiltersToVector(&filters[f], FUZZ_ONE);
            if (filtersVec == NULL) {
                continue;
            }
            if (DevdIsMatchFilters(result, filtersVec)) {
                g_filterMaskMatchCnt++;
                if (!g_filterMaskLogged) {
                    NLSTK_LOG_INFO("[FUZZ] filter mask variant first match");
                    g_filterMaskLogged = true;
                }
            }
            SDF_DestroyVector(filtersVec);
        }
        FreeAdvResult(result);
    }

    /* 构造 DLI_AdvReportEvt 并直调 DevdSleScanReportCbk（覆盖 size/dataLength 校验，devd_scan.c:218） */
    static void FuzzFeedScanReportEvt(const uint8_t *addr, uint8_t eventType, const uint8_t *data, uint8_t dataLen)
    {
        size_t evtSize = sizeof(DLI_AdvReportEvt) + dataLen;
        DLI_AdvReportEvt *evt = (DLI_AdvReportEvt *)SDF_MemZalloc(evtSize);
        if (evt == NULL) {
            return;
        }
        evt->eventType = eventType;
        evt->addrType = 0;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            evt->addr[i] = addr[i];
        }
        evt->primFrameType = 0; /* 帧一 */
        evt->rssi = 0xB0;
        evt->dataLength = dataLen;
        for (uint8_t i = 0; i < dataLen; i++) {
            evt->data[i] = data[i];
        }
        DLI_ExecuteCmdRetParam cmdRes = {};
        cmdRes.size = (uint32_t)evtSize;
        cmdRes.eventParameter = evt;
        DevdSleScanReportCbk(NULL, DLI_SUCCESS, &cmdRes);
        SDF_MemFree(evt);
    }

    /* 生命周期闭环 + 乱序对抗。整体作为单个任务投递保证同线程顺序驱动：
     * Reg→Alloc→StartScan(STARTING)→SetScanParamCbk(成功,自动Enable)→EnableScanCbk(START_OK→STARTED)
     * →ScanReportCbk→ReportCbk→过滤→onScanCallback→StopScan(STOPPING)→EnableScanCbk(STOP_OK→STOPPED)
     * →Dereg/Remove。fuzz 下 DLI 桩不回包，后两段回调靠此处直调补全。 */
    static void FuzzScanLifecycleClosedLoop(void *arg)
    {
        (void)arg;
        (void)DevdScanInit(); /* 确保缓存链表初始化且模块真实 ScanCbk/ReportCbk 已注册到全局单点 */

        /* S6: 缓存逐出（7 址满 + 第 8 址逐出最老 + 被逐地址重发现） */
        FuzzCacheEviction();
        /* S7: 过滤 mask 等长/不等长/全零/全一/逐字节差异 */
        FuzzFilterMaskVariants();

        DevdScanModuleParam_S modParam = {};
        modParam.scanCbk.onStartOrStopEvent = FuzzModuleScanEvent;
        modParam.scanCbk.onScanCallback = FuzzModuleScanReport;
        DevdRegScanModule(&modParam);
        if (modParam.moduleId >= DEVD_MAX_MODULE_ID) {
            DevdScanDeInit();
            return;
        }
        DevdScannerParam_S scnParam = {};
        scnParam.moduleId = modParam.moduleId;
        DevdAllocScannerId(&scnParam);
        if (scnParam.scannerId == DEVD_INVALID_SCANNER_ID) {
            DevdDeregScanModule(&modParam);
            DevdScanDeInit();
            return;
        }
        DevdStartScanParam_S startParam = {};
        startParam.scannerId = scnParam.scannerId;
        startParam.setting.scanMode = SCAN_MODE_LOW_POWER;
        startParam.setting.frameType = DEVD_SCAN_FRAME_TYPE_1;
        startParam.filters = DevdConvertFiltersToVector(NULL, 0); /* 通配过滤器，保证上报可达 */
        if (startParam.filters == NULL) {
            DevdRemoveScannerId(&scnParam);
            DevdDeregScanModule(&modParam);
            DevdScanDeInit();
            return;
        }
        DevdStartScan(&startParam); /* STOPPED→STARTING */
        /* 乱序对抗：STARTING 态早到 EnableScanCbk（默认 DISABLE_SCAN 事件，STARTING 忽略）、
         * 畸形 ReportCbk（cmdRes NULL，校验拒绝）、仅记录不 abort */
        DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);
        DevdSleScanReportCbk(NULL, DLI_SUCCESS, NULL);
        /* 正向闭环：SetScanParamCbk 成功后产品侧自动 DevdSleEnableScan */
        DevdSleSetScanParamCbk(NULL, DLI_SUCCESS, NULL);
        DevdSleSetScanParamCbk(NULL, DLI_SUCCESS, NULL); /* 重复 SetParamCbk */
        DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);   /* DEVD_START_OK→STARTED，模块回调到达 */
        /* S3：帧一非可扫描直报 → 解析 DIS 名称 → onScanCallback */
        FuzzFeedScanReportEvt(g_fuzzAddrA, 0x00, g_fuzzDisNameAdv, sizeof(g_fuzzDisNameAdv));
        /* S2：同址双帧 0x02 先缓存 → 0x08 追加拼接后上报 */
        FuzzFeedScanReportEvt(g_fuzzAddrA, 0x02, g_fuzzOldLtvAdv, sizeof(g_fuzzOldLtvAdv));
        FuzzFeedScanReportEvt(g_fuzzAddrA, 0x08, g_fuzzDisNameAdv, sizeof(g_fuzzDisNameAdv));
        /* 停止：STOPPING 态 EnableScanCbk 走 disable 分支 → DEVD_STOP_OK→STOPPED */
        DevdStopScan(&startParam.scannerId);
        DevdSleEnableScanCbk(NULL, DLI_SUCCESS, NULL);
        /* 停止后晚到回包交错：成功/失败 SetParamCbk+EnableScanCbk、畸形 ReportCbk，仅记录不 abort */
        DevdSleSetScanParamCbk(NULL, DLI_SUCCESS, NULL);
        DevdSleEnableScanCbk(NULL, FUZZ_ONE, NULL);
        DLI_ExecuteCmdRetParam badRes = {};
        uint8_t badBuf[FUZZ_FOUR] = {0};
        badRes.size = FUZZ_ONE; /* size < sizeof(DLI_AdvReportEvt)，校验拒绝 */
        badRes.eventParameter = badBuf;
        DevdSleScanReportCbk(NULL, DLI_SUCCESS, &badRes);
        DevdSleScanReportCbk(NULL, DLI_SUCCESS, NULL);

        SDF_DestroyVector(startParam.filters);
        DevdRemoveScannerId(&scnParam);
        DevdDeregScanModule(&modParam);
        DevdScanDeInit();
    }

    /* ========== nlstk_scan_api.h APIs ========== */
    void FuzzNlstkScanApi(uint8_t *fuzzData, size_t size, ScanFuzzState *state)
    {
        SchedulePostTask(FuzzDevdScanInit, NULL, NULL);

        FuzzedDataProvider fdp(fuzzData, size);

        uint8_t moduleId = 0;
        NLSTK_DevdScanCbk_S cbk = {0};
        cbk.onStartOrStopEvent = FuzzModuleScanEvent;
        cbk.onScanCallback = FuzzModuleScanReport;
        NLSTK_Errcode_E ret = NLSTK_DevdRegScanModule(&moduleId, &cbk);
        if (ret == NLSTK_OK) {
            state->moduleRegistered = true;
            state->moduleId = moduleId;
        }

        uint32_t scannerId = 0;
        ret = NLSTK_DevdAllocScannerId(state->moduleId, &scannerId);
        if (ret == NLSTK_OK) {
            state->scannerAllocated = true;
            state->scannerId = scannerId;
        }
        (void)NLSTK_DevdAllocScannerId(fdp.ConsumeIntegral<uint8_t>(), &scannerId);

        NLSTK_DevdScanSetting_S setting = {0};
        setting.reportDelayMillis = fdp.ConsumeIntegral<int8_t>();
        setting.duration = fdp.ConsumeIntegral<uint16_t>();
        setting.scanMode = fdp.ConsumeIntegral<uint8_t>() % SCAN_MODE_INVALID;
        setting.legacy = fdp.ConsumeBool();
        setting.phy = fdp.ConsumeIntegral<uint8_t>() % FUZZ_THREE;
        setting.frameType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;

        NLSTK_DevdScanFilter_S filter = {0};
        filter.filterIndex = fdp.ConsumeIntegral<uint8_t>();
        filter.action = fdp.ConsumeIntegral<uint8_t>();

        filter.hasAddr = fdp.ConsumeBool();
        if (filter.hasAddr) {
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                filter.addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            filter.addr.type = fdp.ConsumeIntegral<uint8_t>();
        }

        FuzzGetVariableData(fdp, &filter.name, FUZZ_THIRTYTWO);

        filter.hasServiceUuid = fdp.ConsumeBool();
        if (filter.hasServiceUuid) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceUuid[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        filter.hasServiceUuidMask = fdp.ConsumeBool();
        if (filter.hasServiceUuidMask) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceUuidMask[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        filter.hasSolicitationUuid = fdp.ConsumeBool();
        if (filter.hasSolicitationUuid) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceSolicitationUuid[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        filter.hasSolicitationUuidMask = fdp.ConsumeBool();
        if (filter.hasSolicitationUuidMask) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceSolicitationUuidMask[i] = fdp.ConsumeIntegral<uint8_t>();
            }
        }

        FuzzGetVariableData(fdp, &filter.serviceData, FUZZ_THIRTYTWO);
        FuzzGetVariableData(fdp, &filter.serviceDataMask, FUZZ_THIRTYTWO);

        filter.manufacturerId = fdp.ConsumeIntegral<uint16_t>();
        FuzzGetVariableData(fdp, &filter.manufacturerData, FUZZ_THIRTYTWO);
        FuzzGetVariableData(fdp, &filter.manufacturerDataMask, FUZZ_THIRTYTWO);

        filter.hasRssiThreshold = fdp.ConsumeBool();
        if (filter.hasRssiThreshold) {
            filter.rssiThreshold = fdp.ConsumeIntegral<int8_t>();
        }

        filter.isSensorHubChannel = fdp.ConsumeBool();
        filter.advFrameType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
        filter.meshInfoReport = fdp.ConsumeBool();
        bool enableFilter = fdp.ConsumeBool();
        NLSTK_DevdScanFilter_S *filterPtr = enableFilter ? &filter : NULL;
        uint16_t filtersNum = enableFilter ? 1 : 0;

        NLSTK_DevdStartScan(state->scannerId, &setting, filterPtr, filtersNum);
        NLSTK_DevdStartScan(fdp.ConsumeIntegral<uint32_t>(), &setting, filterPtr, filtersNum);

        FuzzFreeVariableData(&filter.name);
        FuzzFreeVariableData(&filter.serviceData);
        FuzzFreeVariableData(&filter.serviceDataMask);
        FuzzFreeVariableData(&filter.manufacturerData);
        FuzzFreeVariableData(&filter.manufacturerDataMask);

        NLSTK_DevdStopScan(state->scannerId);
        NLSTK_DevdStopScan(fdp.ConsumeIntegral<uint32_t>());

        NLSTK_DevdRemoveScannerId(state->scannerId);
        NLSTK_DevdRemoveScannerId(fdp.ConsumeIntegral<uint32_t>());
        state->scannerAllocated = false;
        state->scannerId = 0;

        NLSTK_DevdDeregScanModule(state->moduleId);
        NLSTK_DevdDeregScanModule(fdp.ConsumeIntegral<uint8_t>());
        state->moduleRegistered = false;
        state->moduleId = 0;

        NLSTK_DevdStopAllScan();

        // 确保均在同一线程中执行
        FuzzAdvReportArg_S *advReportArg = (FuzzAdvReportArg_S *)SDF_MemZalloc(sizeof(FuzzAdvReportArg_S));
        if (advReportArg == NULL) {
            SchedulePostTask(FuzzDevdAdvDataReport, NULL, NULL);
        } else {
            advReportArg->data = (uint8_t *)SDF_MemZalloc(size);
            if (advReportArg->data == NULL) {
                SDF_MemFree(advReportArg);
                SchedulePostTask(FuzzDevdAdvDataReport, NULL, NULL);
            } else {
                advReportArg->size = size;
                (void)memcpy_s(advReportArg->data, size, fuzzData, size);
                SchedulePostTask(FuzzDevdAdvDataReport, advReportArg, FreeAdvReportArg);
            }
        }

        SchedulePostTask(FuzzDevdScanDeInit, NULL, NULL);
    }

    void FuzzMultiScanApi(uint8_t* data, size_t size)
    {
        if (data == NULL || size < 1) {
            return;
        }

        ScanFuzzState state;
        ResetScanState(&state);

        FuzzNlstkScanApi(data, size, &state);
        FuzzDevdApi(data, size);

        /* PR-5: 生命周期闭环任务（含 S6 缓存逐出 / S7 过滤 mask 变体与乱序对抗） */
        SchedulePostTask(FuzzScanLifecycleClosedLoop, NULL, NULL);

        ResetScanState(&state);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;

    (void)SdfInit();
    (void)ScheduleEnable();
    DevdLocalDeviceInit();
    DevdEnable();
    NLSTK_LOG_INFO("multiscan_fuzzer init success");
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::g_dliThreadUtil.InitThread();
    uint8_t *fuzzData = (uint8_t *)SDF_MemZalloc(size);
    if (fuzzData == NULL) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzMultiScanApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}