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

#include <stddef.h>
#include "sdf_map.h"
#include "sdf_mem.h"

void SDF_MapDtor(SDF_Map *map)
{
    if (map == NULL) {
        return;
    }
    for (SDF_MapIter *cur = map->entry; cur != NULL;) {
        map->keyTraits.dtor(cur->key);
        if (cur->val) {
            map->valTraits.dtor(cur->val);
        }
        SDF_MapIter *next = cur->next;
        SDF_MemFree(cur);
        cur = next;
    }
    SDF_MemFree(map);
}

SDF_Map *SDF_MapCtor(SDF_Traits keyTraits, SDF_Traits valTraits)
{
    if (keyTraits.dtor == NULL || keyTraits.cmptor == NULL ||
        valTraits.dtor == NULL) {
        return NULL;
    }
    SDF_Map *map = SDF_MemZalloc(sizeof(SDF_Map));
    if (map == NULL) {
        return NULL;
    }
    *map = (SDF_Map) {
        .keyTraits = keyTraits,
        .valTraits = valTraits,
        .entry = NULL,
    };
    return map;
}

bool SDF_MapMoveInsert(SDF_Map *map, void *key, void *val)
{
    if (map == NULL || key == NULL || val == NULL || SDF_MapFind(map, key) != NULL) {
        return false;
    }
    SDF_MapIter *node = SDF_MemZalloc(sizeof(SDF_MapIter));
    if (node == NULL) {
        return false;
    }
    *node = (SDF_MapIter) {
        .key = key,
        .val = val,
        .next = map->entry,
    };
    map->entry = node;
    return true;
}

SDF_MapIter *SDF_MapFind(SDF_Map *map, void *key)
{
    if (map == NULL || key == NULL) {
        return NULL;
    }
    for (SDF_MapIter *cur = map->entry; cur != NULL; cur = cur->next) {
        if (map->keyTraits.cmptor(cur->key, key) == 0) {
            return cur;
        }
    }
    return NULL;
}

bool SDF_MapErase(SDF_Map *map, void *key)
{
    if (key == NULL || map == NULL) {
        return false;
    }
    SDF_MapIter fakeHead = { .next = map->entry };
    for (SDF_MapIter *cur = &fakeHead; cur->next != NULL; cur = cur->next) {
        if (map->keyTraits.cmptor(cur->next->key, key) == 0) {
            map->keyTraits.dtor(cur->next->key);
            if (cur->next->val) {
                map->valTraits.dtor(cur->next->val);
            }
            SDF_MapIter *next = cur->next->next;
            SDF_MemFree(cur->next);
            cur->next = next;
            map->entry = fakeHead.next;
            return true;
        }
    }
    return false;
}
