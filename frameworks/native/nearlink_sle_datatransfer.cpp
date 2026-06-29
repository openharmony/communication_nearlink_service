/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_sle_datatransfer.h"
#include "nearlink_datatransfer_parcel.h"
#include "nearlink_sa_manager.h"
#include "nearlink_sle_datatransfer_callback_stub.h"
#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log.h"
#include "i_nearlink_sle_datatransfer.h"
#include "nearlink_safe_map.h"
#include "sle_uuid.h"
#include <memory>
#include <thread>
#include <sys/socket.h>
#include <vector>
#include "nearlink_socket_inputstream.h"
#include "nearlink_socket_outputstream.h"
#include "nearlink_socket_manager.h"
#include "parameters.h"

namespace OHOS::Nearlink {
namespace {
constexpr int PORT_CNT_LIMIT = 120;
constexpr int STANDARD_UUID_LEN = 4;
constexpr int CUSTOM_UUID_LEN = 36;
constexpr int DATA_MAX_LEN = UINT16_MAX;
const char* const STANDARD_UUID_ICCE = "060D";
}

enum class HighSpeedSupportedType {
    HIGH_SPEED_UNKNOWN = 0,
    HIGH_SPEED_SUPPORT = 0x1,
    HIGH_SPEED_NOT_SUPPORT = 0x2
};

struct SleDataTransfer::impl : public std::enable_shared_from_this<impl> {
    impl();
    ~impl();
    void Init();
    NlErrCode CheckUuid(const std::string &uuid);
    NlErrCode CheckParams(const std::string &address, const std::string &uuid);
    void SetSocketDataTransfer(uint16_t port, const std::string &address, uint16_t mtu, int fd);
    bool ReceivedData(std::shared_ptr<InputStream> inputStream, uint16_t portId, const std::string &address);

    class NearlinkSleDataTransferCallbackImp;
    sptr<NearlinkSleDataTransferCallbackImp> callbackImp_ = nullptr;  // 底层返上来的callback需执行到外部传入的cb
    NearlinkSafeMap<uint16_t, std::weak_ptr<SleDataTransferCallback>> callbacks_;  // 外部传入的cb, 端口映射
    NearlinkSafeMap<std::string, uint16_t> uuidAndPort_;                           // uuid和端口映射
    int32_t profileRegisterId_{0};

    std::unique_ptr<PortSocketManager> portSocketManager_ = nullptr;  // 数传socket管理
    std::mutex flagMutex_;
    HighSpeedSupportedType isHighSpeedSupported_ = HighSpeedSupportedType::HIGH_SPEED_UNKNOWN;
    std::weak_ptr<SleDataTransfer::impl> sleDataTransferImplWptr_;
};

class SleDataTransfer::impl::NearlinkSleDataTransferCallbackImp : public NearlinkSleDataTransferCallbackStub {
public:
    explicit NearlinkSleDataTransferCallbackImp(std::weak_ptr<SleDataTransfer::impl> dataTransferImpl)
        : sleDataTransferImpl_(dataTransferImpl)
    {}

    ~NearlinkSleDataTransferCallbackImp()
    {}

    void OnConnectionStateChanged(const NearlinkSleDataTransferConnectionParams &connectionParams, int fd) override
    {
        HILOGD("enter");
        std::shared_ptr<SleDataTransfer::impl> sleDataTransferImplSptr = sleDataTransferImpl_.lock();
        NL_CHECK_RETURN(sleDataTransferImplSptr, "sleDataTransferImplSptr is nullptr.");

        ConnectionParams result(connectionParams.address_, connectionParams.uuid_, connectionParams.state_);
        HILOGD("Receive stack srcPort = %{public}u", connectionParams.port_);
        
        std::shared_ptr<SleDataTransferCallback> callbackSptr =
                GetDataTransferCallback(connectionParams.port_, sleDataTransferImplSptr);

        if (!callbackSptr) {
            close(fd);
            return;
        }

        if (connectionParams.GetState() == static_cast<int32_t>(SleConnectState::CONNECTED) && fd != -1) {
            // socketpair 设置本端socket, 添加工作线程需执行的数据读取函数
            sleDataTransferImplSptr->SetSocketDataTransfer(connectionParams.port_, connectionParams.address_,
                connectionParams.mtu_, fd);
        }

        HILOGI("Receive stack srcPort = %{public}u", connectionParams.port_);

        if (connectionParams.GetState() == static_cast<int32_t>(SleConnectState::DISCONNECTED)) {
            HILOGI("enter");
            sleDataTransferImplSptr->portSocketManager_->DestroyPeerPort(
                connectionParams.port_, connectionParams.address_);
        }
        callbackSptr->OnConnectionStateChanged(result);

        if (connectionParams.GetState() == static_cast<int32_t>(SleConnectState::CONNECTED) && fd != -1) {
            sleDataTransferImplSptr->portSocketManager_->Listen(connectionParams.port_, connectionParams.address_);
            sleDataTransferImplSptr->portSocketManager_->RunThread();
        }
    }

private:
    std::weak_ptr<SleDataTransfer::impl> sleDataTransferImpl_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleDataTransferCallbackImp);
    std::shared_ptr<SleDataTransferCallback> GetDataTransferCallback(
        uint16_t key, std::shared_ptr<SleDataTransfer::impl> dataTransferImpl_)
    {
        std::weak_ptr<SleDataTransferCallback> callbackWptr;
        bool ret = dataTransferImpl_->callbacks_.GetValue(key, callbackWptr);
        NL_CHECK_RETURN_RET(ret, nullptr, "SleDataTransferCallback not exist.");
        return callbackWptr.lock();
    }
};

SleDataTransfer::impl::impl()
{}

SleDataTransfer::impl::~impl()
{
    HILOGD("start");
    callbacks_.Clear();
    uuidAndPort_.Clear();
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN(proxy, "proxy is nullptr");
    NL_CHECK_RETURN(callbackImp_, "callbackImp_ is nullptr");
    proxy->DeregisterSleDataTransferCallback(callbackImp_);
}

void SleDataTransfer::impl::Init()
{
    HILOGD("enter");
    sleDataTransferImplWptr_ = shared_from_this();
    callbackImp_ = new (std::nothrow) NearlinkSleDataTransferCallbackImp(sleDataTransferImplWptr_);
    portSocketManager_ = std::make_unique<PortSocketManager>();

    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(SLE_DATATRANSFER_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkSleDataTransfer> proxy = iface_cast<INearlinkSleDataTransfer>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        NL_CHECK_RETURN(callbackImp_, "callbackImp_ is nullptr");
        proxy->RegisterSleDataTransferCallback(callbackImp_);
    };

    info->stateOffFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkSleDataTransfer> proxy = iface_cast<INearlinkSleDataTransfer>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr.");
        uuidAndPort_.FindAndRmv([this, proxy](std::string uuid, uint16_t portId) {
            callbacks_.Erase(portId);
            portSocketManager_->DestroyPort(portId); // 销毁socket
            proxy->DestroyPort(uuid, portId);
            return true;
        });
    };

    info->serviceStoppedFunc_ = [this]() -> void {
        uuidAndPort_.Iterate([this](std::string uuid, uint16_t portId) {
            portSocketManager_->DestroyPort(portId);
        });
        callbacks_.Clear();
        uuidAndPort_.Clear();
    };
    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("SleDataTransfer profileRegisterId_ is invalid");
    }
}

NlErrCode SleDataTransfer::impl::CheckUuid(const std::string &uuid)
{
    NL_CHECK_RETURN_RET(!uuid.empty(), NL_ERR_INVALID_PARAM, "uuid empty");
    bool valid;
    if (uuid.length() == STANDARD_UUID_LEN) {
        valid = uuid == STANDARD_UUID_ICCE;
    } else {
        valid = uuid.length() == CUSTOM_UUID_LEN;
    }
    NL_CHECK_RETURN_RET(valid, NL_ERR_INVALID_PARAM, "uuid invalid");
    return NL_NO_ERROR;
}

NlErrCode SleDataTransfer::impl::CheckParams(const std::string &address, const std::string &uuid)
{
    NL_CHECK_RETURN_RET(!address.empty(), NL_ERR_INVALID_PARAM, "address empty");
    return CheckUuid(uuid);
}

void SleDataTransfer::impl::SetSocketDataTransfer(uint16_t port, const std::string &address, uint16_t mtu, int fd)
{
    // socketpair 设置本端socket, 添加工作线程需执行的数据读取函数
    portSocketManager_->SetSocket(port, address, mtu, fd,
        [wp = weak_from_this()](std::shared_ptr<InputStream> inputSteam, uint16_t portId, std::string address) {
        auto transfer = wp.lock();
        if (transfer == nullptr) {
            return;
        }
        bool ret = transfer->ReceivedData(inputSteam, portId, address);
        if (ret) { // success
            return;
        }
    });
    HILOGI("SetSocket fd");
}

SleDataTransfer::SleDataTransfer()
{
    pimpl = std::make_unique<impl>();
    pimpl->Init();
    HILOGI("successful");
}

SleDataTransfer::~SleDataTransfer()
{}

std::shared_ptr<SleDataTransfer> SleDataTransfer::CreateSleDataTransfer(void)
{
    static std::shared_ptr<SleDataTransfer> sleDataTransfer = std::make_shared<SleDataTransfer>(Pattern());
    NL_CHECK_RETURN_RET(sleDataTransfer, nullptr, "Create failed.");
    return sleDataTransfer;
}

NlErrCode SleDataTransfer::CreatePort(const std::string &uuid, std::shared_ptr<SleDataTransferCallback> callback)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is nullptr");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check uuid
    NlErrCode ret = pimpl->CheckUuid(uuid);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckUuid failed.");

    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    // 2. check port is exist
    uint16_t port = 0;
    bool existedPort = pimpl->uuidAndPort_.GetValue(uuid, port);
    NL_CHECK_RETURN_RET(!existedPort, NL_ERR_DATATRANSFER_DUPLICATE_REGISTER, "existed port.");

    // 3. check whether the maximum limit is reached.
    NL_CHECK_RETURN_RET(pimpl->uuidAndPort_.Size() < PORT_CNT_LIMIT, NL_ERR_DATATRANSFER_LIMITED, "max port.");

    NlErrCode createPortRet = proxy->CreatePort(uuid, port);
    NL_CHECK_RETURN_RET(port != 0, createPortRet, "CreatePort failed.");
    pimpl->callbacks_.EnsureInsert(port, callback);
    pimpl->uuidAndPort_.EnsureInsert(uuid, port);

    return createPortRet;
}

NlErrCode SleDataTransfer::DestroyPort(const std::string &uuid)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check uuid
    NlErrCode ret = pimpl->CheckUuid(uuid);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckUuid failed.");

    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    uint16_t port = 0;
    bool getPortByUuid = pimpl->uuidAndPort_.GetValue(uuid, port);
    NL_CHECK_RETURN_RET(getPortByUuid, NL_ERR_DATATRANSFER_NO_REGISTER, "getPortByUuid failed.");
    pimpl->uuidAndPort_.Erase(uuid);
    pimpl->callbacks_.Erase(port);
    pimpl->portSocketManager_->DestroyPort(port); // 销毁socket
    NlErrCode errCode = proxy->DestroyPort(uuid, port);
    return errCode;
}

NlErrCode SleDataTransfer::Connect(const ConnectionParams &params)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check params
    NlErrCode ret = pimpl->CheckParams(params.GetAddress(), params.GetUuid());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckConnectionParams failed.");
    NL_CHECK_RETURN_RET(
        params.GetTransMode() < static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_MAX),
        NL_ERR_INVALID_PARAM, "transMode invalid");
    NL_CHECK_RETURN_RET(params.GetFrameType() < static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_INVALID),
        NL_ERR_INVALID_PARAM, "frameType invalid");

    // 2. 如果已经连接，返回
    uint16_t port = 0;
    bool getPortByUuid = pimpl->uuidAndPort_.GetValue(params.GetUuid(), port);
    NL_CHECK_RETURN_RET(getPortByUuid, NL_ERR_INTERNAL_ERROR, "getPortByUuid failed.");

    NearlinkSleDataTransferConnectionParams param;
    param.SetAddress(params.GetAddress());
    param.SetUuid(params.GetUuid());
    param.SetPort(port);
    param.SetTransMode(params.GetTransMode());
    param.SetFrameType(params.GetFrameType());
    HILOGI("param.GetTransMode %{public}hhu, params.GetFrameType() %{public}hhu", param.GetTransMode(),
        param.GetFrameType());
    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    NlErrCode errCode = proxy->Connect(param);
    return errCode;
}

NlErrCode SleDataTransfer::Disconnect(const ConnectionParams &params)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check params
    NlErrCode ret = pimpl->CheckParams(params.GetAddress(), params.GetUuid());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckConnectionParams failed.");

    uint16_t port = 0;
    bool getPortByUuid = pimpl->uuidAndPort_.GetValue(params.GetUuid(), port);
    NL_CHECK_RETURN_RET(getPortByUuid, NL_ERR_INTERNAL_ERROR, "getPortByUuid failed.");

    NearlinkSleDataTransferConnectionParams param;
    param.SetAddress(params.GetAddress());
    param.SetUuid(params.GetUuid());
    param.SetPort(port);
    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    return proxy->Disconnect(param);
}

NlErrCode SleDataTransfer::GetConnectionState(const ConnStateParams &params, int32_t &connState)
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check params
    NlErrCode ret = pimpl->CheckParams(params.GetAddress(), params.GetUuid());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckConnStateParams failed.");

    // 2. whether register
    uint16_t port = 0;
    bool getPortByUuid = pimpl->uuidAndPort_.GetValue(params.GetUuid(), port);
    NL_CHECK_RETURN_RET(getPortByUuid, NL_ERR_INTERNAL_ERROR, "getPortByUuid failed.");

    NearlinkSleDataTransferConnectionParams param;
    param.SetAddress(params.GetAddress());
    param.SetUuid(params.GetUuid());
    param.SetPort(port);
    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");
    return proxy->GetConnectionState(param, connState);
}

NlErrCode SleDataTransfer::WriteData(DataParams &params)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    // 1. check params
    NlErrCode ret = pimpl->CheckParams(params.GetAddress(), params.GetUuid());
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "CheckDataParams failed.");

    uint16_t port = 0;
    bool getPortByUuid = pimpl->uuidAndPort_.GetValue(params.GetUuid(), port);
    NL_CHECK_RETURN_RET(getPortByUuid, NL_ERR_INTERNAL_ERROR, "getPortByUuid failed.");

    size_t length = 0;
    const uint8_t *pData = params.GetData(&length).get();
    NL_CHECK_RETURN_RET(pData != nullptr && length != 0 && length <= DATA_MAX_LEN,
        NL_ERR_INVALID_PARAM, "Invalid parameters");

    std::vector<uint8_t> value(pData, pData + length);

    // Socket 发送消息
    size_t totalLen = 0;
    DataTransferDataParams dataTransfer(params.GetAddress(), params.GetUuid(), port, std::move(value));
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(dataTransfer, totalLen);
    NL_CHECK_RETURN_RET(packageData != nullptr, NL_ERR_SOCKET_TRANS_FAILED, "serializeData failed.");
    SocketTransState sockRet = pimpl->portSocketManager_->SendData(port, params.GetAddress(), packageData.get(),
        totalLen, length); // 返回一个执行结果码
    HILOGD("WriteData ret=%{public}d", sockRet);
    NL_CHECK_RETURN_RET(sockRet == SocketTransState::SLE_TRANS_RESULT_SUCCESS, NL_ERR_SOCKET_TRANS_FAILED,
        "SendData failed.");
    return NL_NO_ERROR;
}

bool SleDataTransfer::IsSupportHighSpeedDataTransfer() const
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), false,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, false, "piml is nullptr");
    std::unique_lock<std::mutex> lock(pimpl->flagMutex_);
    // if check onetime, no need check again, return result
    if (pimpl->isHighSpeedSupported_ != HighSpeedSupportedType::HIGH_SPEED_UNKNOWN) {
        HILOGE("product IsSupportHighSpeedDataTransfer: %{public}d", pimpl->isHighSpeedSupported_);
        return (pimpl->isHighSpeedSupported_ == HighSpeedSupportedType::HIGH_SPEED_SUPPORT);
    }

    if (OHOS::system::GetBoolParameter("const.nearlink.enable.port", true) == false) {
        HILOGE("product don't support HighSpeedDataTransfer");
        pimpl->isHighSpeedSupported_ = HighSpeedSupportedType::HIGH_SPEED_NOT_SUPPORT;
        return false;
    }
    HILOGI("support HighSpeedDataTransfer");
    pimpl->isHighSpeedSupported_ = HighSpeedSupportedType::HIGH_SPEED_SUPPORT;
    return true;
}

bool SleDataTransfer::UpdateConnectInterval(std::string device, int32_t intervalType) const
{
    bool result = false;
#ifdef WATCH_STANDARD
    sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr.");

    NlErrCode ret = proxy->UpdateConnectInterval(device, intervalType, result);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false,
        "UpdateConnectInterval failed, error code: %{public}d", ret);
    HILOGI("intervalType = %{public}d, result:%{public}d", intervalType, result);
#else
    HILOGE("only watch support Update datatransfer Connect Interval");
#endif
    return result;
}

bool SleDataTransfer::impl::ReceivedData(std::shared_ptr<InputStream> inputStream, uint16_t portId,
    const std::string &address)
{
    HILOGD("framework receivedData func start.");
    uint8_t buf[SOCKET_BUFFER_SIZE];
    (void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
    int ret = inputStream->Read(buf, sizeof(buf));
    if (ret <= 0) {
        HILOGE("inputStream.Read failed, ret = %{public}d", ret);
        return false;
    } else {
        std::vector<DataTransferDataParams> packageList = NearlinkDataTransferDataParams::DeserializeDataList(buf, ret);

        NL_CHECK_RETURN_RET(!packageList.empty(), false, "DeserializeData failed.");
        for (auto &dataParams : packageList) {
            DataParams result(dataParams.address_, dataParams.uuid_);
            result.SetData(dataParams.data_.data(), dataParams.data_.size());
            HILOGD("Receive stack port = %{public}u", dataParams.port_);

            std::weak_ptr<SleDataTransferCallback> callbackWptr;
            bool ret = callbacks_.GetValue(dataParams.port_, callbackWptr);
            NL_CHECK_RETURN_RET(ret, false, "SleDataTransferCallback not exist.");
            std::shared_ptr<SleDataTransferCallback> callbackSptr = callbackWptr.lock();

            if (callbackSptr) {
                HILOGD("success take OnReceiveData func.");
                callbackSptr->OnReceiveData(result);
            }
        }
        if (ret >= SOCKET_BUFFER_THRESHOLD_REPORT) {
            sptr<INearlinkSleDataTransfer> proxy = GetProxy<INearlinkSleDataTransfer>(SLE_DATATRANSFER_SERVER);
            NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr.");
            NlErrCode errCode = proxy->SocketEmptyMsg(portId, address);
            NL_CHECK_RETURN_RET(!errCode, false, "SocketEmptyMsg is err.");
        }
    }
    return true;
}

ConnectionParams::ConnectionParams()
{}

ConnectionParams::~ConnectionParams()
{}

ConnectionParams::ConnectionParams(const std::string &address, const std::string &uuid, int32_t state)
    : address_(address), uuid_(uuid), state_(state)
{}

void ConnectionParams::SetAddress(const std::string &address)
{
    address_ = address;
}

std::string ConnectionParams::GetAddress() const
{
    return address_;
}

void ConnectionParams::SetUuid(const std::string &uuid)
{
    uuid_ = uuid;
}

std::string ConnectionParams::GetUuid() const
{
    return uuid_;
}

void ConnectionParams::SetState(int32_t state)
{
    state_ = state;
}

int32_t ConnectionParams::GetState() const
{
    return state_;
}

uint8_t ConnectionParams::GetTransMode() const
{
    return transMode_;
}

void ConnectionParams::SetTransMode(uint8_t transMode)
{
    transMode_ = transMode;
}

uint16_t ConnectionParams::GetMtu() const
{
    return mtu_;
}

void ConnectionParams::SetMtu(uint16_t mtu)
{
    mtu_ = mtu;
}

uint8_t ConnectionParams::GetFrameType() const
{
    return frameType_;
}

void ConnectionParams::SetFrameType(uint8_t frameType)
{
    frameType_ = frameType;
}

ConnStateParams::ConnStateParams()
{}

ConnStateParams::~ConnStateParams()
{}

ConnStateParams::ConnStateParams(const std::string &address, const std::string &uuid) : address_(address), uuid_(uuid)
{}

void ConnStateParams::SetAddress(const std::string &address)
{
    address_ = address;
}

std::string ConnStateParams::GetAddress() const
{
    return address_;
}

void ConnStateParams::SetUuid(const std::string &uuid)
{
    uuid_ = uuid;
}

std::string ConnStateParams::GetUuid() const
{
    return uuid_;
}

DataParams::DataParams(const std::string &address, const std::string &uuid)
    : address_(address), uuid_(uuid), data_(nullptr), length_(0)
{}

DataParams::DataParams(const DataParams &src) : address_(src.address_), uuid_(src.uuid_), length_(src.length_)
{
    if (nullptr != src.data_ && 0 != length_) {
        data_ = std::make_unique<uint8_t[]>(length_);
        (void)memcpy_s(data_.get(), length_, src.data_.get(), length_);
    } else {
        data_.reset(nullptr);
        length_ = 0;
    }
}

DataParams &DataParams::operator=(const DataParams &src)
{
    if (this != &src) {
        address_ = src.address_;
        uuid_ = src.uuid_;
        length_ = src.length_;

        if (nullptr != src.data_ && 0 != length_) {
            data_ = std::make_unique<uint8_t[]>(length_);
            (void)memcpy_s(data_.get(), length_, src.data_.get(), length_);
        } else {
            data_.reset(nullptr);
            length_ = 0;
        }
    }
    return *this;
}

void DataParams::SetAddress(const std::string &address)
{
    address_ = address;
}

std::string DataParams::GetAddress() const
{
    return address_;
}

void DataParams::SetUuid(const std::string &uuid)
{
    uuid_ = uuid;
}

std::string DataParams::GetUuid() const
{
    return uuid_;
}

const std::unique_ptr<uint8_t[]> &DataParams::GetData(size_t *size) const
{
    *size = length_;
    return data_;
}

void DataParams::SetData(const uint8_t *values, const size_t length)
{
    if (values == nullptr || length == 0) {
        HILOGE("values is nullptr, or length is 0");
        return;
    }
    data_ = std::make_unique<uint8_t[]>(length);
    length_ = length;
    (void)memcpy_s(data_.get(), length, values, length);
}

}  // namespace OHOS::Nearlink