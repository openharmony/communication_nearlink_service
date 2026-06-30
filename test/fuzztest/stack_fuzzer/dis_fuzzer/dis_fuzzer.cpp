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

#include "dis_fuzzer.h"
#include "securec.h"

#include "nai_log.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "sdf_worker.h"
#include "sdf_timer.h"
#include "cp_worker.h"

#include "dis_stm.h"
#include "nlstk_dis_client.h"
#include "nlstk_dis_server.h"

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN     = 7,
    FUZZ_EIGHT     = 8,
    FUZZ_SIXTEEN     = 16,
    FUZZ_TWENTY     = 20,
    FUZZ_TWENTYFOUR     = 24,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

// mock func
extern "C" uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return NLSTK_OK;
}

extern "C" uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return NLSTK_OK;
}

extern "C" uint32_t ScheduleEnable(void)
{
    return NLSTK_OK;
}

extern "C" void ScheduleDisable(void)
{
    return;
}

extern "C" uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param)
{
    (void)handle;
    (void)param;
    return CP_OK;
}

extern "C" void ScheduleTimerDel(int handle)
{
    (void)handle;
}

namespace OHOS {
    void Mock_DisConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errNumb)
    {
        (void)addr;
        (void)curState;
        (void)prevState;
    }

    void FuzzDisClientApi(uint8_t* data, size_t size)
    {
        if (size < sizeof(SLE_Addr_S) || size < FUZZ_FOUR) {
            return;
        }
        NLSTK_DisClientCbk_S clientCallback = {0};
        clientCallback.stateChangeCbk = Mock_DisConnectStateChangeCbk;
        NLSTK_DisRegisterCallbBack(&clientCallback);

        SLE_Addr_S addr = {0};
        (void)memcpy_s(&addr, sizeof(SLE_Addr_S), data, sizeof(SLE_Addr_S));
        NLSTK_DisProfileConnect(&addr);
        NLSTK_DisProfileDisconnect(&addr);

        NLSTK_DisInfoType_E type = (NLSTK_DisInfoType_E)(data[FUZZ_ZERO]);
        NLSTK_VariableData_S outData = {0};
        outData.data = (uint8_t *)SDF_MemZalloc(DIS_MAX_VAR_LEN);
        if (outData.data == nullptr) {
            return;
        }
        NLSTK_DisReadInfo(&addr, type, &outData);
        uint32_t appearance = (data[FUZZ_ZERO] << FUZZ_TWENTYFOUR) | (data[FUZZ_ONE] << FUZZ_SIXTEEN) |
            (data[FUZZ_TWO] << FUZZ_EIGHT) | (data[FUZZ_THREE]);
        NLSTK_DisReadAppearanceInfo(&addr, &appearance);
        NLSTK_GetConnectedDeviceNum(data);

        SDF_MemFree(outData.data);
    }

    void AllocAndFillDevInfo(NLSTK_VariableData_S *devInfoValue, uint8_t *data)
    {
        if (devInfoValue == nullptr || data == nullptr) {
            return;
        }
        devInfoValue->len = FUZZ_ONE;
        devInfoValue->data = (uint8_t *)SDF_MemZalloc(FUZZ_ONE);
        if (devInfoValue->data == nullptr) {
            return;
        }
        if (memcpy_s(devInfoValue->data, FUZZ_ONE, data, FUZZ_ONE) != EOK) {
            SDF_MemFree(devInfoValue->data);
            return;
        }
    }

    void AllocDisDevInfo(NLSTK_DeviceInfo_S *devInfo, uint8_t *data)
    {
        AllocAndFillDevInfo(&(devInfo->manufacturerInfo), data);
        AllocAndFillDevInfo(&(devInfo->deviceModel), data);
        AllocAndFillDevInfo(&(devInfo->deviceSerialNumber), data);
        AllocAndFillDevInfo(&(devInfo->hardwareVersion), data);
        AllocAndFillDevInfo(&(devInfo->firmwareVersion), data);
        AllocAndFillDevInfo(&(devInfo->softwareVersion), data);
        AllocAndFillDevInfo(&(devInfo->deviceLocalAlias), data);
    }

    void FreeDisDevInfo(NLSTK_DeviceInfo_S *devInfo)
    {
        if (devInfo == nullptr) {
            return;
        }
        if (devInfo->manufacturerInfo.data != nullptr) {
            SDF_MemFree(devInfo->manufacturerInfo.data);
        }
        if (devInfo->deviceModel.data != nullptr) {
            SDF_MemFree(devInfo->deviceModel.data);
        }
        if (devInfo->deviceSerialNumber.data != nullptr) {
            SDF_MemFree(devInfo->deviceSerialNumber.data);
        }
        if (devInfo->hardwareVersion.data != nullptr) {
            SDF_MemFree(devInfo->hardwareVersion.data);
        }
        if (devInfo->firmwareVersion.data != nullptr) {
            SDF_MemFree(devInfo->firmwareVersion.data);
        }
        if (devInfo->softwareVersion.data != nullptr) {
            SDF_MemFree(devInfo->softwareVersion.data);
        }
        if (devInfo->deviceLocalAlias.data != nullptr) {
            SDF_MemFree(devInfo->deviceLocalAlias.data);
        }
    }

    void FuzzDisServerApi(uint8_t* data, size_t size)
    {
        if (size < FUZZ_ONE) {
            return;
        }
        NLSTK_DeviceInfo_S devInfo = {{0}};
        AllocDisDevInfo(&devInfo, data);
        NLSTK_DisCreateInstance(&devInfo);
        NLSTK_VariableData_S name = {0};
        name.len = FUZZ_ONE;
        name.data = (uint8_t *)SDF_MemZalloc(FUZZ_ONE);
        if (name.data == nullptr) {
            FreeDisDevInfo(&devInfo);
            return;
        }
        if (memcpy_s(name.data, FUZZ_ONE, data, FUZZ_ONE) != EOK) {
            SDF_MemFree(name.data);
            FreeDisDevInfo(&devInfo);
            return;
        }
        NLSTK_DisUpdateLocalDeviceName(&name);
        NLSTK_DisDestroyInstance();
        SDF_MemFree(name.data);
        FreeDisDevInfo(&devInfo);
    }

    void FuzzDisApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzDisClientApi(data, size);
        FuzzDisServerApi(data, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    if (NLSTK_IsStackInited()) {
        NLSTK_DisableStack();
        NLSTK_DeinitStack();
    }
    NLSTK_InitStack();
    NLSTK_EnableStack();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzDisApi(fuzzData, size);
    free(fuzzData);
    return 0;
}