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

#ifndef QOSM_H
#define QOSM_H

#include <list>
#include "nearlink_ASC_source.h"
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {
enum Qos : uint8_t {
    NL_SLE_QOS_NONE        = 0,
    NL_SLE_QOS_1           = 1,
    NL_SLE_QOS_2           = 2,
    NL_SLE_QOS_3           = 3,
    NL_SLE_QOS_4           = 4,    // 空间音频头动模式
    NL_SLE_QOS_5           = 5,
    NL_SLE_QOS_6           = 6,
    NL_SLE_QOS_7           = 7,
    NL_SLE_QOS_8           = 8,    // 空间音频固定模式(在service层使用，给stack底层配置qos1)
    NL_SLE_QOS_9           = 9,    // K歌
    NL_SLE_QOS_10          = 10,   // 移动全景音模式
    NL_SLE_QOS_BUTT        = 11,
};

// QOS的能力定义
typedef struct {
    Qos qos;
    uint8_t priority;         // 优先级（越大越高）
    bool isImmediatelyStop;   // 立即结束
    bool isUpRequire;         // 需要上行
    bool isDownRequire;       // 需要下行
} QosDefineStru;

// 设备的Qos
typedef struct {
    Qos cos;
    std::list<Qos> gos;
} DevQosmStru;

class QosM {
public:
    static QosM& GetInstance();
    explicit QosM();
    virtual ~QosM();

    Qos GetCOS(const RawAddress &device);
    Qos CalculateNos(const RawAddress &device);
    void SetCos(const RawAddress &device, Qos nos);
    bool AddQos(const RawAddress &device, Qos qos);
    bool JudgeQosPriority(const RawAddress &device, DevQosmStru& qosm, Qos nos);
    bool DeleteQos(const RawAddress &device, Qos qos, bool& isStopStream, bool& isStopImmediately);
    void ClearQosM(const RawAddress &device);
    bool IsQos4Exist(const RawAddress &device);
    bool IsQosExist(const RawAddress &device, Qos qos);
    void SyncQos(const RawAddress& device, const RawAddress& coSetDevice);
    void ClearQosIfStartFailed(const RawAddress& device, Qos qos);
    uint8_t GetQosPriority(Qos qos);
private:
    void Init();
    bool IsStopImmediately(const RawAddress &device, Qos nos);
    bool IsUpExist(const RawAddress &device);
    bool IsSingleDownExist(const RawAddress &device);
    void PrintGOS(const RawAddress &device, const std::list<Qos>& list);
    void DeleteQosFromGos(const RawAddress &device, std::list<Qos>& gosList, Qos qos);
    void CheckAndDeleteQos(const RawAddress &device, Qos qosIn);
    bool IsAudioSceneCall();
    Qos UpdateNosAfterDeleteQos(const RawAddress& device, Qos cos);
    std::map<std::string, DevQosmStru> deviceQosmMap_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // QOSM_H