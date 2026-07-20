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

#include "QosM.h"
#include "log_util.h"
#include "audio_system_client_policy_manager.h"

namespace OHOS {
namespace Nearlink {
QosDefineStru g_qosDefineTable[NL_SLE_QOS_BUTT] = {
    /* qos                 优先级（越大越高）    立即结束     需要上行    需要下行 */
    {NL_SLE_QOS_NONE,    0,                false,      false,      false},
    {NL_SLE_QOS_1,       1,                false,      false,      true},
    {NL_SLE_QOS_2,       3,                false,      false,      true},
    {NL_SLE_QOS_3,       5,                false,      true,       true},
    {NL_SLE_QOS_4,       2,                false,      false,      true},
    {NL_SLE_QOS_5,       4,                true,       true,       false},
    {NL_SLE_QOS_6,       5,                true,       true,       true},
    {NL_SLE_QOS_7,       1,                false,      false,      true},
    {NL_SLE_QOS_8,       2,                false,      false,      true},
    {NL_SLE_QOS_9,       3,                false,      true,       true},
    {NL_SLE_QOS_10,      3,                false,      false,      true},
};

QosM::QosM()
{
    Init();
}

QosM::~QosM()
{
}

void QosM::Init()
{
    HILOGD("enter Init");
}

QosM& QosM::GetInstance()
{
    static QosM config;
    return config;
}

void QosM::ClearQosM(const RawAddress &device)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]ClearQosM No Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    DevQosmStru& qosm = it->second;
    qosm.cos = NL_SLE_QOS_NONE;
    if (qosm.gos.empty()) {
        HILOGI("[ASCService]ClearQosM %{public}s GOS empty", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    qosm.gos.clear();
    HILOGI("[ASCService]ClearQosM %{public}s GOS cleared", GetEncryptAddr(device.GetAddress()).c_str());
    return;
}

void QosM::ClearQosIfStartFailed(const RawAddress &device, Qos qos)
{
    // 起播失败qos删除
    bool isStopStream = false;
    bool isStopImmediately = false;
    DeleteQos(device, qos, isStopStream, isStopImmediately);

    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService] No Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    DevQosmStru& qosm = it->second;
    std::list<Qos>& gosList = it->second.gos;
    if (!gosList.empty()) {
        HILOGI("[ASCService] %{public}s GOS not empty", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    qosm.cos = NL_SLE_QOS_NONE;
    HILOGI("[ASCService] %{public}s GOS empty, clear qos %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos);
    return;
}

Qos QosM::GetCOS(const RawAddress &device)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it != deviceQosmMap_.end()) {
        HILOGI("[ASCService]GetCOS %{public}s COS %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            it->second.cos);
        return it->second.cos;
    }

    HILOGD("[ASCService]GetCOS %{public}s NO Item COS %{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
        NL_SLE_QOS_NONE);
    return NL_SLE_QOS_NONE;
}

void QosM::SetCos(const RawAddress &device, Qos nos)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService] No Item %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    if (nos >= NL_SLE_QOS_BUTT) {
        return;
    }

    DevQosmStru& qosm = it->second;
    qosm.cos = nos;
    HILOGI("[ASCService] %{public}s Group of Current QOS %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), nos);
}

void QosM::PrintGOS(const RawAddress &device, const std::list<Qos>& list)
{
    std::ostringstream oss;
    for (const auto& qos : list) {
        oss << static_cast<int>(qos) << " ";
    }
    std::string gos = oss.str();
    HILOGI("[ASCService]PrintGOS %{public}s Group of Current QOS %{public}s",
        GetEncryptAddr(device.GetAddress()).c_str(), gos.c_str());
}

bool QosM::IsStopImmediately(const RawAddress &device, Qos nos)
{
    Qos cos = GetCOS(device);
    if (nos == NL_SLE_QOS_NONE) {
        const QosDefineStru& qosDefine = g_qosDefineTable[cos];
        if (qosDefine.isImmediatelyStop) {
            HILOGI("[ASCService]IsStopImmediately true %{public}s NOS %{public}d COS %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), nos, cos);
            return true;
        }
    }

    HILOGI("[ASCService]IsStopImmediately false %{public}s NOS %{public}d COS %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), nos, cos);
    return false;
}

bool QosM::IsQos4Exist(const RawAddress &device)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]IsQos4Exist false %{public}s No Qosm item",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]IsQos4Exist false %{public}s GOS empty",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    bool isQos4Exist = false;
    for (const Qos& qos : gosList) {
        if (qos == NL_SLE_QOS_4) {
            isQos4Exist = true;
            break;
        }
    }

    HILOGI("[ASCService]IsQos4Exist %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isQos4Exist);
    return isQos4Exist;
}

bool QosM::IsQosExist(const RawAddress &device, Qos qos)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]IsQosExist false %{public}s No Qosm item",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]IsQosExist false %{public}s GOS empty",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    bool isQosExist = false;
    for (const Qos& qosInner : gosList) {
        if (qosInner == qos) {
            isQosExist = true;
            break;
        }
    }

    HILOGI("[ASCService]IsQosExist %{public}s %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), isQosExist);
    return isQosExist;
}

bool QosM::AddQos(const RawAddress &device, Qos qos)
{
    if (qos >= NL_SLE_QOS_BUTT) {
        HILOGE("[ASCService]AddQos, qos(%{public}d).", qos);
        return false;
    }

    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        // 创建并插入map
        DevQosmStru qosm {};
        qosm.cos = NL_SLE_QOS_NONE;
        deviceQosmMap_[device.GetAddress()] = qosm;
    }

    std::list<Qos>& gosList = deviceQosmMap_[device.GetAddress()].gos;
    gosList.emplace_back(qos);
    HILOGI("[ASCService]AddQos %{public}s, qos %{public}d", GetEncryptAddr(device.GetAddress()).c_str(), qos);
    // 打印GOS
    PrintGOS(device, gosList);

    // NOS
    Qos nos = NL_SLE_QOS_NONE;
    // 规则五：QOS添加/删除后，NOS为GOS里优先级最高的QOS（规则三，四优先级更高）
    nos = CalculateNos(device);

    DevQosmStru& qosm = deviceQosmMap_[device.GetAddress()];
    // 规则十：QOS添加后，NOS为4的时候，如果是GOS没有2，COS是2的时候，替换之
    if ((qosm.cos == NL_SLE_QOS_2) && (nos == NL_SLE_QOS_4) && !IsQosExist(device, NL_SLE_QOS_2)) {
        HILOGI("[ASCService]AddQos replace COS %{public}s, Qos4 replace Qos2, COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        qosm.cos = nos;
        return true;
    }

    bool isPriorityChanged = JudgeQosPriority(device, qosm, nos);
    if (isPriorityChanged) {
        return true;
    }

    // 通话延迟断链中起播非通话业务(媒体/游戏)，需要重配置链路。通话延迟断链中起播通话业务，不需要重配置链路
    if ((qosm.cos == NL_SLE_QOS_3) && (qos != NL_SLE_QOS_3) && !IsQosExist(device, NL_SLE_QOS_3)) {
        HILOGI("[ASCService]AddQos replace COS %{public}s, Qos3 delay stopping, COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        qosm.cos = nos;
        return true;
    }

    HILOGI("[ASCService]AddQos %{public}s, qos %{public}d COS %{public}d NOS %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), qos, qosm.cos, nos);
    return false;
}

bool QosM::JudgeQosPriority(const RawAddress &device, DevQosmStru& qosm, Qos nos)
{
    uint8_t cosPriority = GetQosPriority(qosm.cos);
    uint8_t nosPriority = GetQosPriority(nos);
    if (cosPriority > nosPriority) {
        return false;
    }

    // 规则六：QOS添加后，NOS优先级高于COS，替换之，否则无变化
    if (cosPriority < nosPriority) {
        HILOGI("JudgeQosPriority replace COS %{public}s, COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        qosm.cos = nos;
        return true;
    }

    // 规则十一：空间音频切换场景(8和4相互切换)，优先级相同，需要返回true
    if ((qosm.cos == NL_SLE_QOS_4 && nos == NL_SLE_QOS_8) || (qosm.cos == NL_SLE_QOS_8 && nos == NL_SLE_QOS_4)) {
        HILOGI("JudgeQosPriority %{public}s COS %{public}d equals NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        return true;
    }
    return false;
}

bool QosM::IsAudioSceneCall()
{
    AudioStandard::AudioScene scene = AudioStandard::AudioSystemClientPolicyManager::GetInstance().GetAudioScene();
    HILOGI("[ASCService]IsAudioSceneCall scene %{public}d", scene);

    return ((scene == AudioStandard::AudioScene::AUDIO_SCENE_RINGING) ||
        (scene == AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CALL) ||
        (scene == AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CHAT));
}

uint8_t QosM::GetQosPriority(Qos qos)
{
    if (qos >= NL_SLE_QOS_BUTT) {
        HILOGE("[ASCService]GetQosPriority, qos(%{public}d).", qos);
        return 0; // invalid qos, return lowest priority
    }

    const QosDefineStru& qosDefine = g_qosDefineTable[qos];
    HILOGI("[ASCService]GetQosPriority qos %{public}d priority %{public}d", qos, qosDefine.priority);
    return qosDefine.priority;
}

Qos QosM::CalculateNos(const RawAddress &device)
{
    Qos nos = NL_SLE_QOS_NONE;
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]CalculateNos %{public}s No Qosm item, NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), nos);
        return nos;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]CalculateNos %{public}s GOS empty, NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), nos);
        return nos;
    }

    // 查优先级表决定NOS
    uint8_t priorityMax = 0;
    for (const Qos& qos : gosList) {
        uint8_t priority = GetQosPriority(qos);
        if (priority > priorityMax) {
            priorityMax = priority;
            nos = qos;
        }
    }

    HILOGI("[ASCService]CalculateNos %{public}s NOS %{public}d priorityMax %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), nos, priorityMax);
    return nos;
}

bool QosM::IsUpExist(const RawAddress &device)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]IsUpExist false %{public}s No Qosm item", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]IsUpExist false %{public}s GOS empty", GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    for (const Qos& qos : gosList) {
        const QosDefineStru& qosDefine = g_qosDefineTable[qos];
        if (qosDefine.isUpRequire) {
            HILOGI("[ASCService]IsUpExist true %{public}s, qos %{public}d Exists",
                GetEncryptAddr(device.GetAddress()).c_str(), qos);
            return true;
        }
    }

    HILOGI("[ASCService]IsUpExist false %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    return false;
}

bool QosM::IsSingleDownExist(const RawAddress &device)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]IsSingleDownExist false %{public}s No Qosm item",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]IsSingleDownExist false %{public}s GOS empty",
            GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }

    for (const Qos& qos : gosList) {
        const QosDefineStru& qosDefine = g_qosDefineTable[qos];
        if (!qosDefine.isUpRequire && qosDefine.isDownRequire) {
            HILOGI("[ASCService]IsSingleDownExist true %{public}s, qos %{public}d Exists",
                GetEncryptAddr(device.GetAddress()).c_str(), qos);
            return true;
        }
    }

    HILOGI("[ASCService]IsSingleDownExist false %{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    return false;
}

void QosM::CheckAndDeleteQos(const RawAddress &device, Qos qosIn)
{
    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]CheckAndDeleteQos4 %{public}s No Qosm item",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]CheckAndDeleteQos4 %{public}s GOS empty",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    bool isMediaExist = false;
    for (const Qos& qos : gosList) {
        if (GetQosPriority(qos) < GetQosPriority(qosIn)) {
            isMediaExist = true;
            break;
        }
    }

    if (isMediaExist) {
        HILOGI("[ASCService]CheckAndDeleteQos4 %{public}s Media Exist",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    // 删除qos4/8
    DeleteQosFromGos(device, gosList, qosIn);

    return;
}

void QosM::DeleteQosFromGos(const RawAddress &device, std::list<Qos>& gosList, Qos qos)
{
    // 删除qos
    for (auto itGos = gosList.begin(); itGos != gosList.end();) {
        if (*itGos == qos) {
            itGos = gosList.erase(itGos);
            HILOGI("[ASCService]DeleteQosFromGos %{public}s deleted %{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), qos);
            break;
        } else {
            ++itGos;
        }
    }
}

bool QosM::DeleteQos(const RawAddress &device, Qos qos, bool& isStopStream, bool& isStopImmediately)
{
    if (qos >= NL_SLE_QOS_BUTT) {
        HILOGE("[ASCService]DeleteQos, qos(%{public}d).", qos);
        return false;
    }

    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        HILOGI("[ASCService]DeleteQos %{public}s No Qosm item, qos %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qos);
        return false;
    }

    std::list<Qos>& gosList = it->second.gos;
    if (gosList.empty()) {
        HILOGI("[ASCService]DeleteQos %{public}s GOS empty, qos %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qos);
        return false;
    }

    // 删除qos
    DeleteQosFromGos(device, gosList, qos);

    // 媒体全停播时Qos4/8一并删除
    if (IsQos4Exist(device)) {
        CheckAndDeleteQos(device, NL_SLE_QOS_4);
    }
    if (IsQosExist(device, NL_SLE_QOS_8)) {
        CheckAndDeleteQos(device, NL_SLE_QOS_8);
    }
    // 主动删除Qos10和上行流时，不需要重复删除，避免影响其他流
    if ((qos != NL_SLE_QOS_10 && qos != NL_SLE_QOS_3) && IsQosExist(device, NL_SLE_QOS_10)) {
        CheckAndDeleteQos(device, NL_SLE_QOS_10);
    }

    // 打印GOS
    PrintGOS(device, gosList);

    // NOS
    DevQosmStru& qosm = deviceQosmMap_[device.GetAddress()];
    Qos nos = UpdateNosAfterDeleteQos(device, qosm.cos);

    // 规则一：NOS为0时，延迟结束的COS会保持3.5S的同步链路
    // 规则二：NOS为0时，立即结束的COS会立即结束同步链路
    if (nos == NL_SLE_QOS_NONE) {
        isStopStream = true;
        isStopImmediately = IsStopImmediately(device, nos);
    }

    // 规则七：QOS删除后，NOS优先级小于COS，替换之，否则无变化
    if ((GetQosPriority(qosm.cos) > GetQosPriority(nos)) && (nos != NL_SLE_QOS_NONE)) {
        HILOGI("[ASCService]DeleteQos replace COS %{public}s, COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        qosm.cos = nos;
        return true;
    }

    // 规则十二：QOS 10删除后，NOS更新cos
    if (qosm.cos == NL_SLE_QOS_10) {
        HILOGI("[ASCService]DeleteQos10 COS %{public}s, COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qosm.cos, nos);
        qosm.cos = nos;
        return false;
    }

    HILOGI("[ASCService]DeleteQos %{public}s, qos %{public}d COS %{public}d NOS %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), qos, qosm.cos, nos);
    return false;
}

Qos QosM::UpdateNosAfterDeleteQos(const RawAddress& device, Qos cos) {
    // NOS
    Qos nos = NL_SLE_QOS_NONE;

    // 规则四：QOS删除后，如果COS是3，GOS里面依然存在上行的业务，NOS=QOS3
    if ((cos == NL_SLE_QOS_3) && IsUpExist(device)) {
        nos = NL_SLE_QOS_3;
    } else if ((cos == NL_SLE_QOS_2) && IsSingleDownExist(device)) {
        // 规则九：QOS删除后，如果COS是2，GOS里面依然存在单下行的业务，NOS=QOS2
        nos = NL_SLE_QOS_2;
    } else {
        // 规则五：QOS添加/删除后，NOS为GOS里优先级最高的QOS（规则三，四优先级更高）
        nos = CalculateNos(device);
    }
    return nos;
}

void QosM::SyncQos(const RawAddress& device, const RawAddress& coSetDevice)
{
    std::map<std::string, DevQosmStru>::iterator itCo = deviceQosmMap_.find(coSetDevice.GetAddress());
    if (itCo == deviceQosmMap_.end()) {
        HILOGI("[ASCService]SyncQos %{public}s No Qosm item", GetEncryptAddr(coSetDevice.GetAddress()).c_str());
        return;
    }

    std::list<Qos>& coGosList = itCo->second.gos;
    if (coGosList.empty()) {
        HILOGI("[ASCService]SyncQos %{public}s GOS empty", GetEncryptAddr(coSetDevice.GetAddress()).c_str());
        return;
    }

    std::map<std::string, DevQosmStru>::iterator it = deviceQosmMap_.find(device.GetAddress());
    if (it == deviceQosmMap_.end()) {
        // 创建并插入map
        DevQosmStru qosm {};
        qosm.cos = NL_SLE_QOS_NONE;
        deviceQosmMap_[device.GetAddress()] = qosm;
    }

    std::list<Qos>& gosList = deviceQosmMap_[device.GetAddress()].gos;
    for (auto itCoGos = coGosList.rbegin(); itCoGos != coGosList.rend(); ++itCoGos) {
        gosList.emplace_back(*itCoGos);
    }

    // 打印GOS
    PrintGOS(coSetDevice, coGosList);
    PrintGOS(device, gosList);

    deviceQosmMap_[device.GetAddress()].cos = deviceQosmMap_[coSetDevice.GetAddress()].cos;
}

} // namespace Nearlink
} // namespace OHOS
