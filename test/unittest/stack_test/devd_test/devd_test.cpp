/*
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
 */

#include <cstring>
#include "securec.h"
#include "gtest/gtest.h"
#include "dpfwk_errcode.h"
#include "sdf_evc.h"
#include "sdf_timer.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "sdf_sem_api.h"
#include "nlstk_init_api.h"
#include "nlstk_devd.h"
#include "devd_scan.h"
#include "devd_tbl.h"
#include "devd_local.h"
#include "devd_adv.h"
#include "dli.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "devd_cbk.h"
#include <thread>
#include "common_ext_func_wrapper.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class DEVD_Test : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    void SetUp() override
    {
        NLSTK_InitStack();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    void TearDown() override
    {
        NLSTK_DeinitStack();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};


static void ReportCbk(NLSTK_DevdAdvReportInfo_S *report)
{
    (void)report;
}

static void AdvCbk(NLSTK_DevdAdvCbkParam_S *param)
{
    (void)param;
}

TEST_F(DEVD_Test, TestDEVD_SleSetScanParam)
{
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    scanParams->localAddrType = LONG_MECH_PUBLIC_ADDRESS;
    scanParams->scanFilterPolicy = SCAN_FLT_BASIC;
    scanParams->params[0].scanType = SCAN_TYPE_PASSIVE;
    scanParams->params[0].scanInterval = 0x0004;
    scanParams->params[0].scanWindow = 0x0004;
    DEVD_SCAN_MGR.tmpScanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanParams_S));

    DevdSleSetScanParam(scanParams);

    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_SET_SCAN_PARAMS);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams->localAddrType, LONG_MECH_PUBLIC_ADDRESS);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams->scanFilterPolicy, SCAN_FLT_BASIC);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams->params[0].scanType, SCAN_TYPE_PASSIVE);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams->params[0].scanInterval, 0x0004);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams->params[0].scanWindow, 0x0004);
    SDF_MemFree(scanParams);
}

TEST_F(DEVD_Test, TestDEVD_SleEnableScan_On)
{
    NLSTK_DevdSleScanEnable_S scanEnable;
    scanEnable.scanEnable = 1;
    scanEnable.scanFilterDuplicates = 1;

    DevdSleEnableScan(&scanEnable);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE);
}

TEST_F(DEVD_Test, TestDEVD_SleEnableScan_Off)
{
    NLSTK_DevdSleScanEnable_S scanEnable;
    scanEnable.scanEnable = 0;
    scanEnable.scanFilterDuplicates = 1;

    DevdSleEnableScan(&scanEnable);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE);
}


TEST_F(DEVD_Test, TestDEVD_SleScanReportCbk)
{
    DLI_ExecuteCmdRetParam *cfm_par;
    void *context = nullptr;
    uint16_t status = 0;
    NLSTK_DevdSleScanExterCbk_S *param = (NLSTK_DevdSleScanExterCbk_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanExterCbk_S));
    param->reportCbk = ReportCbk;
    DevdRegScanEventCbk(param);
    cfm_par = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
    cfm_par->size = sizeof(DLI_AdvReportEvt) + 1;
    cfm_par->eventParameter = SDF_MemZalloc(cfm_par->size);
    struct DLI_AdvReportEvt *par = (struct DLI_AdvReportEvt *)cfm_par->eventParameter;
    par->dataLength = 1;

    DevdSleScanReportCbk(context, status, cfm_par);

    EXPECT_EQ(par->data[0], 0);
    SDF_MemFree(param);
    SDF_MemFree(cfm_par->eventParameter);
    SDF_MemFree(cfm_par);
}

TEST_F(DEVD_Test, TestDEVD_SleScanReportCbk_Error)
{
    DLI_ExecuteCmdRetParam *cfm_par = nullptr;
    void *context = nullptr;
    uint16_t status = 0;
    DevdSleScanReportCbk(context, status, cfm_par);
    cfm_par = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
    DevdSleScanReportCbk(context, status, cfm_par);
    cfm_par->size = 1;
    cfm_par->eventParameter = SDF_MemZalloc(cfm_par->size);
    DevdSleScanReportCbk(context, status, cfm_par);
    SDF_MemFree(cfm_par->eventParameter);
    SDF_MemFree(cfm_par);
}

TEST_F(DEVD_Test, TestDEVD_SleSetScanParamCbk)
{
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    void *context = nullptr;
    DEVD_SCAN_MGR.scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanParams_S));
    uint16_t status = 0;
    DevdSleSetScanParamCbk(context, status, cmdRes);
    DEVD_SCAN_MGR.tmpScanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanParams_S));
    status = 1;
    DevdSleSetScanParamCbk(context, status, cmdRes);
    EXPECT_EQ(DEVD_SCAN_MGR.tmpScanParams, nullptr);
    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_IDLE);
}

TEST_F(DEVD_Test, TestDEVD_SleEnableScanCbk)
{
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    void *context = nullptr;
    uint16_t status = 0;
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE;
    DevdSleEnableScanCbk(context, status, cmdRes);
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE;
    DevdSleEnableScanCbk(context, status, cmdRes);
    EXPECT_EQ(DEVD_SCAN_MGR.status, DEVD_SLE_STATUS_IDLE);
}

TEST_F(DEVD_Test, TestDEVD_EnableAdv)
{
    uint8_t handle = DevdCreateAdvHandle(AdvCbk);
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    params->data.advData = (uint8_t *)SDF_MemZalloc(1);
    DevdSetAdvParam(params);
    DevdAdvNode_S *node = DevdGetAdvNode(handle, DEVD_ADV_LIST);
    NLSTK_DevdSetAdvData_S *data = (NLSTK_DevdSetAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvData_S));
    data->data.advData = (uint8_t *)SDF_MemZalloc(1);
    data->data.advDataLen = 1;
    data->data.scanRspData = (uint8_t *)SDF_MemZalloc(1);
    data->data.scanRspDataLen = 1;
    node->status = DEVD_SLE_STATUS_IDLE;
    node->param = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
    SDF_MemFree(node->tempParam);
    node->tempParam = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
    DevdSetAdvData(data);
    NLSTK_DevdSetAdvEnable_S *enable = (NLSTK_DevdSetAdvEnable_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvEnable_S));
    enable->advHandle = handle;
    enable->enable = 1;
    node->status = DEVD_SLE_STATUS_IDLE;
    DevdEnableAdv(enable);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_ENABLE_ADV);
    enable->enable = 2;
    node->status = DEVD_SLE_STATUS_IDLE;
    DevdEnableAdv(enable);
    EXPECT_EQ(node->status, DEVD_SLE_STATUS_DISABLE_ADV);

    DevdRemoveAdv(&handle);
    SDF_MemFree(enable);
    SDF_MemFree(data->data.scanRspData);
    SDF_MemFree(data->data.advData);
    SDF_MemFree(data);
    SDF_MemFree(params->data.advData);
    SDF_MemFree(params);
}

TEST_F(DEVD_Test, TestDEVD_EnableAdvCbk_Error)
{
    uint8_t handle = DevdCreateAdvHandle(AdvCbk);
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    params->param.handle = handle;
    params->data.advData = (uint8_t *)SDF_MemZalloc(1);
    DevdSetAdvParam(params);
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
    cbkContext->advHandle = handle;
    uint16_t status = 0;
    DevdSetAdvParamCbk(cbkContext, status, cmdRes);
    DevdSetAdvDataCbk(cbkContext, status, cmdRes);
    DevdEnableAdvCbk(cbkContext, status, cmdRes);
    DevdAdvNode_S *node = DevdGetAdvNode(cbkContext->advHandle, DEVD_ADV_LIST);

    EXPECT_EQ(node->status, DEVD_SLE_STATUS_IDLE);
    struct DLI_ExecuteCmdRetParam *cmdRes1 = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(
        sizeof(DLI_ExecuteCmdRetParam));
    cmdRes1->size = sizeof(DevdAdvTerminatedEvent_S);
    cmdRes1->eventParameter = SDF_MemZalloc(cmdRes1->size);
    DevdAdvTerminatedCbk(cbkContext, status, cmdRes1);
    DevdRemoveAdvCbk(cbkContext, status, cmdRes);
    SDF_MemFree(params->data.advData);
    SDF_MemFree(params);
    SDF_MemFree(cbkContext);
    SDF_MemFree(cmdRes1->eventParameter);
    SDF_MemFree(cmdRes1);
}

TEST_F(DEVD_Test, TestDEVD_SetAdvParamCbk_Error)
{
    uint8_t handle = DevdCreateAdvHandle(AdvCbk);
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    params->param.handle = handle;
    params->data.advData = (uint8_t *)SDF_MemZalloc(1);
    DevdSetAdvParam(params);
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
    cbkContext->advHandle = handle;
    uint16_t status = 1;
    DevdAdvNode_S *node = DevdGetAdvNode(cbkContext->advHandle, DEVD_ADV_LIST);
    SDF_MemFree(node->tempParam);
    node->tempParam = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
    DevdSetAdvParamCbk(cbkContext, status, cmdRes);

    SDF_MemFree(params->data.advData);
    SDF_MemFree(params);
    SDF_MemFree(cbkContext);
}
TEST_F(DEVD_Test, TestDEVD_SetAdvDataCbk_Error)
{
    uint8_t handle = DevdCreateAdvHandle(AdvCbk);
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    params->param.handle = handle;
    params->data.advData = (uint8_t *)SDF_MemZalloc(1);
    DevdSetAdvParam(params);
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
    cbkContext->advHandle = handle;
    uint16_t status = 1;
    DevdAdvNode_S *node = DevdGetAdvNode(cbkContext->advHandle, DEVD_ADV_LIST);
    SDF_MemFree(node->tempData->advData);
    SDF_MemFree(node->tempData);
    node->tempData = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
    DevdSetAdvDataCbk(cbkContext, status, cmdRes);

    SDF_MemFree(params->data.advData);
    SDF_MemFree(params);
    SDF_MemFree(cbkContext);
}

TEST_F(DEVD_Test, TestDEVD_FreeSetAdvData)
{
    NLSTK_DevdSetAdvData_S *data = (NLSTK_DevdSetAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvData_S));
    data->data.advData = (uint8_t *)SDF_MemZalloc(1);
    data->data.advDataLen = 1;
    data->data.scanRspData = (uint8_t *)SDF_MemZalloc(1);
    data->data.scanRspDataLen = 1;
    DevdFreeSetAdvData(data);
}

TEST_F(DEVD_Test, TestDEVD_FreeSetAdvParams)
{
    DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
    params->data.advData = (uint8_t *)SDF_MemZalloc(1);
    params->data.advDataLen = 1;
    params->data.scanRspData = (uint8_t *)SDF_MemZalloc(1);
    params->data.scanRspDataLen = 1;
    DevdFreeSetAdvParams(params);
}


TEST_F(DEVD_Test, TestDEVD_Error)
{
    DEVD_SCAN_MGR.scanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanParams_S));
    DEVD_SCAN_MGR.tmpScanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSleScanParams_S));
    void *context = nullptr;
    uint16_t status = 1;
    struct DLI_ExecuteCmdRetParam *cmdRes = nullptr;
    DevdAdvTerminatedCbk(context, status, cmdRes);
    cmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
    cmdRes->size = sizeof(DevdAdvTerminatedEvent_S);
    cmdRes->eventParameter = SDF_MemZalloc(cmdRes->size);
    DevdAdvTerminatedCbk(context, status, cmdRes);
    SDF_MemFree(cmdRes->eventParameter);
    SDF_MemFree(cmdRes);
}

TEST_F(DEVD_Test, TestDevdInit)
{
    DLI_Init();
    DevdInit();
    DevdDeInit();
    DLI_DeInit();
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS