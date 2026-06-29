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

#ifndef NLSTK_REG_COLLAB_TRANS_EXT_H
#define NLSTK_REG_COLLAB_TRANS_EXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DLI_AcbNumChangeCbk)(uint16_t dataNum);

typedef struct {
    void (*setApBufferNum)(uint8_t bufferNum);
    uint8_t (*dliAcbNumGet)(void);
    void (*dliAcbNumChangeRegister)(DLI_AcbNumChangeCbk cbk);
} COLLAB_TransFuncExt;

#ifdef __cplusplus
}
#endif

#endif // NLSTK_REG_COLLAB_TRANS_EXT_H