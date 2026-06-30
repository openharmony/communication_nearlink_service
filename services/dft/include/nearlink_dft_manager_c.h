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

#ifndef NEARLINK_DFT_MANAGER_C_H
#define NEARLINK_DFT_MANAGER_C_H

#include "nearlink_dft_database.h"
#include "hisysevent_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DftParamC DftParamC;

// structure of sub event reference
typedef struct DftSubEventRefC {
    DftEventEnum eventId;
    DftParamC *params;
    size_t size;
} DftSubEventRefC;

// structure of dft event param value
typedef union DftParamValueUnionC {
    HiSysEventParamValue v;
    DftSubEventRefC *ref;
} DftParamValueUnionC;

// structure of dft event param
struct DftParamC {
    int paramId;
    DftParamType t;
    DftParamValueUnionC u;
};

/**
 * Start dft manager, init ffrt module
 * The report and cache interfaces can be valid only after this.
 */
void DftManagerStart(void);

/**
 * Stop dft manager, deinit ffrt module
 */
void DftManagerStop(void);

/**
 * Change HiviewUe state
 */
void DftHiviewUeStateChanged(bool isHiviewUeOn);

/**
 * report dft event
 * Will finds the corresponding event cache
 * based on the key param in the params
 * and reports all params and remove exception event cache
 * @param eventId event Id of exception event
 * @param params  params array(key params inside)
 * @param size len of params
 */
void DftManagerReport(DftEventEnum eventId, const DftParamC params[], size_t size);

/**
 * cache dft event param
 * Will save params into cache
 * @param eventId event Id
 * @param params params array(key params inside)
 * @param size len of params
 */
void DftManagerCache(DftEventEnum eventId, const DftParamC params[], size_t size);

/**
 * erase cache param by eventId and key params
 * @param eventId event Id
 * @param keyParams params array(key params)
 * @param size len of key params
 */
void DftManagerEraseCache(DftEventEnum eventId, const DftParamC keyParams[], size_t size);
#ifdef __cplusplus
}
#endif

#endif // NEARLINK_DFT_MANAGER_C_H