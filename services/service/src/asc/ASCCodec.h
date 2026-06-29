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
#ifndef ASC_CODEC_H
#define ASC_CODEC_H

namespace OHOS {
namespace Nearlink {
/* --------------编解码------------- */
constexpr uint8_t ASC_CODEC_NUM_MAX = 4;      // PCM/L2HC/L2HC_Voice/L2HC_私有(终端L2HC_5.0+HISI_L2HC_4.0)
// 编解码器类型
enum ASCCodecId {
    ASC_CODEC_ID_PCM = 0x00,
    ASC_CODEC_ID_L2HC = 0x01,
    ASC_CODEC_ID_L2HC_VOICE = 0x02,
    ASC_CODEC_ID_L2HC_PRI = 0xFF,
};
// 厂商标识
enum ASCCompanyId {
    ASC_COMPANY_ID_STANDARD = 0x0000,      // PCM/L2HC/L2HC_Voice
    ASC_COMPANY_ID_VENDOR = 0x0009,        // L2HC_5_0
    ASC_COMPANY_ID_HISI = 0x007C,          // L2HC_4_0
};
// 厂商编解码器标识
enum ASCVendorId {
    ASC_VENDOR_ID_STANDARD = 0x0000,       // PCM/L2HC/L2HC_Voice
    ASC_VENDOR_ID_L2HC_5_0 = 0x0001,
    ASC_VENDOR_ID_L2HC_4_0_HISI = 0x0001,
};
// 协议版本号
enum ASCVersion {
    ASC_L2HC_VERSION_STANDARD = 1,
    ASC_L2HC_5_VERSION = 2,
    ASC_L2HC_4_0_HISI_VERSION = 1,
};
enum ASCSampleRate {
    ASC_SAMPLE_RATE_16KHZ = 0x02,
    ASC_SAMPLE_RATE_32KHZ = 0x05,
    ASC_SAMPLE_RATE_44KHZ = 0x06,
    ASC_SAMPLE_RATE_48KHZ = 0x07,
    ASC_SAMPLE_RATE_88KHZ = 0x08,
    ASC_SAMPLE_RATE_96KHZ = 0x09,
};
enum ASCSampleBitDepth {
    ASC_SAMPLE_DEPTH_16BIT = 0x01,       // 采样深度 16bit
    ASC_SAMPLE_DEPTH_24BIT = 0x02,       // 采样深度 24bit
    ASC_SAMPLE_DEPTH_32BIT = 0x03,       // 采样深度 32bit
};
enum ASCChannelType {
    ASC_CHANNEL_SINGLE = 0x01,           // 单声道
    ASC_CHANNEL_DOUBLE = 0x02,           // 双声道立体声
};
enum ASCFrameLen {
    ASC_L2HC_FRAME_5MS = 3,              // 帧长模式 5ms
    ASC_L2HC_FRAME_10MS = 4,             // 帧长模式 10ms
};

// 转译映射DSP version, 与Audio DSP仓中的枚举保持一致
enum DSPL2hcVersion {
    DSP_L2HC_STD_VERSION = 1,
    DSP_L2HC_5_0_VERSION = 2,
    DSP_L2HC_4_0_VERSION = 3,
    DSP_L2HC_VOICE_VERSION = 4,
};
// 映射DSP采样深度
enum DSPBitDepth {
    DSP_BITS_DEPTH_16 = 16,
    DSP_BITS_DEPTH_24 = 24,
    DSP_BITS_DEPTH_32 = 32,
};
// 映射DSP采样率
enum DSPSampleRate {
    DSP_SAMPLE_RATE_16000 = 16000,
    DSP_SAMPLE_RATE_32000 = 32000,
    DSP_SAMPLE_RATE_48000 = 48000,
    DSP_SAMPLE_RATE_96000 = 96000,
};
// 映射DSP 编码器家族 L2HC(L2HC 2.0\L2HC 4.0\L2HC 5.0), L2HC_Voice
enum DSPCodecClan{
    DSP_CODEC_CLAN_L2HC = 0,
    DSP_CODEC_CLAN_L2HC_VOICE = 1,
};

// 双耳录音的上下行通道数量
constexpr uint8_t DSP_DUAL_REC_DOWN_CHANNEL_NUM = 2;
constexpr uint8_t DSP_DUAL_REC_UP_CHANNEL_NUM = 2;
/* --------------编码速率-------------
 * 以位图的形式表示对不同编码速率的支持，值为1表示支持，值为0表示不支持
 ------------------------------------*/
// L2HC_Voice
// bit0：单声道RFU
// bit1：单声道8kbps
constexpr uint64_t L2HC_VOICE_BPS_S_8             = 0x0000000000000002;
// bit2：单声道16kbps
constexpr uint64_t L2HC_VOICE_BPS_S_16            = 0x0000000000000004;
// bit3：单声道32kbps
constexpr uint64_t L2HC_VOICE_BPS_S_32            = 0x0000000000000008;
// bit4：单声道48kbps
constexpr uint64_t L2HC_VOICE_BPS_S_48            = 0x0000000000000010;
// bit5：单声道64kbps
constexpr uint64_t L2HC_VOICE_BPS_S_64            = 0x0000000000000020;
constexpr uint64_t L2HC_VOICE_BPS_S_64_BIT        = 5;
// bit6：单声道96kbps
constexpr uint64_t L2HC_VOICE_BPS_S_96            = 0x0000000000000040;
// bit7：单声道128kbps
constexpr uint64_t L2HC_VOICE_BPS_S_128           = 0x0000000000000080;
// bit8~bit19：RFU
// bit20：双声道RFU
// bit21：双声道16kbps
constexpr uint64_t L2HC_VOICE_BPS_D_16            = 0x0000000000200000;
// bit22：双声道32kbps
constexpr uint64_t L2HC_VOICE_BPS_D_32            = 0x0000000000400000;
// bit23：双声道64kbps
constexpr uint64_t L2HC_VOICE_BPS_D_64            = 0x0000000000800000;
constexpr uint64_t L2HC_VOICE_BPS_D_64_BIT        = 23;
// bit24：双声道96kbps
constexpr uint64_t L2HC_VOICE_BPS_D_96            = 0x0000000001000000;
// bit25：双声道128kbps
constexpr uint64_t L2HC_VOICE_BPS_D_128           = 0x0000000002000000;
// bit26：双声道192kbps
constexpr uint64_t L2HC_VOICE_BPS_D_192           = 0x0000000004000000;
// bit27：双声道256kbps
constexpr uint64_t L2HC_VOICE_BPS_D_256           = 0x0000000008000000;
// bit28~bit38：RFU
// bit39：自适应码率
constexpr uint64_t L2HC_VOICE_BPS_D_AUTO          = 0x0000008000000000;

// L2HC
// bit0：单声道 RFU
// bit1：单声道 RFU
// bit2：单声道48kbps
constexpr uint64_t L2HC_BPS_S_48                  = 0x0000000000000004;
constexpr uint64_t L2HC_BPS_S_48_BIT              = 2;
// bit3：单声道64kbps
constexpr uint64_t L2HC_BPS_S_64                  = 0x0000000000000008;
constexpr uint64_t L2HC_BPS_S_64_BIT              = 3;
// bit4：单声道96kbps
constexpr uint64_t L2HC_BPS_S_96                  = 0x0000000000000010;
constexpr uint64_t L2HC_BPS_S_96_BIT              = 4;
// bit5：单声道128kbps
constexpr uint64_t L2HC_BPS_S_128                 = 0x0000000000000020;
constexpr uint64_t L2HC_BPS_S_128_BIT             = 5;
// bit6：单声道160kbps
constexpr uint64_t L2HC_BPS_S_160                 = 0x0000000000000040;
constexpr uint64_t L2HC_BPS_S_160_BIT             = 6;
// bit7：单声道240kbps
constexpr uint64_t L2HC_BPS_S_240                 = 0x0000000000000080;
constexpr uint64_t L2HC_BPS_S_240_BIT             = 7;
// bit8：单声道320kbps
constexpr uint64_t L2HC_BPS_S_320                 = 0x0000000000000100;
constexpr uint64_t L2HC_BPS_S_320_BIT             = 8;
// bit9：单声道480kbps
constexpr uint64_t L2HC_BPS_S_480                 = 0x0000000000000200;
constexpr uint64_t L2HC_BPS_S_480_BIT             = 9;
// bit10：单声道750kbps
constexpr uint64_t L2HC_BPS_S_750                 = 0x0000000000000400;
constexpr uint64_t L2HC_BPS_S_750_BIT             = 10;
// bit11：单声道960kbps
constexpr uint64_t L2HC_BPS_S_960                 = 0x0000000000000800;
constexpr uint64_t L2HC_BPS_S_960_BIT             = 11;
// bit12：单声道1150kbps
constexpr uint64_t L2HC_BPS_S_1150                = 0x0000000000001000;
constexpr uint64_t L2HC_BPS_S_1150_BIT            = 12;
// bit13：单声道2300kbps
constexpr uint64_t L2HC_BPS_S_2300                = 0x0000000000002000;
constexpr uint64_t L2HC_BPS_S_2300_BIT            = 13;
// bit14~bit19：RFU
// bit20：双声道 RFU
// bit21：双声道 RFU
// bit22：双声道 RFU
// bit23：双声道128kbps
constexpr uint64_t L2HC_BPS_D_128                 = 0x0000000000800000;
// bit24：双声道192kbps
constexpr uint64_t L2HC_BPS_D_192                 = 0x0000000001000000;
// bit25：双声道256kbps
constexpr uint64_t L2HC_BPS_D_256                 = 0x0000000002000000;
// bit26：双声道320kbps
constexpr uint64_t L2HC_BPS_D_320                 = 0x0000000004000000;
// bit27：双声道480kbps
constexpr uint64_t L2HC_BPS_D_480                 = 0x0000000008000000;
// bit28：双声道640kbps
constexpr uint64_t L2HC_BPS_D_640                 = 0x0000000010000000;
// bit29：双声道960kbps
constexpr uint64_t L2HC_BPS_D_960                 = 0x0000000020000000;
// bit30：双声道1500kbps
constexpr uint64_t L2HC_BPS_D_1500                = 0x0000000040000000;
// bit31：双声道1920kbps
constexpr uint64_t L2HC_BPS_D_1920                = 0x0000000080000000;
// bit32：双声道2300kbps
constexpr uint64_t L2HC_BPS_D_2300                = 0x0000000100000000;
// bit33：双声道4600kbps
constexpr uint64_t L2HC_BPS_D_4600                = 0x0000000200000000;
// bit34~bit39：RFU

constexpr uint64_t BPS_32                         = 32;
constexpr uint64_t BPS_48                         = 48;
constexpr uint64_t BPS_64                         = 64;
constexpr uint64_t BPS_96                         = 96;
constexpr uint64_t BPS_128                        = 128;
constexpr uint64_t BPS_160                        = 160;
constexpr uint64_t BPS_240                        = 240;
constexpr uint64_t BPS_320                        = 320;
constexpr uint64_t BPS_480                        = 480;
constexpr uint64_t BPS_750                        = 750;
constexpr uint64_t BPS_960                        = 960;
constexpr uint64_t BPS_1150                       = 1150;
constexpr uint64_t BPS_2300                       = 2300;

// QOS支持的码率位图
// 4600/2300/1500/960/640/320/192/96kbps codec_2 L2HC
constexpr uint64_t BPS_QOS1             = (L2HC_BPS_S_2300 | L2HC_BPS_S_1150 | L2HC_BPS_S_750 | L2HC_BPS_S_480 | \
    L2HC_BPS_S_320 | L2HC_BPS_S_160 | L2HC_BPS_S_96 | L2HC_BPS_S_48);
constexpr uint64_t BPS_QOS1_NO_2300     = (L2HC_BPS_S_750 | L2HC_BPS_S_480 | L2HC_BPS_S_320 | L2HC_BPS_S_160 | \
    L2HC_BPS_S_96 | L2HC_BPS_S_48);
// 不含4.6M
constexpr uint64_t BPS_QOS1_NO_4600      = (L2HC_BPS_S_1150 | L2HC_BPS_S_750 | L2HC_BPS_S_480 | \
    L2HC_BPS_S_320 | L2HC_BPS_S_160 | L2HC_BPS_S_96 | L2HC_BPS_S_48);
// 320/192/96bps codec_2 L2HC
constexpr uint64_t BPS_QOS2             = (L2HC_BPS_S_160 | L2HC_BPS_S_96 | L2HC_BPS_S_48);
// 支持码率自适应,包含32k 上行：64/32kbps codec_1 L2HC_VOICE
constexpr uint64_t BPS_QOS3_AUTORATE_UP          = (L2HC_VOICE_BPS_S_64 | L2HC_VOICE_BPS_S_32);
// 支持码率自适应,包含32k 下行：64/32kbps codec_1 L2HC_VOICE
constexpr uint64_t BPS_QOS3_AUTORATE_DOWN        = (L2HC_VOICE_BPS_S_64 | L2HC_VOICE_BPS_S_32);
// 上行：64kbps codec_1 L2HC_VOICE
constexpr uint64_t BPS_QOS3_UP          = L2HC_VOICE_BPS_S_64;
// 下行：64kbps codec_1 L2HC_VOICE
constexpr uint64_t BPS_QOS3_DOWN        = L2HC_VOICE_BPS_S_64;
// 上行原始数据 下行：640/320/192/96kbps codec_2 L2HC
constexpr uint64_t BPS_QOS4             = (L2HC_BPS_S_320 | L2HC_BPS_S_160 | L2HC_BPS_S_96 | L2HC_BPS_S_48);
// 上行：96kbps codec_1 L2HC_VOICE, 下行：96/48kbps codec_2 L2HC
constexpr uint64_t BPS_QOS5             = (L2HC_BPS_S_96 | L2HC_BPS_S_48);
// 单上行：64/32kbps 下行：64kbps codec_1 L2HC_VOICE
constexpr uint64_t BPS_QOS6             = (L2HC_VOICE_BPS_S_64 | L2HC_VOICE_BPS_S_32);
// 1500/960/640/320/192/96bps codec_2 L2HC
constexpr uint64_t BPS_QOS7_VIDEO       = (L2HC_BPS_S_750 | L2HC_BPS_S_480 | L2HC_BPS_S_320 | L2HC_BPS_S_160 | \
    L2HC_BPS_S_96 | L2HC_BPS_S_48);
constexpr uint64_t BPS_QOS7             = (L2HC_BPS_S_160 | L2HC_BPS_S_96 | L2HC_BPS_S_48);

// 码率位图位宽
constexpr uint64_t BPS_BITMAP_WIDTH     = 64;
// 单声道在位图中最高bit索引
constexpr uint64_t BPS_SINGLE_HIGH_BIT  = 19;
// 立体声在位图中最高bit索引
constexpr uint64_t BPS_DOUBLE_HIGH_BIT  = 39;
// code type
constexpr uint8_t ASC_CODE_TYPE_CODE_DECODE      = 2;
constexpr uint8_t ASC_CODE_TYPE_DECODE           = 1;
constexpr uint8_t ASC_CODE_TYPE_CODE             = 0;
// 双耳录音支持状态值
constexpr int8_t UNKNOWN = -1;
constexpr int8_t NOT_SUPPORTED = 0;
constexpr int8_t SUPPORTED = 1;
typedef struct {
    uint8_t codecId;                    // 关键因素之一, 编解码器标识
    uint16_t companyId;                 // 关键因素之一, 厂商标识
    uint16_t vendorId;                  // 关键因素之一, 厂商编解码器标识
    uint8_t version;                    // 版本号
} AscCodecIdKey;

constexpr AscCodecIdKey ASC_L2HC_CODEC = {
    .codecId = ASC_CODEC_ID_L2HC,
    .companyId = ASC_COMPANY_ID_STANDARD,
    .vendorId = ASC_VENDOR_ID_STANDARD,
    .version = ASC_L2HC_VERSION_STANDARD,
};

constexpr AscCodecIdKey ASC_L2HC_VOICE_CODEC = {
    .codecId = ASC_CODEC_ID_L2HC_VOICE,
    .companyId = ASC_COMPANY_ID_STANDARD,
    .vendorId = ASC_VENDOR_ID_STANDARD,
    .version = ASC_L2HC_VERSION_STANDARD,
};

constexpr AscCodecIdKey ASC_L2HC_5_0_CODEC = {
    .codecId = ASC_CODEC_ID_L2HC_PRI,
    .companyId = ASC_COMPANY_ID_VENDOR,
    .vendorId = ASC_VENDOR_ID_L2HC_5_0,
    .version = ASC_L2HC_5_VERSION,
};

constexpr AscCodecIdKey ASC_L2HC_4_0_CODEC = {
    .codecId = ASC_CODEC_ID_L2HC_PRI,
    .companyId = ASC_COMPANY_ID_HISI,
    .vendorId = ASC_VENDOR_ID_L2HC_4_0_HISI,
    .version = ASC_L2HC_4_0_HISI_VERSION,
};
// 支持默认标准l2hc 本端的采样率能力
constexpr uint16_t ASC_L2HC_SAMPLE_RATE_CAP = (
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_44KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_48KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_88KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_96KHZ)
);
// l2hc voice 本端的采样率能力
constexpr uint16_t ASC_L2HC_VOICE_SAMPLE_RATE_CAP = (
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_16KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_32KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_48KHZ)
);
// l2hc5.0 本端的采样率能力
constexpr uint16_t ASC_L2HC_5_0_SAMPLE_RATE_CAP = (
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_44KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_48KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_88KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_96KHZ)
);
// l2hc4.0 本端的采样率能力
constexpr uint16_t ASC_L2HC_4_0_SAMPLE_RATE_CAP = (
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_44KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_48KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_88KHZ) |
    static_cast<uint16_t>(1 << ASC_SAMPLE_RATE_96KHZ)
);

// 支持默认标准l2hc 本端的位深能力
constexpr uint8_t ASC_L2HC_BIT_DEPTH_CAP = (
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_16BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_24BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_32BIT)
);
// l2hc voice 本端的位深能力
constexpr uint8_t ASC_L2HC_VOICE_BIT_DEPTH_CAP = (
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_16BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_24BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_32BIT)
);
// l2hc5.0 本端的位深能力
constexpr uint8_t ASC_L2HC_5_0_BIT_DEPTH_CAP = (
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_16BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_24BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_32BIT)
);
// l2hc4.0 本端的位深能力
constexpr uint8_t ASC_L2HC_4_0_BIT_DEPTH_CAP = (
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_16BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_24BIT) |
    static_cast<uint8_t>(1 << ASC_SAMPLE_DEPTH_32BIT)
);

// 支持默认标准l2hc 本端的通道能力
constexpr uint8_t ASC_L2HC_CHANNEL_CAP = (
    static_cast<uint8_t>(ASC_CHANNEL_SINGLE) |
    static_cast<uint8_t>(ASC_CHANNEL_DOUBLE)
);
// l2hc voice 本端的通道能力
constexpr uint8_t ASC_L2HC_VOICE_CHANNEL_CAP = (
    static_cast<uint8_t>(ASC_CHANNEL_SINGLE)
);
// l2hc5.0 本端的通道能力
constexpr uint8_t ASC_L2HC_5_0_CHANNEL_CAP = (
    static_cast<uint8_t>(ASC_CHANNEL_SINGLE) |
    static_cast<uint8_t>(ASC_CHANNEL_DOUBLE)
);
// l2hc4.0 本端的通道能力
constexpr uint8_t ASC_L2HC_4_0_CHANNEL_CAP = (
    static_cast<uint8_t>(ASC_CHANNEL_SINGLE) |
    static_cast<uint8_t>(ASC_CHANNEL_DOUBLE)
);
} // namespace Nearlink
} // namespace OHOS
#endif // ASC_DEFINES_H