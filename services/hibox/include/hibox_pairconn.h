/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef HIBOX_PAIRCONN
#define HIBOX_PAIRCONN

#include "hibox_def.h"

#define CACEL_PAIR_LEN 1
#define QUERY_CONN_LEN 1
#define SEC_ADV_LEVEL_LEN 1

#define SDP_AUDIO_MODE_LEN 1
#define SDP_AUDIO_PROFILE_LEN 4
#define SDP_AUDIO_PRODUCT_LEN 1
#define SDP_AUDIO_VERSION_LEN 2
#define SDP_AUDIO_CAP_LEN 1

#define EARBUDS_NATURE_LEN 1
#define HONGMENG_OS_TYPE_LEN 1
#define QUERY_BUSINESS_CONN_TYPE_LEN 1
#define MOBILE_BUSINESS_LEN 1
#define AUTO_CONN_SWITCH_LEN 1
#define NOTIFY_DISCONNECT_PROFILE_LEN 1
#define SLE_ALL_PROFILE_CONNECTED_LEN 1

enum HiboxPairConnCommandId {
    CMDID_AUDIO_SDP = 0x01,
    CMDID_MOBILE_CONN_PARA = 0x02,
    CMDID_CACEL_PAIR = 0x05,
    CMDID_EARBUDS_NATURE = 0x08,
    CMDID_QUERY_CONN = 0x0A,
    CMDID_SECURITY_ADV = 0x0B,
    CMDID_VENDOR_ACCOUNT_HASH = 0x0D,
    CMDID_AUTO_CONN_SWITCH = 0x0E,
    CMDID_PERIPHERAL_MAC_UPDATE = 0x10,
    CMDID_NOTIFY_SLE_DISCONNECT_PROFILE = 0x11,
    CMDID_SLE_ALL_PROFILE_CONNECTED = 0x12,
};

enum HiboxCacelPairType {
    CACEL_PAIR_TYPE = 0x01,
};

enum HiboxQueryConnType {
    QUERY_CONN_REQ_TYPE = 0x01,
    QUERY_CONN_RSP_TYPE = 0x02,
};

enum HiboxSecAdvType {
    SEC_ADV_LEVEL_TYPE = 0x01,
    SEC_ADV_IRK_TYPE = 0x02,
    SEC_ADV_HBK_TYPE = 0x03,
    SEC_ADV_ADDR_TYPE = 0x04,
};

enum HiboxSdpCapPairConnType {
    SDP_AUDIO_MODE_TYPE = 0x01,
    SDP_AUDIO_PROFILE_TYPE = 0x03,
    SDP_AUDIO_PRODUCT_TYPE = 0x04,
    SDP_AUDIO_VERSION_TYPE = 0x05,
};

enum HiboxMobileConnType {
    MOBILE_CONN_BR_TYPE = 0x02,
    MOBILE_CONN_LE_TYPE = 0x03,
};

enum HiboxNatureType {
    EARBUDS_NATURE_TYPE = 0x01,
};

enum HiboxQueryBusinessType {
    QUERY_BUSINESS_CONN_TYPE = 0x01,
    QUERY_BUSINESS_ACCOUNT_HASH_TYPE = 0x02,
    QUERY_BUSINESS_MOBILE_BUSINESS_TYPE_TYPE = 0x03,
};

enum HiboxVendorAccountHashType {
    VENDOR_HONGMENG_OS_TYPE = 0x01,
    VENDOR_ACCOUNT_HASH_TYPE = 0x02,
};

enum HiboxAutoConnSwitchType {
    AUTO_CONN_SWITCH_TYPE = 0x01,
};

enum HiboxPeriperialMacUpdateType {
    PERIPHERAL_MAC = 0x01,
};

enum HiboxNotifySleDisconnectProfileType {
    NOTIFY_DISCONNECT_PROFILE = 0x01,
};

enum HiboxSleProfileStateType {
    ALL_AUDIO_PROFILE_CONNECTED = 0x01,
};

uint16_t HiboxPairConnMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);
bool HiboxPairConnMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
bool HiboxPairConnMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);

#endif
