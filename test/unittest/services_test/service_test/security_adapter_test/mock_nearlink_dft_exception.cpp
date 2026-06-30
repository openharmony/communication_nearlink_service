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

#include "nearlink_dft_exception.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

void DftCacheDisconChipInfo(const std::string &addr, int32_t rssi, std::string channelNoise)
{
    HILOGI("mock DftCacheDisconChipInfo");
}

void DftCachePeerInfoTime(const std::string &addr, DftPeerInfoParamEnum paramId)
{
    HILOGI("mock DftCachePeerInfoTime");
}

void DftCacheAcbFinishConn(const std::string &addr, uint32_t result, int32_t reason, uint32_t count, uint16_t lcid)
{
    HILOGI("mock DftCacheAcbFinishConn");
}

void DftCacheSecurityPairType(const std::string &addr, bool isSecurityPairType)
{
    HILOGI("mock DftCacheSecurityPairType");
}

void DftCacheCdsmInfo(const std::string &device, const std::string &reportAddr, const std::string &otherAddr)
{
    HILOGI("mock DftCacheCdsmInfo");
}

void DftCachePairConnType(const std::string &addr, uint32_t pairType, uint32_t connType)
{
    HILOGI("mock DftCachePairConnType");
}

void DftCacheDisconnInfoMsg(const std::string &addr, const std::string &infoMsg, uint16_t secne)
{
    HILOGI("mock DftCacheDisconnInfoMsg");
}

void DftCacheUnPairInfo(const std::string &addr, int32_t deviceAppearance,
    const std::string &callingName, int32_t state, int32_t pairState)
{
    HILOGI("mock DftCacheUnPairInfo");
}

void DftCacheAcbDisConn(const std::string &addr, const std::string &callingName)
{
    HILOGI("mock DftCacheAcbDisConn");
}

void DftCachePeerInfo(const std::string &addr, const std::string &name, int32_t appearance)
{
    HILOGI("mock DftCachePeerInfo");
}

void DftCachePairConnTime(const std::string &addr, uint32_t currentConnPath, uint32_t pairTimeType)
{
    HILOGI("mock DftCachePairConnTime");
}

void DftCacheCallingName(const std::string &addr, const std::string &callingName)
{
    HILOGI("mock DftCacheCallingName");
}

void DftCacheBgStartConn(const std::string &addr, int32_t deviceAppearance)
{
    HILOGI("mock DftCacheBgStartConn");
}

void DftCacheAcbStartConn(const std::string &addr)
{
    HILOGI("mock DftCacheAcbStartConn");
}
}  // namespace Nearlink
}  // namespace OHOS