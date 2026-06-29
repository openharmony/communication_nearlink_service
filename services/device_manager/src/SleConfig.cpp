/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "SleConfig.h"

#include <charconv>
#include "SleHuksTool.h"
#include "SleUtils.h"
#include "log.h"
#include "log_util.h"
#include "xml_parse.h"
#include "nearlink_utils.h"
#include "SleDefs.h"
namespace OHOS {
namespace Nearlink {
namespace {
const std::string SECTION_HOST = "Host";
const std::string PROPERTY_DEVICE_ADDR = "Address";
const std::string PROPERTY_DEVICE_NAME = "DeviceName";
const std::string PROPERTY_SLE_CREATE_CONFIG_FILE = "CreateConfigFile";
const std::string PROPERTY_IO_CAPABILITY = "IOCapability";
const std::string PROPERTY_ALIAS_NAME = "AliasName";
const std::string PROPERTY_SLE_LOCAL_ADDR_TYPE = "LocalAddrType";

const std::string SECTION_SLE_PAIRED_LIST = "Sle Paired Device List";
const std::string PROPERTY_SLE_PEER_APPEARANCE = "PeerAppearance";
const std::string PROPERTY_SLE_MANUFACTURER_ABILITY = "ManufacturerAbility";
const std::string PROPERTY_SLE_PEER_ADDR_TYPE = "PeerAddrType";
const std::string PROPERTY_SLE_PAIR_DIRECT = "PairDirect";
const std::string PROPERTY_SLE_PEER_LK = "PeerLk";
const std::string PROPERTY_SLE_CRYPTO_ALGO = "CryptoAlgo"; // 加密算法
const std::string PROPERTY_SLE_KEY_DERIV_ALGO = "KeyAlgo"; // 密钥分发算法
const std::string PROPERTY_SLE_INTEGR_CHK = "IntegrChk"; // 完整性校验
const std::string PROPERTY_SLE_GROUP_KEY = "GroupKey"; // 组播加密密钥
const std::string PROPERTY_SLE_GIV = "Giv"; // 组播初始化向量
const std::string PROPERTY_SLE_RANDOM_ADDR = "RandomAddr"; // 随机地址
const std::string PROPERTY_SLE_MUSIC_VOLUME = "MusicVolume"; // 媒体音量
const std::string PROPERTY_SLE_CALL_VOLUME = "CallVolume"; // 通话音量
const std::string PROPERTY_SLE_BT_ADDR_RELATED = "BtAddress";
const std::string PROPERTY_CDMS_ADDR_TYPE = "CdsmAddressType"; // 合作集地址类型标识
const std::string PROPERTY_SLE_IS_AUDIO_DEVICE = "IsAudioDevice";
const std::string PROPERTY_SLE_MODEL_ID = "ModelId";
const std::string PROPERTY_SLE_NEW_MODEL_ID = "NewModelId";
const std::string PROPERTY_SLE_SUB_MODEL_ID = "SubModelId";
const std::string PROPERTY_SLE_ICON_ID = "IconId";
const std::string PROPERTY_SLE_DEV_TYPE = "DevType";
const std::string PROPERTY_SLE_BUSSINESS_TYPE = "SleBussinessType";
const std::string PROPERTY_SLE_CONNECT_CONTROL = "SleConnectControl";
const std::string PROPERTY_SLE_USER_DISCONNECTED_FLAG = "UserDisconnected";

const std::string SECTION_SLE_CDSM_LIST = "Sle Cooperation Device List";
const std::string PROPERTY_SLE_CDSM_MEMBER_LIST = "CdsmMemberList";
const std::string PROPERTY_SLE_CDSM_IS_PRIVATE = "IsPrivateDevice";
const std::string PROPERTY_TWS_WEAR_DETECTION_STATE = "WearDetectionState"; // 佩戴检测开关状态
const std::string PROPERTY_TWS_AUTO_CONNECT_SWITCH = "AutoConnectSwitch";   // 自动连接开关

const std::string SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST = "Sle Cloud Paired Cooperation Device List";
const std::string PROPERTY_CLOUD_PAIRED_BT_ADDR = "BluetoothAddr";
const std::string PROPERTY_CLOUD_PAIRED_DEVICE_NAME = "DeviceName";
const std::string PROPERTY_CLOUD_PAIRED_TOKEN = "Token";
const std::string PROPERTY_CLOUD_PAIRED_REPORT_ADDR = "ReportAddr";
const std::string PROPERTY_CLOUD_PAIRED_MEMBERS_ADDR_LIST = "MembersAddrList";
const std::string PROPERTY_CLOUD_PAIRED_MODEL = "Model";
const std::string PROPERTY_CLOUD_PAIRED_SUB_MODEL_ID = "SubModelId";
const std::string PROPERTY_CLOUD_PAIRED_DEVICE_ICON_ID = "IconId";
const std::string PROPERTY_CLOUD_PAIRED_DEVICE_STATE = "CloudPairState";

const std::string SECTION_LAST_ASC_ACTIVE_DEVICE = "Last ASC Active Device";
const std::string SECTION_LAST_ASC_CONNECTED_DEVICE = "Last ASC Connected Device";
const std::string PROPERTY_LAST_ASC_ACT_CONN_RAW_ADDRESS = "RawAddress";

const std::string SECTION_SLE_BG_RECONN_QUEUE = "Sle Background Reconnection Queue";
const std::string PROPERTY_BG_RECONN_QUEUE_INFO = "QueueInfo";

constexpr size_t RAW_LINK_KEY_STR_LENGTH = 32;
constexpr size_t RAW_CLOUD_PAIR_TOKEN_LENGTH = 64;
constexpr int CONFIG_FILE_NOT_CREATED = 0;
constexpr int CONFIG_FILE_CREATED = 1;
constexpr int WEAR_DETECTION_STATE_INVALID = -1;
constexpr int VIRTUAL_AUTO_CONNECT_SWITCH_OFF = 0;
constexpr int OCTET8_LEN = 8;
}

SleConfig &SleConfig::GetInstance()
{
    static SleConfig instance;
    return instance;
}

SleConfig::SleConfig()
{
    config_ = AdapterDeviceConfig::GetInstance();
}

SleConfig::~SleConfig()
{}

bool SleConfig::IsRawLinkKeyCharValid(const char* linkkeyChar, const uint8_t linkKeyLen) const
{
    NL_CHECK_RETURN_RET(linkKeyLen == RAW_LINK_KEY_STR_LENGTH, false,
        "[SleConfig] linkKeyStr len is invalid, len(%{public}zu)", linkKeyLen);
    for (size_t i = 0; i < linkKeyLen; ++i) {
        NL_CHECK_RETURN_RET(isxdigit(linkkeyChar[i]), false, "[SleConfig] digit check failed.");
    }
    return true;
}

bool SleConfig::IsRawCloudDeviceTokenCharValid(const char* tokenChar, const uint8_t tokenLen) const
{
    NL_CHECK_RETURN_RET(tokenLen == RAW_CLOUD_PAIR_TOKEN_LENGTH, false,
        "[SleConfig] cloudDeviceToken len is invalid, len(%{public}zu)", tokenLen);
    for (size_t i = 0; i < tokenLen; ++i) {
        NL_CHECK_RETURN_RET(isxdigit(tokenChar[i]), false, "[SleConfig] digit check failed.");
    }
    return true;
}

void SleConfig::EncryptLinkKey() const
{
    if (GetFileFlag() == CONFIG_FILE_CREATED) {
        HILOGI("[SleConfig] flag is exist.");
        return;
    }
    HILOGI("[SleConfig] Encrypt link Key start.");
    char *linkkeyChar = new (std::nothrow) char[RAW_LINK_KEY_STR_LENGTH + 1];
    if (!linkkeyChar) {
        HILOGE("[SleConfig] memory alloc failed");
        return;
    }
    std::vector<std::string> pairedAddrList = GetPairedAddrList();
    for (auto &addr : pairedAddrList) {
        (void)memset_s(linkkeyChar, RAW_LINK_KEY_STR_LENGTH + 1, 0x00, RAW_LINK_KEY_STR_LENGTH + 1);
        // 只有当获取的link key为未加密的的长度(16字节)才会复制到linkkeyChar中
        bool retLinkKey = GetLinkKeyChar(addr, linkkeyChar, RAW_LINK_KEY_STR_LENGTH);
        if (!retLinkKey) {
            LOG_ERROR("[SleConfig]:Get sle local ltk failed");
            continue;
        }
        // 校验linkkeyChar是否为有效的密钥，长度为16字节且为16进制字符
        if (!IsRawLinkKeyCharValid(linkkeyChar, RAW_LINK_KEY_STR_LENGTH)) {
            HILOGE("[SleConfig] invalid linkkey");
            (void)memset_s(linkkeyChar, RAW_LINK_KEY_STR_LENGTH + 1, 0x00, RAW_LINK_KEY_STR_LENGTH + 1);
            RemovePairedDevice(addr);
            continue;
        }
        LinkKey linkKey;
        if (!SleUtils::ConvertHexCharToInt(linkkeyChar, linkKey.data(), OCTET16_LEN)) {
            HILOGE("[SleConfig] covert hexchar to int fail");
            (void)memset_s(linkkeyChar, RAW_LINK_KEY_STR_LENGTH + 1, 0x00, RAW_LINK_KEY_STR_LENGTH + 1);
            RemovePairedDevice(addr);
            continue;
        }
        EncryptedLinkKey encryptedLinkKey;
        int32_t ret = SleHksTool::GetInstance().SleLinkKeyEncrypt(linkKey, encryptedLinkKey);
        if (ret != HKS_SUCCESS) {
            HILOGE("[SleConfig] SleLinkKeyEncrypt failed with ret=%{public}d", ret);
            (void)memset_s(linkkeyChar, RAW_LINK_KEY_STR_LENGTH + 1, 0x00, RAW_LINK_KEY_STR_LENGTH + 1);
            (void)memset_s(&linkKey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
            RemovePairedDevice(addr);
            continue;
        }
        (void)memset_s(linkkeyChar, RAW_LINK_KEY_STR_LENGTH + 1, 0x00, RAW_LINK_KEY_STR_LENGTH + 1);
        (void)memset_s(&linkKey, sizeof(LinkKey), 0x00, sizeof(LinkKey));
        std::string hexString = SleUtils::ConvertIntToHexString(encryptedLinkKey.data(), encryptedLinkKey.size());
        SetLinkKey(addr, hexString);
    }
    delete[] linkkeyChar;
    SetFileFlag();
    Save();
}

void SleConfig::EncryptCloudDeviceToken() const
{
    char *tokenChar = new (std::nothrow) char[RAW_CLOUD_PAIR_TOKEN_LENGTH + 1];
    if (!tokenChar) {
        HILOGE("[SleConfig] memory alloc failed");
        return;
    }
    std::vector<std::string> cloudDeviceList = GetCloudDeviceAddrList();
    for (auto &addr : cloudDeviceList) {
        (void)memset_s(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1, 0x00, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1);
        if (!GetCloudDeviceTokenChar(addr, tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH) ||
            strnlen(tokenChar, OCTET160_LEN) != RAW_CLOUD_PAIR_TOKEN_LENGTH) {
            LOG_ERROR("[SleConfig]:Get cloud device token failed or no need to encrypt");
            continue;
        }
        if (!IsRawCloudDeviceTokenCharValid(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH)) {
            HILOGE("[SleConfig] invalid cloud device token");
            (void)memset_s(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1, 0x00, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1);
            RemoveSpecificCloudDevice(addr);
            continue;
        }
        CloudDeviceToken cloudDevicetoken;
        if (!SleUtils::ConvertHexCharToInt(tokenChar, cloudDevicetoken.data(), OCTET32_LEN)) {
            HILOGE("[SleConfig] covert hexchar to int fail");
            (void)memset_s(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1, 0x00, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1);
            RemoveSpecificCloudDevice(addr);
            continue;
        }
        EncryptedCloudDeviceToken encryptedCloudDeviceToken;
        int32_t errCode = SleHksTool::GetInstance().SleCloudDeviceTokenEncrypt(
            cloudDevicetoken, encryptedCloudDeviceToken);
        if (errCode != HKS_SUCCESS) {
            HILOGE("[SleConfig] SleCloudDeviceTokenEncrypt failed with errCode=%{public}d", errCode);
            (void)memset_s(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1, 0x00, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1);
            (void)memset_s(&cloudDevicetoken, sizeof(CloudDeviceToken), 0x00, sizeof(CloudDeviceToken));
            RemoveSpecificCloudDevice(addr);
            continue;
        }
        (void)memset_s(tokenChar, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1, 0x00, RAW_CLOUD_PAIR_TOKEN_LENGTH + 1);
        (void)memset_s(&cloudDevicetoken, sizeof(CloudDeviceToken), 0x00, sizeof(CloudDeviceToken));
        std::string hexStr = SleUtils::ConvertIntToHexString(
            encryptedCloudDeviceToken.data(), encryptedCloudDeviceToken.size());
        bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, addr, PROPERTY_CLOUD_PAIRED_TOKEN, hexStr);
        if (!ret) {
            HILOGE("[SleConfig] Set cloud device token failed");
            RemoveSpecificCloudDevice(addr);
        }
    }
    delete[] tokenChar;
    Save();
}

bool SleConfig::GetCloudDeviceTokenChar(const std::string &address, char* token, uint8_t valueLen) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address,
        PROPERTY_CLOUD_PAIRED_TOKEN, token, valueLen);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle cloud device token failed");
        return false;
    }
    for (size_t i = 0; i < strnlen(token, OCTET160_LEN); ++i) {
        if (token[i] >= 'a' && token[i] <= 'z') {
            token[i] = toupper(token[i]);
        }
    }
    return true;
}

bool SleConfig::LoadConfigInfo() const
{
    HILOGI("[SleConfig]");
    if (config_->Load()) {
        HILOGI("[SleConfig] cfg file is exist.");
        EncryptLinkKey();
        EncryptCloudDeviceToken();
        return true;
    }
    if (!config_->CreateFile()) {
        HILOGE("[SleConfig] create cfg file failed.");
        return false;
    }
    if (!config_->Load()) {
        HILOGI("[SleConfig] Load cfg file failed.");
        return false;
    }
    SetFileFlag();
    return Save();
}

bool SleConfig::Save() const
{
    LOG_DEBUG("[SleConfig]");
    if (!config_->Save()) {
        HILOGE("[SleConfig] Save fail.");
        return false;
    }
#ifdef TV_STANDARD
    if (!config_->Fsync()) {
        HILOGE("[SleConfig] Fsync fail.");
        return false;
    }
#endif
    return true;
}

int SleConfig::GetFileFlag() const
{
    int flag = CONFIG_FILE_NOT_CREATED;
    if (!config_->GetValue(SECTION_HOST, PROPERTY_SLE_CREATE_CONFIG_FILE, flag)) {
        HILOGE("[SleConfig] get value failed.");
        flag = CONFIG_FILE_NOT_CREATED;
    }
    HILOGI("[SleConfig] flag(%{public}d)", flag);
    return flag;
}

void SleConfig::SetFileFlag() const
{
    HILOGI("[SleConfig] Set create cfg file.");
    int flag = CONFIG_FILE_CREATED;
    if (!config_->SetValue(SECTION_HOST, PROPERTY_SLE_CREATE_CONFIG_FILE, flag)) {
        HILOGE("[SleConfig] Set create cfg file failed.");
    }
    HILOGI("[SleConfig] Set create cfg file end");
}

std::string SleConfig::GetLocalName() const
{
    LOG_DEBUG("[SleConfig]");

    std::string name = "";
    bool ret = config_->GetValue(SECTION_HOST, PROPERTY_DEVICE_NAME, name);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get local name failed");
    }
    return name;
}

bool SleConfig::SetLocalName(const std::string &name) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_HOST, PROPERTY_DEVICE_NAME, name);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set local name failed");
    }
    return ret;
}

std::string SleConfig::GetLocalAddress() const
{
    LOG_DEBUG("[SleConfig]");

    std::string addr = SLE_INVALID_MAC_ADDRESS;
    bool ret = config_->GetValue(SECTION_HOST, PROPERTY_DEVICE_ADDR, addr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get local address failed");
    }
    return addr;
}

bool SleConfig::SetLocalAddress(const std::string &addr) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_HOST, PROPERTY_DEVICE_ADDR, addr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set local address failed");
    }
    return ret;
}

int SleConfig::GetIoCapability() const
{
    LOG_DEBUG("[SleConfig]");

    int io = 0;
    bool ret = config_->GetValue(SECTION_HOST, PROPERTY_IO_CAPABILITY, io);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get local IO capability failed!");
    }
    return io;
}

bool SleConfig::SetSleLocalAddrType(int localAddrType) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_HOST, PROPERTY_SLE_LOCAL_ADDR_TYPE, localAddrType);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle local addr failed");
    }
    return ret;
}

std::string SleConfig::GetLinkKey(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    std::string lk;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_PEER_LK, lk);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle local ltk failed");
    }
    return lk;
}

bool SleConfig::GetLinkKeyChar(const std::string &section, char* lk, uint8_t valueLen) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_PEER_LK, lk, valueLen);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle local ltk failed");
        return false;
    }
    return true;
}

int SleConfig::GetCryptoAlgo(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    int algo;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_CRYPTO_ALGO, algo);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle local ltk failed");
    }
    return algo;
}

int SleConfig::GetKeyDerivAlgo(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    int algo;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_KEY_DERIV_ALGO, algo);
    if (!ret) {
        LOG_ERROR("[SleConfig]Get sle local ltk failed");
    }
    return algo;
}

int SleConfig::GetIntegrChk(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    int check;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_INTEGR_CHK, check);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle local ltk failed");
    }
    return check;
}

std::string SleConfig::GetGroupKey(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    std::string groupkey;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_GROUP_KEY, groupkey);
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get sle local group key failed");
        return "";
    }
    return groupkey;
}

uint64_t SleConfig::GetGiv(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    std::string givstr;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_GIV, givstr);
    uint64_t giv;
    std::array<uint8_t, OCTET8_LEN> givarr;
    if (!ret) {
        LOG_ERROR("[SleConfig]:Get giv failed");
        return 0;
    }
    if (!SleUtils::ConvertHexStringToInt(givstr, givarr.data(), givarr.size())) {
        LOG_ERROR("[SleConfig]:Convert giv failed");
        return 0;
    }
    if (memcpy_s(&giv, OCTET8_LEN, &givarr[0], OCTET8_LEN) != EOK) {
        LOG_ERROR("[SleConfig] memcpy_s failed!");
        return 0;
    }
    return giv;
}

std::string SleConfig::GetPeerName(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    std::string name = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_DEVICE_NAME, name);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer name failed");
    }
    return name;
}

std::string SleConfig::GetPeerAlias(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    std::string name = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_ALIAS_NAME, name);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer alias name failed");
    }
    return name;
}

bool SleConfig::SetPeerAlias(const std::string &subSection, const std::string &name) const
{
    if (!config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_ALIAS_NAME, name)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    return true;
}

bool SleConfig::GetUserDisconnectedFlag(const std::string &subSection) const
{
    bool isUserDisconnected = false;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection,
        PROPERTY_SLE_USER_DISCONNECTED_FLAG, isUserDisconnected);
    if (!ret) {
        LOG_WARN("[SleConfig] Get isUserDisconnected failed");
    }
    LOG_DEBUG("Get isUserDisconnected = %{public}d", isUserDisconnected);
    return isUserDisconnected;
}

bool SleConfig::SetUserDisconnectedFlag(const std::string &subSection, bool isUserDisconnected) const
{
    LOG_DEBUG("Set isUserDisconnected = %{public}d", isUserDisconnected);
    if (!config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection,
        PROPERTY_SLE_USER_DISCONNECTED_FLAG, isUserDisconnected)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }
    return true;
}

/* 设置合作集成员设备标识 */
bool SleConfig::SetCdsmAddrType(const std::string &subSection, const int cdsmAddrType) const
{
    if (!config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_CDMS_ADDR_TYPE, cdsmAddrType)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    return true;
}

/* 获取合作集成员设备标识 */
int SleConfig::GetCdsmAddrType(const std::string &subSection) const
{
    int cdsmAddrType = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_CDMS_ADDR_TYPE, cdsmAddrType);
    if (!ret) {
        LOG_WARN("[SleConfig] Get peer addr cdsm addr type failed failed");
    }

    return cdsmAddrType;
}

/* 设置合作集地址列表 */
bool SleConfig::SetCdsmMemberList(const std::string &subSection, const std::vector<std::string> &cdsmAddrList) const
{
    /* 合作集地址拼接，分隔符为; */
    std::string devList = "";
    for (auto &dev : cdsmAddrList) {
        if (!devList.empty()) {
            devList += ";";
        }
        devList += dev;
    }

    /* 保存到xml */
    if (!config_->SetValue(SECTION_SLE_CDSM_LIST, subSection, PROPERTY_SLE_CDSM_MEMBER_LIST, devList)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    return true;
}

/* 获取合作集地址列表 */
bool SleConfig::GetCdsmMemberList(const std::string &subSection, std::vector<std::string> &cdsmAddrList) const
{
    cdsmAddrList.clear();
    std::string memberList = "";
    if (!config_->GetValue(SECTION_SLE_CDSM_LIST, subSection, PROPERTY_SLE_CDSM_MEMBER_LIST, memberList)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    size_t start = 0;
    size_t end = memberList.find(';');
    while (end != std::string::npos) {
        cdsmAddrList.push_back(memberList.substr(start, end - start));
        start = end + 1;
        end = memberList.find(';', start);
    }
    cdsmAddrList.push_back(memberList.substr(start));

    return true;
}

bool SleConfig::SetCdsmIsPrivateDevice(const std::string &subSection, const int isPrivate) const
{
    /* 保存到xml */
    if (!config_->SetValue(SECTION_SLE_CDSM_LIST, subSection, PROPERTY_SLE_CDSM_IS_PRIVATE, isPrivate)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    return true;
}

bool SleConfig::GetCdsmIsPrivateDevice(const std::string &subSection) const
{
    int isPrivate = 0;
    if (!config_->GetValue(SECTION_SLE_CDSM_LIST, subSection, PROPERTY_SLE_CDSM_IS_PRIVATE, isPrivate)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    if (isPrivate != 0) {
        return true;
    }
    return false;
}

/* 读取合作集report列表 */
std::vector<std::string> SleConfig::GetAllCdsmReportList() const
{
    LOG_DEBUG("[SleConfig] Enter");

    std::vector<std::string> cdsmReportList = {};
    bool ret = config_->GetSubSections(SECTION_SLE_CDSM_LIST, cdsmReportList);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get cdsm device list Device List failed");
    }
    return cdsmReportList;
}

/* 删除xml中合作集数据 */
bool SleConfig::RemoveCdsmGroup(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig] Enter");

    bool ret = config_->RemoveSection(SECTION_SLE_CDSM_LIST, subSection);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove cdsm group device info failed");
    }

    return ret;
}

int SleConfig::GetPeerDeviceIoCapability(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    int io = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_IO_CAPABILITY, io);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer io capability failed");
    }
    return io;
}

bool SleConfig::SetPeerName(const std::string &subSection, const std::string &name) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_DEVICE_NAME, name);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set peer device name failed");
    }
    return ret;
}

bool SleConfig::SetPeerRandomAddress(const std::string &subSection, const std::string &addr) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_RANDOM_ADDR, addr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set peer random address failed");
    }
    return ret;
}

std::string SleConfig::GetPeerRandomAddress(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    std::string addr = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_RANDOM_ADDR, addr);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer random address failed");
    }
    return addr;
}

bool SleConfig::SetPeerAddressType(const std::string &subSection, const uint8_t type) const
{
    LOG_DEBUG("[SleConfig]");

    int addrType = static_cast<int>(type);
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_PEER_ADDR_TYPE, addrType);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set peer address type failed");
    }
    return ret;
}

uint8_t SleConfig::GetPeerAddressType(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    int type = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_PEER_ADDR_TYPE, type);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer address type failed");
    }
    return static_cast<uint8_t>(type);
}

bool SleConfig::SetPeerAppearance(const std::string &subSection, const int appearance) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_PEER_APPEARANCE, appearance);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set peer appearance failed");
    }
    return ret;
}

int SleConfig::GetPeerAppearance(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    int appearance = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_PEER_APPEARANCE, appearance);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get peer appearance failed");
    }
    return appearance;
}

bool SleConfig::SetManufacturerAbility(const std::string &subSection, const std::string &manuAbility) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_MANUFACTURER_ABILITY, manuAbility);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set manufactureAbility failed");
    }
    return ret;
}

std::string SleConfig::GetManufacturerAbility(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");
    std::string manuAbility = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_MANUFACTURER_ABILITY, manuAbility);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get manufactureAbility failed");
    }
    return manuAbility;
}

bool SleConfig::SetIsAudioDeviceFlag(const std::string &subSection, const bool isAudioDevice) const
{
    HILOGD("%{public}s: isAudioDevice=%{public}d", GetEncryptAddr(subSection).c_str(), isAudioDevice);

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_IS_AUDIO_DEVICE,
        isAudioDevice);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set audio device flag failed");
    }
    return ret;
}

bool SleConfig::GetIsAudioDeviceFlag(const std::string &subSection) const
{
    bool isAudioDevice = false;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_IS_AUDIO_DEVICE,
        isAudioDevice);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get audio device flag failed");
    }
    HILOGD("%{public}s: isAudioDevice=%{public}d", GetEncryptAddr(subSection).c_str(), isAudioDevice);
    return isAudioDevice;
}

bool SleConfig::RemovePairedDevice(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->RemoveSection(SECTION_SLE_PAIRED_LIST, subSection);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove paired device info failed");
    }
    return ret;
}

bool SleConfig::RemoveAllPairedDevices() const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->RemoveSection(SECTION_SLE_PAIRED_LIST);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove all paired device info failed");
    }
    return ret;
}

std::vector<std::string> SleConfig::GetPairedAddrList() const
{
    LOG_DEBUG("[SleConfig]");

    std::vector<std::string> pairedList;
    bool ret = config_->GetSubSections(SECTION_SLE_PAIRED_LIST, pairedList);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get Paired Device List failed");
    }
    return pairedList;
}

bool SleConfig::SetPeerDeviceIoCapability(const std::string &subSection, int io) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_IO_CAPABILITY, io);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set peer device name failed");
    }
    return ret;
}

bool SleConfig::SetLinkKey(const std::string &section, const std::string &lk) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_PEER_LK, lk);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle local ltk failed");
    }
    return ret;
}

bool SleConfig::SetCryptoAlgo(const std::string &section, const int algo) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_CRYPTO_ALGO, algo);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle crypto Algo failed");
    }
    return ret;
}

bool SleConfig::SetKeyDerivAlgo(const std::string &section, const int algo) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_KEY_DERIV_ALGO, algo);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle key Deriv Algo failed");
    }
    return ret;
}

bool SleConfig::SetIntegrChk(const std::string &section, const int check) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_INTEGR_CHK, check);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle chk failed");
    }
    return ret;
}

bool SleConfig::SetGroupkey(const std::string &section, const std::string &groupkey) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_GROUP_KEY, groupkey);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle group key failed");
    }
    return ret;
}

bool SleConfig::SetGiv(const std::string &section, const uint64_t giv) const
{
    LOG_DEBUG("[SleConfig]");
    uint8_t givBytes[OCTET8_LEN];
    if (memcpy_s(&givBytes, OCTET8_LEN, &giv, sizeof(uint64_t)) != EOK) {
        LOG_ERROR("[SleConfig] memcpy_s failed!");
        return false;
    }
    std::string givstr = SleUtils::ConvertIntToHexString(givBytes, OCTET8_LEN);
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_GIV, givstr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set Giv failed");
    }
    return ret;
}

bool SleConfig::SetPairDirect(const std::string &section, const int activePair) const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_PAIR_DIRECT, activePair);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle pair direct failed");
    }
    return ret;
}

int SleConfig::GetPairDirect(const std::string &section) const
{
    LOG_DEBUG("[SleConfig]");
    int activePair = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, section, PROPERTY_SLE_PAIR_DIRECT, activePair);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get sle pair direct failed");
    }
    return activePair;
}

bool SleConfig::SetDeviceVolume(const std::string &address, const std::string &property, const int volume) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, property, volume);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle device %{public}s volume=%{public}d failed", property.c_str(), volume);
    }
    return ret;
}

int SleConfig::GetDeviceVolume(const std::string &address, const std::string &property, const int defaultVolume) const
{
    int volume = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, property, volume);
    if (!ret) {
        // 读取保存的音量失败时，使用默认音量
        volume = defaultVolume;
        LOG_ERROR("[SleConfig] Get sle device %{public}s volume failed, return default", property.c_str());
    }
    LOG_DEBUG("[SleConfig] volume=%{public}d", volume);
    return volume;
}

bool SleConfig::SetDeviceMediaVolume(const std::string &address, const int volume) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = SetDeviceVolume(address, PROPERTY_SLE_MUSIC_VOLUME, volume);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle device media volume=%{public}d failed", volume);
    }
    return ret;
}

int SleConfig::GetDeviceMediaVolume(const std::string &address, const int defaultVolume) const
{
    LOG_DEBUG("[SleConfig]");
    int volume = GetDeviceVolume(address, PROPERTY_SLE_MUSIC_VOLUME, defaultVolume);
    return volume;
}

bool SleConfig::SetDeviceCallVolume(const std::string &address, const int volume) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = SetDeviceVolume(address, PROPERTY_SLE_CALL_VOLUME, volume);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle device call volume=%{public}d failed", volume);
    }
    return ret;
}

int SleConfig::GetDeviceCallVolume(const std::string &address, const int defaultVolume) const
{
    LOG_DEBUG("[SleConfig]");
    int volume = GetDeviceVolume(address, PROPERTY_SLE_CALL_VOLUME, defaultVolume);
    return volume;
}

std::string SleConfig::GetBtAddrBySleAddr(const std::string &sleAddr) const
{
    LOG_DEBUG("[SleConfig]");
    std::string name = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, sleAddr, PROPERTY_SLE_BT_ADDR_RELATED, name);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get btAddr failed");
    }
    return name;
}

bool SleConfig::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, sleAddr, PROPERTY_SLE_BT_ADDR_RELATED, btAddr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set btAddr failed");
    }
    return ret;
}

bool SleConfig::SetConfigWearDetectionState(const std::string &address, const int state) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_TWS_WEAR_DETECTION_STATE, state);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle device wear detection state=%{public}d failed", state);
    }
    return ret;
}

int SleConfig::GetConfigWearDetectionState(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    int state = WEAR_DETECTION_STATE_INVALID;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_TWS_WEAR_DETECTION_STATE, state);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get wear detection state failed");
    }
    return state;
}

bool SleConfig::SetDeviceModelId(const std::string &address, const std::string &modelId) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_MODEL_ID, modelId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle model id failed");
    }
    return ret;
}

std::string SleConfig::GetDeviceModelId(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    std::string modelId = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_MODEL_ID, modelId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get model id failed");
    }
    return modelId;
}

bool SleConfig::SetDeviceNewModelId(const std::string &address, const std::string &newModelId) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_NEW_MODEL_ID, newModelId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle new model id failed");
    }
    return ret;
}

std::string SleConfig::GetDeviceNewModelId(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    std::string newModelId = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_NEW_MODEL_ID, newModelId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get new model id failed");
    }
    return newModelId;
}

bool SleConfig::SetDeviceSubModelId(const std::string &address, const std::string &subModelId) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_SUB_MODEL_ID, subModelId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle sub model id failed");
    }
    return ret;
}

std::string SleConfig::GetDeviceSubModelId(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    std::string subModelId = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_SUB_MODEL_ID, subModelId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get sub model id failed");
    }
    return subModelId;
}

bool SleConfig::SetDeviceIconId(const std::string &address, const std::string &iconId) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_ICON_ID, iconId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle icon id failed");
    }
    return ret;
}

std::string SleConfig::GetDeviceIconId(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    std::string iconId = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_ICON_ID, iconId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get icon id failed");
    }
    return iconId;
}

bool SleConfig::SetDeviceDevType(const std::string &address, const std::string &devType) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_DEV_TYPE, devType);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle devType failed");
    }
    return ret;
}
 
std::string SleConfig::GetDeviceDevType(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    std::string devType = "";
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_DEV_TYPE, devType);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get devType failed");
    }
    return devType;
}

bool SleConfig::SetSleBusiness(const std::string &subSection, const int sleBusinessType) const
{
    HILOGI("%{public}s: sleBusinessType=%{public}d", GetEncryptAddr(subSection).c_str(), sleBusinessType);

    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_BUSSINESS_TYPE, sleBusinessType);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle business type failed");
    }
    return ret;
}

int SleConfig::GetSleBusiness(const std::string &subSection) const
{
    int sleBusinessType = 0;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, subSection, PROPERTY_SLE_BUSSINESS_TYPE, sleBusinessType);
    if (!ret) {
        LOG_WARN("[SleConfig] Get sle business type failed");
    }

    return sleBusinessType;
}

bool SleConfig::SetAvailableControl(const std::string &address, bool availableControl) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_CONNECT_CONTROL, availableControl);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle availableControl failed");
    }
    return ret;
}

bool SleConfig::GetAvailableControl(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    bool availableControl;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_SLE_CONNECT_CONTROL, availableControl);
    if (!ret) {
        HILOGI("[SleConfig] Get sle availableControl failed, 5.0 upgrade 6.0");
        return true; // 5.0配对记录升级6.0场景，无控制选项，默认可用设备，可连接
    }
    return availableControl;
}
bool SleConfig::SetAutoConnectSwitch(const std::string &address, const int result) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_TWS_AUTO_CONNECT_SWITCH, result);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle preemption result=%{public}d failed", result);
    }
    return ret;
}

int SleConfig::GetAutoConnectSwitch(const std::string &address) const
{
    LOG_DEBUG("[SleConfig]");
    int result = VIRTUAL_AUTO_CONNECT_SWITCH_OFF;
    bool ret = config_->GetValue(SECTION_SLE_PAIRED_LIST, address, PROPERTY_TWS_AUTO_CONNECT_SWITCH, result);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get sle preemption result fail");
    }
    return result;
}

// 云配对相关
bool SleConfig::SetCloudDeviceBtAddr(const std::string &address, const std::string &btAddr) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address, PROPERTY_CLOUD_PAIRED_BT_ADDR, btAddr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device btAddr failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceBtAddr(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string btAddr = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address, PROPERTY_CLOUD_PAIRED_BT_ADDR, btAddr);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device btAddr failed");
    }
    return btAddr;
}

bool SleConfig::SetCloudDeviceName(const std::string &address, const std::string &deviceName) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address,
        PROPERTY_CLOUD_PAIRED_DEVICE_NAME, deviceName);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device name failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceName(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string deviceName = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address,
        PROPERTY_CLOUD_PAIRED_DEVICE_NAME, deviceName);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device name failed");
    }
    return deviceName;
}

bool SleConfig::SetCloudDeviceToken(const std::string &address, const std::vector<uint8_t> &token) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    CloudDeviceToken cloudDeviceToken;
    NL_CHECK_RETURN_RET(token.size() == OCTET32_LEN, false, "[SleConfig] token size check failed");
    for (int i = 0; i < OCTET32_LEN; i++) {
        cloudDeviceToken[i] = token[i];
    }
    EncryptedCloudDeviceToken encryptedCloudDeviceToken;
    if (SleHksTool::GetInstance().SleCloudDeviceTokenEncrypt(
        cloudDeviceToken, encryptedCloudDeviceToken) != HKS_SUCCESS) {
        LOG_ERROR("[SleConfig] Token encryption falied");
        return false;
    }
    std::string encryptedToken = SleUtils::ConvertIntToHexString(
        encryptedCloudDeviceToken.data(), encryptedCloudDeviceToken.size());
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address,
        PROPERTY_CLOUD_PAIRED_TOKEN, encryptedToken);
    NL_CHECK_RETURN_RET(ret, false, "[SleConfig] Set cloud device token failed");
    return ret;
}

bool SleConfig::GetCloudDeviceToken(const std::string &address, std::vector<uint8_t> &token) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    std::string encryptedToken = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, address,
        PROPERTY_CLOUD_PAIRED_TOKEN, encryptedToken);
    NL_CHECK_RETURN_RET(ret, false, "[SleConfig] Get cloud device token failed");
    CloudDeviceToken cloudDeviceToken;
    EncryptedCloudDeviceToken encryptedCloudDeviceToken;
    ret = SleUtils::ConvertHexStringToInt(encryptedToken,
        encryptedCloudDeviceToken.data(), encryptedCloudDeviceToken.size());
    NL_CHECK_RETURN_RET(ret, false, "ConvertHexStringToInt fail");
    if (SleHksTool::GetInstance().SleCloudDeviceTokenDecrypt(
        encryptedCloudDeviceToken, cloudDeviceToken) != HKS_SUCCESS) {
        LOG_ERROR("[SleConfig] Token decryption falied");
        return false;
    }
    for (int i = 0; i < OCTET32_LEN; i++) {
        token[i] = cloudDeviceToken[i];
    }
    return true;
}

bool SleConfig::SetCloudDeviceReportAddr(const std::string &address, const std::string &reportAddr) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_REPORT_ADDR, reportAddr);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device report addr failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceReportAddr(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string reportAddr = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_REPORT_ADDR, reportAddr);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device report addr failed");
    }
    return reportAddr;
}

bool SleConfig::SetCloudDeviceModel(const std::string &address, const std::string &model) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_MODEL, model);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device model failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceModel(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string model = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_MODEL, model);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device model failed");
    }
    return model;
}

bool SleConfig::SetCloudDeviceSubModelId(const std::string &address, const std::string &subModelId) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_SUB_MODEL_ID, subModelId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device sub model id failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceSubModelId(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string subModelId = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_SUB_MODEL_ID, subModelId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device sub model id failed");
    }
    return subModelId;
}

bool SleConfig::SetCloudDeviceIconId(const std::string &address, const std::string &iconId) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_DEVICE_ICON_ID, iconId);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device icon id failed");
    }
    return ret;
}

std::string SleConfig::GetCloudDeviceIconId(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return "";
    }
    LOG_DEBUG("[SleConfig]");
    std::string iconId = "";
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_DEVICE_ICON_ID, iconId);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device icon id failed");
    }
    return iconId;
}

bool SleConfig::SetCloudDeviceState(const std::string &address, const int32_t cloudPairState) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_DEVICE_STATE, cloudPairState);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set cloud device state failed");
    }
    return ret;
}

int32_t SleConfig::GetCloudDeviceState(const std::string &address)
{
    if (!IsValidAddress(address)) {
        return false;
    }
    LOG_DEBUG("[SleConfig]");
    int32_t cloudPairState;
    bool ret = config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address, PROPERTY_CLOUD_PAIRED_DEVICE_STATE, cloudPairState);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get cloud device state failed");
    }
    return cloudPairState;
}

bool SleConfig::SetCloudDeviceMembersAddrList(const std::string &address,
    const std::vector<std::string> &membersAddrList) const
{
    if (!IsValidAddress(address)) {
        return false;
    }
    std::string devList = "";
    for (auto &dev : membersAddrList) {
        if (!devList.empty()) {
            devList += ";";
        }
        devList += dev;
    }
    if (!config_->SetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address,  PROPERTY_CLOUD_PAIRED_MEMBERS_ADDR_LIST, devList)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }
    return true;
}

bool SleConfig::GetCloudDeviceMembersAddrList(const std::string &address, std::vector<std::string> &membersAddrList)
{
    if (!IsValidAddress(address)) {
        return false;
    }
    membersAddrList.clear();
    std::string memberList = "";
    if (!config_->GetValue(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST,
        address,  PROPERTY_CLOUD_PAIRED_MEMBERS_ADDR_LIST, memberList)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }

    size_t start = 0;
    size_t end = memberList.find(';');
    while (end != std::string::npos) {
        std::string curAddr = memberList.substr(start, end - start);
        if (!IsValidAddress(curAddr)) {
            return false;
        }
        membersAddrList.push_back(curAddr);
        start = end + 1;
        end = memberList.find(';', start);
    }
    std::string lastAddr = memberList.substr(start);
    if (!IsValidAddress(lastAddr)) {
        return false;
    }
    membersAddrList.push_back(lastAddr);

    return true;
}

bool SleConfig::RemoveAllCloudDevice() const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->RemoveSection(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove sle cloud paired device failed");
    }
    return ret;
}

bool SleConfig::RemoveSpecificCloudDevice(const std::string &subSection) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->RemoveSection(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, subSection);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove sle cloud paired device failed");
    }
    return ret;
}

std::vector<std::string> SleConfig::GetCloudDeviceAddrList() const
{
    LOG_DEBUG("[SleConfig]");
    std::vector<std::string> cloudDeviceList;
    bool ret = config_->GetSubSections(SECTION_SLE_CLOUD_PAIRED_DEVICE_LIST, cloudDeviceList);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get Cloud Device List failed");
    }
    return cloudDeviceList;
}

bool SleConfig::SetLastASCActiveDevice(const std::string &device) const
{
    if (!IsValidAddress(device)) {
        return false;
    }
    if (!config_->SetValue(SECTION_LAST_ASC_ACTIVE_DEVICE, PROPERTY_LAST_ASC_ACT_CONN_RAW_ADDRESS, device)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }
    return true;
}

std::string SleConfig::GetLastASCActiveDevice() const
{
    LOG_DEBUG("[SleConfig]");

    std::string device = "";
    bool ret = config_->GetValue(SECTION_LAST_ASC_ACTIVE_DEVICE, PROPERTY_LAST_ASC_ACT_CONN_RAW_ADDRESS, device);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get sle last ASC active device failed");
    }
    return device;
}

bool SleConfig::RemoveLastASCActiveDevice() const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->RemoveSection(SECTION_LAST_ASC_ACTIVE_DEVICE);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove sle last ASC active device failed");
    }
    HILOGI("Remove last ASC active ret : %{public}d", ret);
    return ret;
}

bool SleConfig::SetLastASCConnectedDevice(const std::string &device) const
{
    if (!IsValidAddress(device)) {
        return false;
    }
    if (!config_->SetValue(SECTION_LAST_ASC_CONNECTED_DEVICE, PROPERTY_LAST_ASC_ACT_CONN_RAW_ADDRESS, device)) {
        LOG_WARN("[SleConfig] failed!");
        return false;
    }
    return true;
}

std::string SleConfig::GetLastASCConnectedDevice() const
{
    LOG_DEBUG("[SleConfig]");

    std::string device = "";
    bool ret = config_->GetValue(SECTION_LAST_ASC_CONNECTED_DEVICE, PROPERTY_LAST_ASC_ACT_CONN_RAW_ADDRESS, device);
    if (!ret) {
        LOG_DEBUG("[SleConfig] Get sle last ASC connected device failed");
    }
    return device;
}

bool SleConfig::RemoveLastASCConnectedDevice() const
{
    LOG_DEBUG("[SleConfig]");

    bool ret = config_->RemoveSection(SECTION_LAST_ASC_CONNECTED_DEVICE);
    if (!ret) {
        LOG_ERROR("[SleConfig] Remove sle last ASC connected device failed");
    }
    HILOGI("Remove last ASC connected ret : %{public}d", ret);
    return ret;
}

bool SleConfig::SetReconnectDeviceAddressList(const std::string &addressList) const
{
    LOG_DEBUG("[SleConfig]");
    bool ret = config_->SetValue(SECTION_SLE_BG_RECONN_QUEUE, PROPERTY_BG_RECONN_QUEUE_INFO, addressList);
    if (!ret) {
        LOG_ERROR("[SleConfig] Set sle bg reconn device address list failed, ret = %{public}d", ret);
        return false;
    }
    return true;
}

std::string SleConfig::GetReconnectDeviceAddressList() const
{
    LOG_DEBUG("[SleConfig]");
    std::string addressList = "";
    bool ret = config_->GetValue(SECTION_SLE_BG_RECONN_QUEUE, PROPERTY_BG_RECONN_QUEUE_INFO, addressList);
    if (!ret) {
        LOG_ERROR("[SleConfig] Get sle bg reconn device address list failed, ret = %{public}d", ret);
    }
    return addressList;
}

}  // namespace Nearlink
}  // namespace OHOS
