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
#include <map>
#include <dlfcn.h>

#include "TwsDefines.h"
#include "TwsMessage.h"
#include "TwsService.h"
#include "TwsSharedLibApi.h"

namespace OHOS {
namespace Nearlink {

TwsSharedLibApi &TwsSharedLibApi::GetInstance() /* 单例运行 */
{
    // C++11 static local variable initialization is thread-safe.
    static TwsSharedLibApi hiboxInstance;
    return hiboxInstance;
}

/* 动态库加载 & 函数指针读取 */
bool TwsSharedLibApi::Init()
{
    if (libHandle_ != nullptr) {
        HILOGD("[Tws LibApi]:encode/decode lib already load.");
        return true;
    }

    /* 加载动态库 */
    libHandle_ = dlopen(TWS_ENCODE_DECODE_LIB_NAME.c_str(), RTLD_NOW);
    NL_CHECK_RETURN_RET(libHandle_, false, "[Tws LibApi]:load encode/decode lib failed.");

    /* 清空之前的错误 */
    dlerror();

    const std::vector<std::string> TWS_ENCODE_DECODE_FUNC_LIST = {
        TWS_LIB_INTERFACE_RECV_REQ,
        TWS_LIB_INTERFACE_RECV_RSP,
        TWS_LIB_INTERFACE_SEND_REQ,
        TWS_LIB_INTERFACE_SEND_RSP,
        TWS_LIB_INTERFACE_CALLBACK_REG,
    };

    /* 获取符号表句柄 */
    for (const auto &funcName : TWS_ENCODE_DECODE_FUNC_LIST) {
        void *funcHandle = dlsym(libHandle_, funcName.c_str());
        const char* dlsymError = dlerror();
        if (funcHandle == nullptr || dlsymError != nullptr) {
            HILOGE("[Tws LibApi]:load lib func:%{public}s failed:%{public}s.", funcName.c_str(), dlsymError);
            dlclose(libHandle_);
            libHandle_ = nullptr;
            libFuncHandle_.Clear();
            return false;
        }

        libFuncHandle_.EnsureInsert(funcName, funcHandle);
    }

    /* 动态库加载完成 */
    HILOGI("[Tws LibApi]:load encode/decode lib:%{public}s & funcs success.", TWS_ENCODE_DECODE_LIB_NAME.c_str());

    /* 符号表读取完成，注册回调指针给动态库 */
    if (!InitLibCallBack()) {
        HILOGE("[Tws LibApi]:register library callback failed.");
        dlclose(libHandle_);
        libHandle_ = nullptr;
        libFuncHandle_.Clear();
        return false;
    }
    return true;
}

/* 动态库加载：去初始化 */
void TwsSharedLibApi::DeInit()
{
    if (libHandle_) {
        dlclose(libHandle_);
        libHandle_ = nullptr;
    }

    libFuncHandle_.Clear();
}

// 根据函数名调用函数指针
template<typename FuncType>
FuncType TwsSharedLibApi::GetFuncHandleByName(const std::string& funcName)
{
    void* funcHandle = nullptr;
    libFuncHandle_.GetValue(funcName, funcHandle);
    NL_CHECK_RETURN_RET(funcHandle, nullptr,
        "[Tws LibApi]:Get func handle by name:%{public}s failed!", funcName.c_str());

    return reinterpret_cast<FuncType>(funcHandle);
}

/* 提取发送消息编码 */
void TwsSharedLibApi::PostSendMsgEventToService(TwsMessage event, uint8_t *data, uint16_t dataLen)
{
    event.dataStream_ = std::make_unique<uint8_t[]>(dataLen);
    event.streamLen_ = dataLen;
    (void)memcpy_s(event.dataStream_.get(), event.streamLen_, data, dataLen);

    /* step2: tws服务发送数据 */
    TwsService *twsInstance = TwsService::GetService();
    NL_CHECK_RETURN(twsInstance, "[Tws LibApi]:send encode request message fail,tws service instance invalid!");

    twsInstance->ProcessEvent(event);
}

/* 提取接收消息编码 */
void TwsSharedLibApi::PostRecvMsgEventToService(TwsMessage event, uint8_t *data, uint16_t dataLen)
{
    NL_CHECK_RETURN(dataLen >= sizeof(TwsEncodeMsg),
        "[Tws LibApi]:recv decode message fail,message length invalid:%{public}u!", dataLen);

    TwsEncodeMsg *decodeMsg =  reinterpret_cast<TwsEncodeMsg *>(data);
    NL_CHECK_RETURN(decodeMsg != nullptr, "[Tws LibApi]:decode message invalid");
    event.msgType_ = decodeMsg->msgType;
    event.serviceDataLen_ = decodeMsg->dataLen;
    event.serviceData_ = std::make_unique<uint8_t[]>(event.serviceDataLen_);
    (void)memcpy_s(event.serviceData_.get(), event.serviceDataLen_, decodeMsg->data, decodeMsg->dataLen);

    /* 交由service处理 */
    TwsService *twsService = TwsService::GetService();
    NL_CHECK_RETURN(twsService, "[Tws LibApi]:recv decode message fail,tws service instance invalid!");

    twsService->PostEvent(event);
}

/* 动态库交互，回调：（手机发送）请求消息编码结果 */
void TwsSharedLibApi::GetEncodeReqMsgResult(uint8_t *args, uint8_t *reqData, uint16_t dataLen)
{
    NL_CHECK_RETURN(dataLen != 0 && reqData && args,
        "[Tws LibApi]:send encode request message fail,param invalid,data len:%{public}u!", dataLen);

    TwsSharedLibMsgArgs *encodeArgs = reinterpret_cast<TwsSharedLibMsgArgs *>(args);
    TwsMessage reqMsg(TWS_SERVICE_SSAP_SEND_DATA_EVENT);
    RawAddress peerAddr = RawAddress::ConvertToString(encodeArgs->peerAddr.addr, SLE_ADDR_LEN);
    reqMsg.dev_ = peerAddr.GetAddress();
    reqMsg.msgDirect_ = TWS_MSG_DIRECT_REQ; /* 请求消息 */
    PostSendMsgEventToService(reqMsg, reqData, dataLen);
}

/* 动态库交互，回调：（手机发送）响应消息编码结果 */
void TwsSharedLibApi::GetEncodeRspMsgResult(uint8_t *args, uint8_t *rspData, uint16_t dataLen)
{
    /* args当前传参是地址 */
    NL_CHECK_RETURN(dataLen != 0 && rspData && args,
        "[Tws LibApi]:send encode response message fail,param invalid,data len:%{public}u!", dataLen);

    /* step1: 拼接消息到Event */
    TwsSharedLibMsgArgs *encodeArgs = reinterpret_cast<TwsSharedLibMsgArgs *>(args);
    RawAddress peerAddr = RawAddress::ConvertToString(encodeArgs->peerAddr.addr, SLE_ADDR_LEN);
    TwsMessage rspMsg(TWS_SERVICE_SSAP_SEND_DATA_EVENT);
    rspMsg.dev_ = peerAddr.GetAddress();
    rspMsg.msgDirect_ = TWS_MSG_DIRECT_RSP; /* 请求消息 */

    PostSendMsgEventToService(rspMsg, rspData, dataLen);
}

/* 动态库交互，回调：（手机接收）收到的请求消息解码结果 */
void TwsSharedLibApi::GetRecvReqMsgDecodeResult(uint8_t echoType, uint8_t *data, uint16_t dataLen, uint8_t *args)
{
    NL_CHECK_RETURN(data && dataLen != 0 && args && echoType == TWS_MSG_DIRECT_REQ,
        "[Tws LibApi]:recv request decode fail,param invalid,data len:%{public}u!", dataLen);

    TwsSharedLibMsgArgs *decodeArgs = reinterpret_cast<TwsSharedLibMsgArgs *>(args);
    TwsMessage reqMsg(TWS_SERVICE_RECV_REQ_EVENT);
    RawAddress peerAddr = RawAddress::ConvertToString(decodeArgs->peerAddr.addr, SLE_ADDR_LEN);
    reqMsg.dev_ = peerAddr.GetAddress();
    reqMsg.msgDirect_ = TWS_MSG_DIRECT_REQ;
    reqMsg.isNeedRsp_ = true; /* HFP协议要求：请求消息需要有响应 */

    PostRecvMsgEventToService(reqMsg, data, dataLen);
}

/* 动态库交互，回调：（手机接收）收到的响应消息解码结果 */
void TwsSharedLibApi::GetRecvRspMsgDecodeResult(uint8_t echoType, uint8_t *data, uint16_t dataLen, uint8_t *args)
{
    NL_CHECK_RETURN(data && dataLen != 0 && args && echoType == TWS_MSG_DIRECT_RSP,
        "[Tws LibApi]:recv response decode fail,param invalid,data len:%{public}u!", dataLen);

    TwsSharedLibMsgArgs *decodeArgs = reinterpret_cast<TwsSharedLibMsgArgs *>(args);
    TwsMessage rspMsg(TWS_SERVICE_RECV_RSP_EVENT);
    rspMsg.msgDirect_ = TWS_MSG_DIRECT_RSP;
    RawAddress peerAddr = RawAddress::ConvertToString(decodeArgs->peerAddr.addr, SLE_ADDR_LEN);
    rspMsg.dev_ = peerAddr.GetAddress();
    rspMsg.isNeedRsp_ = false;

    PostRecvMsgEventToService(rspMsg, data, dataLen);
}

/* 动态库交互，注册回调 */
bool TwsSharedLibApi::InitLibCallBack()
{
    auto regCallBack = GetFuncHandleByName<LibCallbackReg>(TWS_LIB_INTERFACE_CALLBACK_REG);
    NL_CHECK_RETURN_RET(regCallBack, false, "[Tws LibApi]:register lib callback list failed,func null,name:%{public}s!",
        TWS_LIB_INTERFACE_CALLBACK_REG.c_str());

    HiboxRegisterFunc libCbkList;
    libCbkList.sendReqFunc = &TwsSharedLibApi::GetEncodeReqMsgResult;
    libCbkList.sendRspFunc = &TwsSharedLibApi::GetEncodeRspMsgResult;
    libCbkList.msgIndFunc = &TwsSharedLibApi::GetRecvReqMsgDecodeResult;
    libCbkList.msgCfmFunc = &TwsSharedLibApi::GetRecvRspMsgDecodeResult;
    regCallBack(&libCbkList);

    return true;
}

/* 对外接口：（手机发送）编码请求/响应消息 */
bool TwsSharedLibApi::EncodeMessage(const TwsMessage event)
{
    NL_CHECK_RETURN_RET(event.serviceDataLen_ != 0, false,
        "[Tws LibApi]:encode request/response message failed,length invalid!");

    LibFuncEncodeMsg encodeMsgHandle = nullptr;
    switch (event.msgDirect_) {
        case TWS_MSG_DIRECT_REQ:
            encodeMsgHandle = GetFuncHandleByName<LibFuncEncodeMsg>(TWS_LIB_INTERFACE_SEND_REQ);
            break;
        case TWS_MSG_DIRECT_RSP:
            encodeMsgHandle = GetFuncHandleByName<LibFuncEncodeMsg>(TWS_LIB_INTERFACE_SEND_RSP);
            break;
        default:
            HILOGE("[Tws LibApi]:encode message failed,message direct:%{public}u invalid!", event.msgDirect_);
            return false;
    }

    /* 按编解码库消息头拼接数据 */
    uint16_t encodeLen = sizeof(TwsEncodeMsg) + event.serviceDataLen_;
    TwsEncodeMsg *encodeSrc = reinterpret_cast<TwsEncodeMsg *>(new(std::nothrow)uint8_t[encodeLen]);
    NL_CHECK_RETURN_RET(encodeSrc, false, "[Tws LibApi]:encode request/response message fail,alloc new mem fail!");
    encodeSrc->msgType = event.msgType_;
    encodeSrc->dataLen = event.serviceDataLen_;
    errno_t secRet = memcpy_s(encodeSrc->data, encodeSrc->dataLen, event.serviceData_.get(), event.serviceDataLen_);
    if (secRet != EOK) {
        HILOGE("[Tws LibApi]:encode request/response message fail,memcpy data fail:%{public}d!", secRet);
        delete[] encodeSrc;
        return false;
    }

    /* 编码消息接口，args为对端地址 */
    RawAddress peerAddr(event.dev_);
    TwsSharedLibMsgArgs msgArgs;
    peerAddr.ConvertToUint8(msgArgs.peerAddr.addr, SLE_ADDR_LEN);
    if (encodeMsgHandle == nullptr) {
        HILOGE("[Tws LibApi]:get encode req func handle by name failed,func type:%{public}u!", event.msgDirect_);
        delete[] encodeSrc;
        return false;
    }
    HILOGD("[Tws LibApi]:start encode msg,addr:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
    bool ret = encodeMsgHandle(reinterpret_cast<uint8_t *>(&msgArgs),
        reinterpret_cast<uint8_t *>(encodeSrc), encodeLen);
    delete[] encodeSrc;
    return ret;
}

/* 对外接口：（手机接收）解码请求/响应消息 */
bool TwsSharedLibApi::DecodeMessage(const TwsMessage event)
{
    NL_CHECK_RETURN_RET(event.streamLen_ != 0, false,
        "[Tws LibApi]:decode request/response message failed,length invalid!");

    LibFuncDecodeMsg decodeMsgHandle = nullptr;
    switch (event.msgDirect_) {
        case TWS_MSG_DIRECT_REQ:
            decodeMsgHandle = GetFuncHandleByName<LibFuncDecodeMsg>(TWS_LIB_INTERFACE_RECV_REQ);
            break;
        case TWS_MSG_DIRECT_RSP:
            decodeMsgHandle = GetFuncHandleByName<LibFuncDecodeMsg>(TWS_LIB_INTERFACE_RECV_RSP);
            break;
        default:
            HILOGE("[Tws LibApi]:decode message failed,message direct:%{public}u invalid!", event.msgDirect_);
            return false;
    }

    NL_CHECK_RETURN_RET(decodeMsgHandle, false,
        "[Tws LibApi]:get decode func handle by name failed,func type:%{public}u!", event.msgDirect_);

    RawAddress peerAddr(event.dev_);
    TwsSharedLibMsgArgs msgArgs;
    peerAddr.ConvertToUint8(msgArgs.peerAddr.addr, SLE_ADDR_LEN);

    HILOGD("[Tws LibApi]:start decode msg,addr:%{public}s", GET_ENCRYPT_ADDR(peerAddr));

    /* 解码请求/响应消息，args传参是对端地址 */
    return decodeMsgHandle(event.dataStream_.get(), static_cast<uint16_t>(event.streamLen_),
        reinterpret_cast<uint8_t *>(&msgArgs));
}

} // namespace Sle
} // namespace OHOS