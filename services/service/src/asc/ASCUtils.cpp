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

#include "ASCUtils.h"
#include "log.h"
#include "actm_api_type.h"
#include "cm_api.h"
#include "nearlink_dft_ue.h"

namespace OHOS {
namespace Nearlink {
uint8_t ASCUtils::GetEncodeSoundChannelNum()
{
    /*
        编码输出声道数 0：单声道 1：双声道
        业务类型               编码输出声道数
        媒体    音箱媒体        1
               多个音箱媒体     0
               耳机媒体         0
        通话    组播通话        0
               单播通话         0
    */
    return 0;
}

uint8_t ASCUtils::GetDecodeSoundChannelNum()
{
    // 解码输入声道数 0：单声道 1：双声道
    // 通话单声道，音乐目前起播单声道160k
    return 0;
}

uint8_t ASCUtils::GetCosetDeviceNum(const RawAddress& device)
{
    // 合作集内设备数
    return COSET_MAX_NUM;
}

uint8_t ASCUtils::GetEncodeCodecClan(uint8_t codecId)
{
    return (codecId == ASC_CODEC_ID_L2HC_VOICE) ? DSP_CODEC_CLAN_L2HC_VOICE : DSP_CODEC_CLAN_L2HC;
}

uint8_t ASCUtils::GetDecodeCodecClan(const RawAddress& device, const ASCToDSPInfo &ascToDspInfo, Qos cos)
{
    if (cos == NL_SLE_QOS_5) {
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, DUAL_REC_START, DUAL_REC_NOTIFY_DSP);
        return DSP_CODEC_CLAN_L2HC_VOICE;
    }
    // 默认上下行一致
    return ascToDspInfo.encodeCodecClan;
}

uint8_t ASCUtils::GetCustomizeDownChannelNum(uint8_t pointType, uint8_t enCodecId)
{
    uint8_t downChannelNum = 0;
    // 单播/组播：媒体0 通话1
    uint8_t mcast = (enCodecId == ASC_CODEC_ID_L2HC_VOICE)
        ? ASC_COMM_TYPE_MULTICAST : ASC_COMM_TYPE_UNICAST;
    if (pointType == NLSTK_ACTM_ALL_POINT) {
        downChannelNum = (mcast == ASC_COMM_TYPE_MULTICAST) ? ASC_ICB_NUM_ONE : ASC_ICB_NUM_TWO;
    } else if (pointType == NLSTK_ACTM_SINK_POINT) {
        downChannelNum = (mcast == ASC_COMM_TYPE_MULTICAST) ? ASC_ICB_NUM_ONE : ASC_ICB_NUM_TWO;
    } else if (pointType == NLSTK_ACTM_SOURCE_POINT) {
        downChannelNum = 0; // 不需要编码
    } else {
        HILOGE("[ASCUtils] pointType illegeal %{public}d", pointType);
    }
    return downChannelNum;
}

uint8_t ASCUtils::GetCustomizeUpChannelNum(uint8_t pointType, Qos cos)
{
    uint8_t upChannelNum = 0;
    if (cos == NL_SLE_QOS_5) {
        upChannelNum = ASC_ICB_NUM_TWO; // 双耳录音
    } else if (pointType == NLSTK_ACTM_ALL_POINT) {
        upChannelNum = ASC_ICB_NUM_ONE;
    } else if (pointType == NLSTK_ACTM_SINK_POINT) {
        upChannelNum = 0; // 不需要解码
    } else if (pointType == NLSTK_ACTM_SOURCE_POINT) {
        upChannelNum = ASC_ICB_NUM_ONE; // 三方录音
    } else {
        HILOGE("[ASCUtils] pointType illegeal %{public}d", pointType);
    }
    return upChannelNum;
}

uint8_t ASCUtils::GetBpsBitIndexByBps(uint16_t bps)
{
    uint8_t index = 0;
    switch (bps) {
        case BPS_48:
            index = L2HC_BPS_S_48_BIT;
            break;
        case BPS_64:
            index = L2HC_BPS_S_64_BIT;
            break;
        case BPS_96:
            index = L2HC_BPS_S_96_BIT;
            break;
        case BPS_128:
            index = L2HC_BPS_S_128_BIT;
            break;
        case BPS_160:
            index = L2HC_BPS_S_160_BIT;
            break;
        case BPS_240:
            index = L2HC_BPS_S_240_BIT;
            break;
        case BPS_320:
            index = L2HC_BPS_S_320_BIT;
            break;
        case BPS_480:
            index = L2HC_BPS_S_480_BIT;
            break;
        case BPS_750:
            index = L2HC_BPS_S_750_BIT;
            break;
        case BPS_960:
            index = L2HC_BPS_S_960_BIT;
            break;
        case BPS_1150:
            index = L2HC_BPS_S_1150_BIT;
            break;
        case BPS_2300:
            index = L2HC_BPS_S_2300_BIT;
            break;
        default:
            index = L2HC_BPS_S_64_BIT;
            HILOGE("[ASCUtils]GetBpsBitIndexByBps error bps %{public}d", bps);
            break;
    }

    return index;
}

void ASCUtils::SetQosmInfo(const NLSTK_ActmQosmInfo_S& infoActm, AscQosmInfo& infoAsc)
{
    infoAsc.qosIndex          = infoActm.qosIndex;
    infoAsc.codecId           = infoActm.codecId;
    infoAsc.version           = infoActm.version;
    infoAsc.frame             = infoActm.frame;
    infoAsc.bitSamp           = infoActm.bitSamp;
    infoAsc.channelMode       = infoActm.channelMode;
    infoAsc.linkCnt           = infoActm.linkCnt;
    infoAsc.sca               = infoActm.sca;
    infoAsc.packing           = infoActm.packing;
    infoAsc.framing           = infoActm.framing;
    infoAsc.ft                = infoActm.ft;
    infoAsc.rtn               = infoActm.rtn;
    infoAsc.nse               = infoActm.nse;
    infoAsc.bn                = infoActm.bn;
    infoAsc.phy               = infoActm.phy;
    infoAsc.mcs               = infoActm.mcs;
    infoAsc.pilot             = infoActm.pilot;
    infoAsc.frameType         = infoActm.frameType;
    infoAsc.companyId         = infoActm.companyId;
    infoAsc.vendorId         = infoActm.vendorId;
    infoAsc.bps               = infoActm.bps;
    infoAsc.gHandle           = infoActm.gHandle;
    infoAsc.connHandle        = infoActm.connHandle;
    infoAsc.sduInterval       = infoActm.sduInterval;
    infoAsc.maxSdu            = infoActm.maxSdu;
    infoAsc.maxPdu            = infoActm.maxPdu;
    infoAsc.bufNum            = infoActm.bufNum;
    infoAsc.maxLatency        = infoActm.maxLatency;
    infoAsc.icbInterval       = infoActm.icbInterval;
    infoAsc.rate              = infoActm.rate;
}

bool ASCUtils::IsNeedSetDirection(Qos qos)
{
    if ((qos == NL_SLE_QOS_3) || (qos == NL_SLE_QOS_5) || (qos == NL_SLE_QOS_6)) {
        return true;
    }

    return false;
}

uint8_t ASCUtils::GetDirection(bool isRolePrimary, Qos qos)
{
    uint8_t derection = NLSTK_ACTM_DIRECTION_BOTH;
    switch (qos) {
        case NL_SLE_QOS_1:
            derection = NLSTK_ACTM_DIRECTION_DOWN;
            break;
        case NL_SLE_QOS_2:
            derection = NLSTK_ACTM_DIRECTION_DOWN;
            break;
        case NL_SLE_QOS_3:
            derection = isRolePrimary ? NLSTK_ACTM_DIRECTION_BOTH : NLSTK_ACTM_DIRECTION_DOWN;
            break;
        case NL_SLE_QOS_4:
            derection = NLSTK_ACTM_DIRECTION_BOTH;
            break;
        case NL_SLE_QOS_5:
            derection = NLSTK_ACTM_DIRECTION_BOTH;
            break;
        case NL_SLE_QOS_6:
            derection = isRolePrimary ? NLSTK_ACTM_DIRECTION_BOTH : NLSTK_ACTM_DIRECTION_DOWN;
            break;
        case NL_SLE_QOS_7:
            derection = NLSTK_ACTM_DIRECTION_DOWN;
            break;
        case NL_SLE_QOS_9:
            derection = NLSTK_ACTM_DIRECTION_BOTH;
            break;
        case NL_SLE_QOS_10:
            derection = NLSTK_ACTM_DIRECTION_DOWN;
            break;
        default:
            HILOGE("[ASCService]GetDirection qos illegeal %{public}d", qos);
            break;
    }

    return derection;
}

void ASCUtils::TransferProperty(const RawAddress& device, uint8_t num, NLSTK_ActmProp_S *prop,
    std::vector<AscProp>& properties)
{
    HILOGD("[ASCService]TransferProperty %{public}s num %{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), num);

    properties.reserve(num);
    for (uint8_t i = 0; i < num; i++) {
        NLSTK_ActmProp_S* actmProp = &(prop[i]);
        AscProp ascProp {};
        ascProp.pointType = actmProp->pointType;

        NLSTK_ActmAbility_S* actmAbility = &(actmProp->ability);
        AscAbility* ascAbility = &(ascProp.ability);
        ascAbility->codecNum = actmAbility->codecNum;

        for (uint8_t j = 0; j < ascAbility->codecNum; j++) {
            NLSTK_ActmCodecParam_S* actmCodec = &(actmAbility->codec[j]);
            AscCodecConfig* ascCodec = &(ascAbility->codec[j]);
            ascCodec->codecId = actmCodec->codecId;
            ascCodec->companyId = actmCodec->companyId;
            ascCodec->vendorId = actmCodec->vendorId;
            // 0x0: PCM(暂不支持), 0x1: L2HC, 0x2: L2HC_Voice
            if (ascCodec->codecId == ASC_CODEC_ID_PCM) {
                HILOGE("[ASCService]TransferProperty in %{public}s codecId %{public}d is PCM",
                    GetEncryptAddr(device.GetAddress()).c_str(), ascCodec->codecId);
                // 数据非法，直接返回，不再继续解析和保存
                return;
            } else if ((ascCodec->codecId == ASC_CODEC_ID_L2HC) || (ascCodec->codecId == ASC_CODEC_ID_L2HC_PRI) ||
                (ascCodec->codecId == ASC_CODEC_ID_L2HC_VOICE)) {
                AscL2HCParam* ascL2HCParam = &(ascCodec->param.l2hcParam);
                NLSTK_L2HCParam_S* actmL2HCParam = &(actmCodec->l2hc);
                ascL2HCParam->version = actmL2HCParam->version;
                ascL2HCParam->rate = actmL2HCParam->rate;
                ascL2HCParam->depth = actmL2HCParam->depth;
                ascL2HCParam->channel = actmL2HCParam->channel;
                ascL2HCParam->frame = actmL2HCParam->frame;
                ascL2HCParam->bps = actmL2HCParam->bps;
            } else {
                HILOGE("[ASCService]TransferProperty in %{public}s codecId %{public}d is illegal",
                    GetEncryptAddr(device.GetAddress()).c_str(), ascCodec->codecId);
                // 数据非法，直接返回，不再继续解析和保存
                return;
            }
        }

        ascAbility->comm = actmAbility->comm;
        ascAbility->supportType = actmAbility->supportType;

        properties.emplace_back(ascProp);
    }
}

uint8_t ASCUtils::GetDecodeBitDepth(const ASCToDSPInfo &ascToDspInfo, Qos cos)
{
    if (cos == NL_SLE_QOS_5) {
        return static_cast<uint8_t>(DSP_BITS_DEPTH_16); // 位宽
    }
    // 默认上下行一致
    return ascToDspInfo.encodeBitDepth;
}

uint32_t ASCUtils::GetDecodeSampleRate(const ASCToDSPInfo &ascToDspInfo, Qos cos)
{
    if (cos == NL_SLE_QOS_5) {
        return static_cast<uint32_t>(DSP_SAMPLE_RATE_48000); // 采样率
    }
    // 默认上下行一致
    return ascToDspInfo.encodeSampleRate;
}

// 通话Autorate 目标码率对应发给对端的码率位图
uint32_t ASCUtils::GetAutorateTargetBpsRange(uint16_t targetBps)
{
    uint64_t callAutorateDefineBps = BPS_QOS3_AUTORATE_DOWN;
    std::map<uint16_t, uint64_t> l2hcBpsBitMap = {
        {BPS_128, L2HC_VOICE_BPS_S_128},
        {BPS_96,  L2HC_VOICE_BPS_S_96},
        {BPS_64,  L2HC_VOICE_BPS_S_64},
        {BPS_48,  L2HC_VOICE_BPS_S_48},
        {BPS_32,  L2HC_VOICE_BPS_S_32},
    };
    if (l2hcBpsBitMap.find(targetBps) == l2hcBpsBitMap.end()) {
        HILOGE("input targetBps = %{public}d error", targetBps);
        return callAutorateDefineBps;
    }
    uint64_t targetBpsRange = 0;
    for (auto it = l2hcBpsBitMap.begin(); it != l2hcBpsBitMap.end(); it++) {
        if (it->first <= targetBps) {
            targetBpsRange |= (it->second);
        }
    }
    return callAutorateDefineBps & targetBpsRange;
}

// 通话Autorate 切码率对应的帧格式
uint8_t ASCUtils::GetAutorateTargetFrameType(bool isLevelUp)
{
    return isLevelUp ? CM_RADIO_FRAME_TYPE_1 : CM_RADIO_FRAME_TYPE_4_M0;
}

// 通话Autorate 切码率对应的物理层带宽
uint8_t ASCUtils::GetAutorateTargetPhyType(bool isLevelUp)
{
    return isLevelUp ? CM_PHY_TYPE_1M : CM_PHY_TYPE_2M;
}

}  // namespace Nearlink
}  // namespace OHOS