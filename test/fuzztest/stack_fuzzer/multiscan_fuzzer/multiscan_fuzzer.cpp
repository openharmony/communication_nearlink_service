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

static inline uint8_t FuzzGetU8(uint8_t* fuzzData, size_t* idx, size_t size)
{
    if (size == 0) {
        return 0;
    }
    uint8_t val = fuzzData[*idx % size];
    *idx += 1;
    return val;
}

static inline uint16_t FuzzGetU16(uint8_t* fuzzData, size_t* idx, size_t size)
{
    if (size == 0) {
        return 0;
    }
    uint16_t val = fuzzData[*idx % size] | (fuzzData[(*idx + 1) % size] << 8);
    *idx += 2;
    return val;
}

static inline uint32_t FuzzGetU32(uint8_t* fuzzData, size_t* idx, size_t size)
{
    if (size == 0) {
        return 0;
    }
    uint32_t val = fuzzData[*idx % size] | (fuzzData[(*idx + 1) % size] << 8) |
                   (fuzzData[(*idx + 2) % size] << 16) | (fuzzData[(*idx + 3) % size] << 24);
    *idx += 4;
    return val;
}

static inline int8_t FuzzGetI8(uint8_t* fuzzData, size_t* idx, size_t size)
{
    return (int8_t)FuzzGetU8(fuzzData, idx, size);
}

static inline void FuzzGetVariableData(uint8_t* fuzzData, size_t* idx, size_t size,
    NLSTK_VariableData_S* varData, uint8_t maxLen)
{
    if (size == 0) {
        varData->len = 0;
        varData->data = nullptr;
        return;
    }
    uint8_t len = FuzzGetU8(fuzzData, idx, size) % (maxLen + 1);
    varData->len = len;
    if (len > 0) {
        varData->data = fuzzData + (*idx % size);
    } else {
        varData->data = nullptr;
    }
    *idx += len;
}

static void GenerateRandomData(uint8_t* buf, uint8_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = rand() % 0xFF;
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

    static uint8_t* GenRandBuf(size_t len)
    {
        uint8_t* buf = (uint8_t*)SDF_MemZalloc(len);
        if (buf == nullptr) {
            return nullptr;
        }
        for (size_t i = 0; i < len; i++) {
            buf[i] = rand() % 0xFF;
        }
        return buf;
    }

    static void FuzzNlstkDevdSetAdvData(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        NLSTK_DevdSetAdvData_S setAdvData = {0};
        setAdvData.advHandle = FuzzGetU8(fuzzData, idx, size);

        uint16_t advDataLen = FuzzGetU16(fuzzData, idx, size) % (MAX_ADV_DATA_LEN + 1);
        setAdvData.data.advDataLen = advDataLen;
        if (advDataLen > 0) {
            setAdvData.data.advData = GenRandBuf(advDataLen);
            if (setAdvData.data.advData == nullptr) {
                return;
            }
        }

        uint16_t scanRspDataLen = FuzzGetU16(fuzzData, idx, size) % (MAX_ADV_DATA_LEN + 1);
        setAdvData.data.scanRspDataLen = scanRspDataLen;
        if (scanRspDataLen > 0) {
            setAdvData.data.scanRspData = GenRandBuf(scanRspDataLen);
            if (setAdvData.data.scanRspData == nullptr) {
                SDF_MemFree(setAdvData.data.advData);
                return;
            }
        }

        (void)NLSTK_DevdSetAdvData(&setAdvData);

        SDF_MemFree(setAdvData.data.advData);
        SDF_MemFree(setAdvData.data.scanRspData);
    }

    static void FuzzNlstkDevdStartAdv(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        NLSTK_DevdSetAdvParams_S advParams = {0};
        advParams.accessMode = FuzzGetU8(fuzzData, idx, size) % (ADV_ACCESS_MODE_DEFAULT + 1);
        advParams.discoveryLevel = FuzzGetU8(fuzzData, idx, size) % (ADV_DISCOVERY_LEVEL_SPECIAL + 1);
        advParams.param.advHandle = FuzzGetU8(fuzzData, idx, size);
        advParams.param.advMode = FuzzGetU8(fuzzData, idx, size) % (ADV_MODE_CONNECTABLE_DIRECTED + 1);
        if (advParams.param.advMode == ADV_MODE_CONNECTABLE_DIRECTED) {
            advParams.param.advMode = ADV_MODE_CONNECTABLE_SCANABLE;
        }
        advParams.param.advGtRole = FuzzGetU8(fuzzData, idx, size) % (ADV_GT_ROLE_G_NO_NEGO + 1);
        advParams.param.advIntervalMin = FuzzGetU32(fuzzData, idx, size);
        if (advParams.param.advIntervalMin < DEVD_ADV_INTERVAL_MIN) {
            advParams.param.advIntervalMin = DEVD_ADV_INTERVAL_MIN;
        }
        advParams.param.advIntervalMax = FuzzGetU32(fuzzData, idx, size);
        if (advParams.param.advIntervalMax < advParams.param.advIntervalMin) {
            advParams.param.advIntervalMax = advParams.param.advIntervalMin;
        }
        if (advParams.param.advIntervalMax > DEVD_ADV_INTERVAL_MAX) {
            advParams.param.advIntervalMax = DEVD_ADV_INTERVAL_MAX;
        }
        advParams.param.advChannelMap = FuzzGetU8(fuzzData, idx, size) % (ADV_CHANNEL_MAP_DEFAULT + 1);
        advParams.param.ownAddr.type = FuzzGetU8(fuzzData, idx, size) % ADDR_TYPE_END;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParams.param.ownAddr.addr[i] = FuzzGetU8(fuzzData, idx, size);
        }
        advParams.param.peerAddr.type = FuzzGetU8(fuzzData, idx, size) % ADDR_TYPE_END;
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParams.param.peerAddr.addr[i] = FuzzGetU8(fuzzData, idx, size);
        }
        advParams.param.primaryFrameType = FuzzGetU8(fuzzData, idx, size) % (ADV_FRAME_TYPE_4 + 1);
        advParams.param.extParam = nullptr;
        advParams.param.connParam = nullptr;

        uint16_t advDataLen = FuzzGetU16(fuzzData, idx, size) % (MAX_ADV_DATA_LEN + 1);
        advParams.data.advDataLen = advDataLen;
        if (advDataLen > 0) {
            advParams.data.advData = GenRandBuf(advDataLen);
            if (advParams.data.advData == nullptr) {
                return;
            }
        }
        uint16_t scanRspDataLen = FuzzGetU16(fuzzData, idx, size) % (MAX_ADV_DATA_LEN + 1);
        advParams.data.scanRspDataLen = scanRspDataLen;
        if (scanRspDataLen > 0) {
            advParams.data.scanRspData = GenRandBuf(scanRspDataLen);
            if (advParams.data.scanRspData == nullptr) {
                SDF_MemFree(advParams.data.advData);
                return;
            }
        }

        (void)NLSTK_DevdStartAdv(&advParams);

        SDF_MemFree(advParams.data.advData);
        SDF_MemFree(advParams.data.scanRspData);
    }

    static void FuzzNlstkDevdEnableAdv(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        NLSTK_DevdSetAdvEnable_S setEnable = {0};
        setEnable.advHandle = g_advHandle;
        setEnable.enable = FuzzGetU8(fuzzData, idx, size) % FUZZ_TWO;
        setEnable.duration = FuzzGetU16(fuzzData, idx, size);
        setEnable.maxAdvEvent = FuzzGetU8(fuzzData, idx, size);
        (void)NLSTK_DevdEnableAdv(&setEnable);
    }

    static void FuzzNlstkDevdSetTxPower(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        NLSTK_DevdSetTxPower_S setTxPower = {0};
        setTxPower.bleMaxPower = FuzzGetI8(fuzzData, idx, size);
        setTxPower.sleMaxPower = FuzzGetI8(fuzzData, idx, size);
        (void)NLSTK_DevdSetTxPower(&setTxPower);
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

    static void FuzzNlstkDevdSleStartScan(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        uint8_t frameType = FuzzGetU8(fuzzData, idx, size) % FUZZ_THREE;
        uint8_t phyCount = FuzzGetU8(fuzzData, idx, size) % FUZZ_THREE;
        size_t totalSize = sizeof(NLSTK_DevdSleScanParams_S) + phyCount * sizeof(NLSTK_DevdSleScanParamsNoPhy_S);
        NLSTK_DevdSleScanParams_S* sleScanParams = (NLSTK_DevdSleScanParams_S*)SDF_MemZalloc(totalSize);
        if (sleScanParams == nullptr) {
            return;
        }

        sleScanParams->localAddrType = FuzzGetU8(fuzzData, idx, size) % ADDR_TYPE_END;
        sleScanParams->scanFilterPolicy = FuzzGetU8(fuzzData, idx, size) % (SCAN_FLT_EXTEND + 1);
        sleScanParams->frameType = frameType & 3;
        for (uint8_t i = 0; i < phyCount; i++) {
            sleScanParams->params[i].scanType = FuzzGetU8(fuzzData, idx, size) % (SCAN_TYPE_ACTIVE + 1);
            uint16_t scanInterval = FuzzGetU16(fuzzData, idx, size);
            sleScanParams->params[i].scanInterval =
                (scanInterval < SCAN_INTERVAL_MIN) ? SCAN_INTERVAL_MIN : scanInterval;
            uint16_t scanWindow = FuzzGetU16(fuzzData, idx, size);
            sleScanParams->params[i].scanWindow = (scanWindow < SCAN_WINDOW_MIN) ? SCAN_WINDOW_MIN : scanWindow;
        }

        (void)NLSTK_DevdSleStartScan(sleScanParams);
        SDF_MemFree(sleScanParams);
    }

    static void FuzzNlstkDevdSleEnableScan(uint8_t* fuzzData, size_t* idx, size_t size)
    {
        NLSTK_DevdSleScanEnable_S sleScanEnable = {0};
        sleScanEnable.scanEnable = FuzzGetU8(fuzzData, idx, size) % FUZZ_TWO;
        sleScanEnable.scanFilterDuplicates = FuzzGetU8(fuzzData, idx, size) % FUZZ_TWO;
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

    void FuzzDevdApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        g_advHandle = 0;
        size_t idx = 0;

        FuzzNlstkDevdCreateAdvHandle();

        FuzzNlstkDevdSetAdvData(data, &idx, size);

        idx = 0;
        FuzzNlstkDevdStartAdv(data, &idx, size);

        FuzzNlstkDevdEnableAdv(data, &idx, size);

        FuzzNlstkDevdSetTxPower(data, &idx, size);

        FuzzNlstkDevdRemoveAdv();

        FuzzNlstkDevdRegScanEventCbk();

        FuzzNlstkDevdCreateAdvHandle();

        FuzzNlstkDevdSleStartScan(data, &idx, size);

        FuzzNlstkDevdSleEnableScan(data, &idx, size);
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
        uint8_t data[0xFF] = {0};
        uint8_t pos = 0;
        data[pos++] = 0x01;
        data[pos++] = 0x01;
        GenerateRandomData(&data[pos], 0x01);
        pos += 0x01;

        data[pos++] = 0x03;
        data[pos++] = 0x10;
        GenerateRandomData(&data[pos], 0x10);
        pos += 0x10;

        data[pos++] = 0x04;
        data[pos++] = 0x20;
        GenerateRandomData(&data[pos], 0x20);
        pos += 0x20;

        data[pos++] = 0x05;
        data[pos++] = 0x08;
        GenerateRandomData(&data[pos], 0x08);
        pos += 0x08;

        data[pos++] = 0x07;
        data[pos++] = 0x04;
        GenerateRandomData(&data[pos], 0x04);
        pos += 0x04;

        data[pos++] = 0x0A;
        data[pos++] = 0x10;
        GenerateRandomData(&data[pos], 0x10);
        pos += 0x10;

        data[pos++] = 0x0B;
        data[pos++] = 0x10;
        GenerateRandomData(&data[pos], 0x10);
        pos += 0x10;

        data[pos++] = 0x0C;
        data[pos++] = 0x01;
        GenerateRandomData(&data[pos], 0x01);
        pos += 0x01;

        data[pos++] = 0x10;
        data[pos++] = 0x10;
        GenerateRandomData(&data[pos], 0x10);
        pos += 0x10;

        data[pos++] = 0xFF;
        data[pos++] = 0x10;
        GenerateRandomData(&data[pos], 0x10);
        pos += 0x10;

        NLSTK_DevdScanFilter_S *filters = (NLSTK_DevdScanFilter_S *)
            SDF_MemZalloc(sizeof(NLSTK_DevdScanFilter_S) * 0x05);
        uint16_t filtersNum = 0x05;
        filters[0].hasServiceUuid = true;
        filters[1].hasSolicitationUuid = true;
        filters[2].hasRssiThreshold = true;
        filters[3].meshInfoReport = true;
        filters[4].isNoFilter = true;
        SDF_Vector_S *filtersVec = DevdConvertFiltersToVector(filters, filtersNum);

        NLSTK_DevdAdvReportInfo_S *report = (NLSTK_DevdAdvReportInfo_S *)
            SDF_MemZalloc(sizeof(NLSTK_DevdAdvReportInfo_S) + pos);
        if (report == NULL) {
            return;
        }
        report->extendParams.eventType = rand() % 0xFF;
        report->extendParams.primFrameType = rand() % 0x02;
        report->rssi = rand() % 0xFF;
        report->dataLength = pos;
        (void)memcpy_s(report->data, pos, data, pos);
        NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
        if (result != NULL) {
            DevdIsMatchFilters(result, filtersVec);
            FreeAdvResult(result);
        }

        GenerateRandomData(report->data, report->dataLength);
        result = DevdAdvDataReport(report);
        if (result != NULL) {
            DevdIsMatchFilters(result, filtersVec);
            FreeAdvResult(result);
        }

        SDF_DestroyVector(filtersVec);
        SDF_MemFree(report);
    }

    /* ========== nlstk_scan_api.h APIs ========== */
    void FuzzNlstkScanApi(uint8_t* fuzzData, size_t size, ScanFuzzState* state)
    {
        SchedulePostTask(FuzzDevdScanInit, NULL, NULL);

        size_t idx = 0;
        (void)FuzzGetU8(fuzzData, &idx, size);

        uint8_t moduleId = 0;
        NLSTK_DevdScanCbk_S cbk = {0};
        cbk.onStartOrStopEvent = nullptr;
        cbk.onScanCallback = nullptr;
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
        (void)NLSTK_DevdAllocScannerId(FuzzGetU8(fuzzData, &idx, size), &scannerId);

        NLSTK_DevdScanSetting_S setting = {0};
        setting.reportDelayMillis = FuzzGetI8(fuzzData, &idx, size);
        setting.duration = FuzzGetU16(fuzzData, &idx, size);
        setting.scanMode = FuzzGetU8(fuzzData, &idx, size) % SCAN_MODE_INVALID;
        setting.legacy = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        setting.phy = FuzzGetU8(fuzzData, &idx, size) % FUZZ_THREE;
        setting.frameType = FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO;

        NLSTK_DevdScanFilter_S filter = {0};
        filter.filterIndex = FuzzGetU8(fuzzData, &idx, size);
        filter.action = FuzzGetU8(fuzzData, &idx, size);

        filter.hasAddr = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasAddr) {
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                filter.addr.addr[i] = FuzzGetU8(fuzzData, &idx, size);
            }
            filter.addr.type = FuzzGetU8(fuzzData, &idx, size);
        }

        FuzzGetVariableData(fuzzData, &idx, size, &filter.name, FUZZ_THIRTYTWO);

        filter.hasServiceUuid = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasServiceUuid) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceUuid[i] = FuzzGetU8(fuzzData, &idx, size);
            }
        }

        filter.hasServiceUuidMask = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasServiceUuidMask) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceUuidMask[i] = FuzzGetU8(fuzzData, &idx, size);
            }
        }

        filter.hasSolicitationUuid = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasSolicitationUuid) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceSolicitationUuid[i] = FuzzGetU8(fuzzData, &idx, size);
            }
        }

        filter.hasSolicitationUuidMask = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasSolicitationUuidMask) {
            for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
                filter.serviceSolicitationUuidMask[i] = FuzzGetU8(fuzzData, &idx, size);
            }
        }

        FuzzGetVariableData(fuzzData, &idx, size, &filter.serviceData, FUZZ_THIRTYTWO);
        FuzzGetVariableData(fuzzData, &idx, size, &filter.serviceDataMask, FUZZ_THIRTYTWO);

        filter.manufacturerId = FuzzGetU16(fuzzData, &idx, size);
        FuzzGetVariableData(fuzzData, &idx, size, &filter.manufacturerData, FUZZ_THIRTYTWO);
        FuzzGetVariableData(fuzzData, &idx, size, &filter.manufacturerDataMask, FUZZ_THIRTYTWO);

        filter.hasRssiThreshold = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        if (filter.hasRssiThreshold) {
            filter.rssiThreshold = FuzzGetI8(fuzzData, &idx, size);
        }

        filter.isSensorHubChannel = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        filter.advIndReport = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        filter.meshInfoReport = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        bool enableFilter = (FuzzGetU8(fuzzData, &idx, size) % FUZZ_TWO) == 1;
        NLSTK_DevdScanFilter_S *filterPtr = enableFilter ? &filter : nullptr;
        uint16_t filtersNum = enableFilter ? 1 : 0;

        NLSTK_DevdStartScan(state->scannerId, &setting, filterPtr, filtersNum);
        NLSTK_DevdStartScan(FuzzGetU32(fuzzData, &idx, size), &setting, filterPtr, filtersNum);

        NLSTK_DevdStopScan(state->scannerId);
        NLSTK_DevdStopScan(FuzzGetU32(fuzzData, &idx, size));

        NLSTK_DevdRemoveScannerId(state->scannerId);
        NLSTK_DevdRemoveScannerId(FuzzGetU32(fuzzData, &idx, size));
        state->scannerAllocated = false;
        state->scannerId = 0;

        NLSTK_DevdDeregScanModule(state->moduleId);
        NLSTK_DevdDeregScanModule(FuzzGetU8(fuzzData, &idx, size));
        state->moduleRegistered = false;
        state->moduleId = 0;

        NLSTK_DevdStopAllScan();

        // 确保均在同一线程中执行
        SchedulePostTask(FuzzDevdAdvDataReport, NULL, NULL);

        SchedulePostTask(FuzzDevdScanDeInit, NULL, NULL);
    }

    void FuzzMultiScanApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
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
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzMultiScanApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}