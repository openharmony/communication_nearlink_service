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

#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "sdf_vector.h"
#include "devd_scan_type.h"
#include "devd_scan_util.h"

void DevdFreeDevdScanner(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)ptr;
    SDF_DestroyVector(scanner->filters);
    SDF_MemFree(scanner);
}

void DevdFreeStartScanParam(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    DevdStartScanParam_S *param = (DevdStartScanParam_S *)ptr;
    SDF_DestroyVector(param->filters);
    SDF_MemFree(param);
}

NLSTK_DevdScanSettingInner_S *FetchFrame1TargetScanSettings(SDF_Vector_S *scanners)
{
    NLSTK_CHECK_RETURN(scanners->size != 0, NULL, "no scanner");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, 0);
    NLSTK_DevdScanSettingInner_S *target = &scanner->scanSetting;
    for (size_t i = 1; i < scanners->size; i++) {
        scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, i);
        // 最优扫描参数选择占空比（window/interval）大的，若占空比相同选interval小的
        if ((target->window * scanner->scanSetting.interval < target->interval * scanner->scanSetting.window) ||
            (target->window * scanner->scanSetting.interval == target->interval * scanner->scanSetting.window &&
             target->interval > scanner->scanSetting.interval)) {
            target = &scanner->scanSetting;
        }
    }
    return target;
}

NLSTK_DevdScanSettingInner_S *FetchFrame4TargetScanSettings(SDF_Vector_S *scanners)
{
    NLSTK_CHECK_RETURN(scanners->size != 0, NULL, "no scanner");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, 0);
    NLSTK_DevdScanSettingInner_S *target = &scanner->frame4ScanSetting;
    for (size_t i = 1; i < scanners->size; i++) {
        scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(scanners, i);
        // 最优扫描参数选择占空比（window/interval）大的，若占空比相同选interval小的
        if ((target->window * scanner->frame4ScanSetting.interval <
             target->interval * scanner->frame4ScanSetting.window) ||
            (target->window * scanner->frame4ScanSetting.interval ==
                 target->interval * scanner->frame4ScanSetting.window &&
             target->interval > scanner->frame4ScanSetting.interval)) {
            target = &scanner->frame4ScanSetting;
        }
    }
    return target;
}

bool CheckFiltersLegal(NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum)
{
    if (filtersNum != 0) {
        NLSTK_CHECK_RETURN(filters != NULL, false, "[DEVDS] filters is null");
    }
    for (uint16_t i = 0; i < filtersNum; i++) {
        if ((filters[i].name.len != 0) ^ (filters[i].name.data != NULL)) {
            NLSTK_LOG_ERROR("[DEVDS] filters i:%hu name.len:%hu", i, filters[i].name.len);
            return false;
        }
        if ((filters[i].serviceData.len != 0) ^ (filters[i].serviceData.data != NULL)) {
            NLSTK_LOG_ERROR("[DEVDS] filters i:%hu serviceData.len:%hu", i, filters[i].serviceData.len);
            return false;
        }
        if ((filters[i].serviceDataMask.len != 0) ^ (filters[i].serviceDataMask.data != NULL)) {
            NLSTK_LOG_ERROR("[DEVDS] filters i:%hu serviceDataMask.len:%hu", i, filters[i].serviceDataMask.len);
            return false;
        }
        if ((filters[i].manufacturerData.len != 0) ^ (filters[i].manufacturerData.data != NULL)) {
            NLSTK_LOG_ERROR("[DEVDS] filters i:%hu manufacturerData.len:%hu", i, filters[i].manufacturerData.len);
            return false;
        }
        if ((filters[i].manufacturerDataMask.len != 0) ^ (filters[i].manufacturerDataMask.data != NULL)) {
            NLSTK_LOG_ERROR("[DEVDS] filters i:%hu manufacturerDataMask.len:%hu", i,
                filters[i].manufacturerDataMask.len);
            return false;
        }
    }
    return true;
}

static void DevdFreeFilter(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    NLSTK_DevdScanFilter_S *filter = (NLSTK_DevdScanFilter_S *)ptr;
    if (filter->name.data != NULL) {
        SDF_MemFree(filter->name.data);
    }
    if (filter->serviceData.data != NULL) {
        SDF_MemFree(filter->serviceData.data);
    }
    if (filter->serviceDataMask.data != NULL) {
        SDF_MemFree(filter->serviceDataMask.data);
    }
    if (filter->manufacturerData.data != NULL) {
        SDF_MemFree(filter->manufacturerData.data);
    }
    if (filter->manufacturerDataMask.data != NULL) {
        SDF_MemFree(filter->manufacturerDataMask.data);
    }
    SDF_MemFree(filter);
}

static uint8_t *CopyMemoryField(uint8_t *src, size_t srcLen)
{
    uint8_t *dest = (uint8_t *)SDF_MemZalloc(srcLen);
    NLSTK_CHECK_RETURN(dest != NULL, NULL, "[MCP] dest malloc fail");
    (void)memcpy_s(dest, srcLen, src, srcLen);
    return dest;
}

static NLSTK_DevdScanFilter_S *MallocAndCopyFilter(NLSTK_DevdScanFilter_S *filter)
{
    NLSTK_DevdScanFilter_S *copyFilter = (NLSTK_DevdScanFilter_S *)SDF_MemZalloc(sizeof(NLSTK_DevdScanFilter_S));
    NLSTK_CHECK_RETURN(copyFilter != NULL, NULL, "[DEVDS] filter malloc fail");
    (void)memcpy_s(copyFilter, sizeof(NLSTK_DevdScanFilter_S), filter, sizeof(NLSTK_DevdScanFilter_S));
    copyFilter->name.data = NULL;
    copyFilter->serviceData.data = NULL;
    copyFilter->serviceDataMask.data = NULL;
    copyFilter->manufacturerData.data = NULL;
    copyFilter->manufacturerDataMask.data = NULL;
    if (filter->name.len != 0) {
        copyFilter->name.data = CopyMemoryField(filter->name.data, filter->name.len);
        if (copyFilter->name.data == NULL) {
            goto FAIL;
        }
    }
    if (filter->serviceData.len != 0) {
        copyFilter->serviceData.data = CopyMemoryField(filter->serviceData.data, filter->serviceData.len);
        if (copyFilter->serviceData.data == NULL) {
            goto FAIL;
        }
    }
    if (filter->serviceDataMask.len != 0) {
        copyFilter->serviceDataMask.data = CopyMemoryField(filter->serviceDataMask.data, filter->serviceDataMask.len);
        if (copyFilter->serviceDataMask.data == NULL) {
            goto FAIL;
        }
    }
    if (filter->manufacturerData.len != 0) {
        copyFilter->manufacturerData.data = CopyMemoryField(filter->manufacturerData.data,
            filter->manufacturerData.len);
        if (copyFilter->manufacturerData.data == NULL) {
            goto FAIL;
        }
    }
    if (filter->manufacturerDataMask.len != 0) {
        copyFilter->manufacturerDataMask.data = CopyMemoryField(filter->manufacturerDataMask.data,
            filter->manufacturerDataMask.len);
        if (copyFilter->manufacturerDataMask.data == NULL) {
            goto FAIL;
        }
    }
    return copyFilter;
FAIL:
    DevdFreeFilter(copyFilter);
    return NULL;
}

SDF_Vector_S *DevdConvertFiltersToVector(NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum)
{
    SDF_Traits traits = {
        .dtor = DevdFreeFilter,
    };
    SDF_Vector_S *filtersVector = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(filtersVector != NULL, NULL, "[DEVDS] filtersVector create fail");
    if (filtersNum == 0) {
        NLSTK_DevdScanFilter_S *filter = (NLSTK_DevdScanFilter_S *)SDF_MemZalloc(sizeof(NLSTK_DevdScanFilter_S));
        if (filter == NULL) {
            NLSTK_LOG_ERROR("[DEVDS] filtersVector create fail");
            SDF_DestroyVector(filtersVector);
            return NULL;
        }
        filter->isNoFilter = true;
        if (!SDF_VectorEmplaceBack(filtersVector, filter)) {
            NLSTK_LOG_ERROR("[DEVDS] filtersVector emplace back fail");
            DevdFreeFilter(filter);
            SDF_DestroyVector(filtersVector);
            return NULL;
        }
        return filtersVector;
    }
    for (uint16_t i = 0; i < filtersNum; i++) {
        NLSTK_DevdScanFilter_S *filter = MallocAndCopyFilter(&filters[i]);
        if (filter == NULL) {
            NLSTK_LOG_ERROR("[DEVDS] filter malloc fail");
            SDF_DestroyVector(filtersVector);
            return NULL;
        }
        if (!SDF_VectorEmplaceBack(filtersVector, filter)) {
            NLSTK_LOG_ERROR("[DEVDS] filtersVector emplace back fail");
            DevdFreeFilter(filter);
            SDF_DestroyVector(filtersVector);
            return NULL;
        }
    }
    return filtersVector;
}

SDF_Vector_S *DevdCloneFiltersVector(SDF_Vector_S *filters)
{
    if (filters == NULL) {
        return NULL;
    }
    SDF_Traits traits = {
        .dtor = DevdFreeFilter,
    };
    SDF_Vector_S *cloneVector = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(cloneVector != NULL, NULL, "[DEVDS] clone filtersVector create fail");
    for (size_t i = 0; i < filters->size; i++) {
        NLSTK_DevdScanFilter_S *filter = (NLSTK_DevdScanFilter_S *)SDF_VectorElementAt(filters, i);
        NLSTK_DevdScanFilter_S *cloneFilter = MallocAndCopyFilter(filter);
        if (cloneFilter == NULL) {
            NLSTK_LOG_ERROR("[DEVDS] filter clone fail");
            SDF_DestroyVector(cloneVector);
            return NULL;
        }
        if (!SDF_VectorEmplaceBack(cloneVector, cloneFilter)) {
            NLSTK_LOG_ERROR("[DEVDS] clone filtersVector emplace back fail");
            DevdFreeFilter(cloneFilter);
            SDF_DestroyVector(cloneVector);
            return NULL;
        }
    }
    return cloneVector;
}