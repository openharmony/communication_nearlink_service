/****************************************************************************
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
****************************************************************************/

#include "cm_signaling_version.h"
#include "sle_logic_link_mgr.h"
#include "cm_log.h"
#include "cm_def.h"
#include "cm_signaling_internal.h"

#define COMPANY_ID_CY      0x006C  /* CY company ID */
#define COMPANY_ID_HUAWEI  0x0009  /* CompanyNameMagicTag company ID */
#define COMPANY_ID_HISI    0x007C  /* CompanyNameMagicTag */

#define TRANS_MODE_MASK    0x0001
#define FLOW_MODE_BIT_SHIFT     1
#define BYPASS_MODE_BIT_SHIFT   2

void CM_SetLinkProtocolVersion(uint16_t connId, uint16_t protocolVersion)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    link->protocolVersion = protocolVersion;
}

void CM_SetLinkRxWindow(uint16_t connId, uint8_t rxWindow)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    link->rxWnd = rxWindow;
}

void CM_SetLinkTransMode(uint16_t connId, CM_TransMode mode)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    link->supportTransMode = ((mode.reliableMode & TRANS_MODE_MASK) |
        ((mode.flowMode & TRANS_MODE_MASK) << FLOW_MODE_BIT_SHIFT) |
        ((mode.byPassMode & TRANS_MODE_MASK) << BYPASS_MODE_BIT_SHIFT));
}

void CM_SetLinkMtu(uint16_t connId, uint16_t mtu)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    link->mtu = mtu;
}

void CM_SetLinkExchangeVersion(uint16_t connId, uint16_t exchangeVersion)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    link->exchangeVersion = exchangeVersion;
}

uint16_t CM_GetDeviceLinkDeviceType(uint16_t connId)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN_RET(link != NULL, CM_DEVTYPE_UNKNOWN, "SleLogicLinkGetByLcid failed");
    CM_LOGD("link companyId:%hu, devType:%hu", link->companyId, link->devType);
    if (link->devType != CM_DEVTYPE_UNKNOWN) {
        return link->devType;
    }

    /* 不确定时判断第三方 */
    if ((link->companyId != COMPANY_ID_CY) &&
        (link->companyId != COMPANY_ID_HUAWEI) &&
        (link->companyId != COMPANY_ID_HISI)) {
        link->devType = CM_DEVTYPE_THIRD_PARTY;
        return link->devType;
    }
    /* 未收到对端响应，认为是hisi old */
    return CM_DEVTYPE_OLD;
}

void CM_SetDeviceLinkDeviceType(uint16_t connId, bool hasVerBit)
{
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connId);
    CM_CHECK_RETURN(link != NULL, "SleLogicLinkGetByLcid failed");
    CM_LOGI("link companyId:%hu, devType:%hu, hasVerBit:%d, protocolVersion:%hu",
        link->companyId, link->devType, hasVerBit, link->protocolVersion);
    if (link->devType >= CM_DEVTYPE_OLD) {
        /* 当前为确认态，不进行修改 */
        return;
    }

    /* 确认是三方设备 */
    if ((link->companyId != COMPANY_ID_CY) &&
        (link->companyId != COMPANY_ID_HUAWEI) &&
        (link->companyId != COMPANY_ID_HISI)) {
        link->devType = CM_DEVTYPE_THIRD_PARTY;
        return;
    }

    if ((hasVerBit) || (link->protocolVersion != CM_INVALID_VERSION)) {
        /* 收到对端version响应 */
        link->devType = CM_DEVTYPE_NEW;
        return;
    }
    link->devType = CM_DEVTYPE_OLD;
}
