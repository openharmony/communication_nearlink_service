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
#include "devd_scan_filter.h"
#include "devd_report_parse.h"
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

    static void DevdScanEventCbkStub(uint8_t eventMsg, uint8_t result)
    {
        (void)eventMsg;
        (void)result;
    }

    static void DevdAdvReportCbkStub(NLSTK_DevdAdvReportInfo_S *report)
    {
        (void)report;
    }

    static void DevdScanFilterCbkStub(NLSTK_DevdScanFilterInfo_S *info)
    {
        (void)info;
    }

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

    static void FuzzNlstkDevdRegScanEventCbk(void)
    {
        NLSTK_DevdSleScanExterCbk_S scanEventCbk = {0};
        scanEventCbk.scanCbk = DevdScanEventCbkStub;
        scanEventCbk.reportCbk = DevdAdvReportCbkStub;
        scanEventCbk.scanFilterCbk = DevdScanFilterCbkStub;
        (void)NLSTK_DevdRegScanEventCbk(&scanEventCbk);
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

        FuzzNlstkDevdRegScanEventCbk();

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
        report->extendParams.eventType = fdp.ConsumeIntegral<uint8_t>();
        report->extendParams.primFrameType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_TWO;
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

    /* ========== nlstk_scan_api.h APIs ========== */
    void FuzzNlstkScanApi(uint8_t *fuzzData, size_t size, ScanFuzzState *state)
    {
        SchedulePostTask(FuzzDevdScanInit, NULL, NULL);

        FuzzedDataProvider fdp(fuzzData, size);

        uint8_t moduleId = 0;
        NLSTK_DevdScanCbk_S cbk = {0};
        cbk.onStartOrStopEvent = NULL;
        cbk.onScanCallback = NULL;
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
        filter.advFrameType = static_cast<uint8_t>(fdp.ConsumeBool());
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