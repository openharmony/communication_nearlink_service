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
#ifndef TWS_SHARED_LIB_API_H
#define TWS_SHARED_LIB_API_H

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <cmath>
#include <cstring>
#include <mutex>

#include "SleInterfaceProfileTws.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "context.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

class TwsSharedLibApi {
public:
    explicit TwsSharedLibApi() : libHandle_(nullptr) {}
    ~TwsSharedLibApi() = default;

    static TwsSharedLibApi &GetInstance(); /* 单例运行 */
    bool Init();    /* 加载动态库，符号表初始化，回调初始化 */
    void DeInit();  /* 去初始化 */

    bool DecodeMessage(const TwsMessage event);
    bool EncodeMessage(const TwsMessage event);

    static void GetEncodeReqMsgResult(uint8_t *args, uint8_t *reqData, uint16_t dataLen);
    static void GetEncodeRspMsgResult(uint8_t *args, uint8_t *rspData, uint16_t dataLen);
    static void GetRecvReqMsgDecodeResult(uint8_t echoType, uint8_t *data, uint16_t dataLen, uint8_t *args);
    static void GetRecvRspMsgDecodeResult(uint8_t echoType, uint8_t *data, uint16_t dataLen, uint8_t *args);

    bool InitLibCallBack();

private:
    void *libHandle_ = nullptr;
    NearlinkSafeMap<std::string, void*> libFuncHandle_ {};     /* 函数名  函数指针 */

    template<typename FuncType>
    FuncType GetFuncHandleByName(const std::string& funcName);

    static void PostSendMsgEventToService(TwsMessage event, uint8_t *data, uint16_t dataLen);
    static void PostRecvMsgEventToService(TwsMessage event, uint8_t *data, uint16_t dataLen);
};

} // namespace Sle
} // namespace OHOS
#endif /* TWS_SHARED_LIB_API_H */