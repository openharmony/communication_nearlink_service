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

#ifndef SDF_MAP_H
#define SDF_MAP_H

#include <stdbool.h>
#include "sdf_traits.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void SDF_MapDtor(SDF_Map *map);
SDF_Map *SDF_MapCtor(SDF_Traits keyTraits, SDF_Traits valTraits);

bool SDF_MapMoveInsert(SDF_Map *map, void *key, void *val);
SDF_MapIter *SDF_MapFind(SDF_Map *map, void *key);
bool SDF_MapErase(SDF_Map *map, void *key);

#ifdef __cplusplus
}
#endif

#endif /* SDF_MAP_H */
