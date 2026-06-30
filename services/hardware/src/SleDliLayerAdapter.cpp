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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>
#include <v1_0/isle_hci_interface.h>
#include <v1_1/isle_hci_interface.h>
#include "log.h"
#include "SleDliCallbacks.h"
#include "SleDliLayerAdapter.h"
#include "SleDliSnoop.h"
#include <iproxy_broker.h>
#include <iremote_object.h>
#include <csignal>
#include <unistd.h>

using OHOS::HDI::Nearlink::Hci::V1_0::ISleHciInterface;
using OHOS::sptr;
using OHOS::wptr;
using OHOS::IRemoteObject;
using std::signal;

static sptr<ISleHciInterface> g_iSleDli = nullptr;
static sptr<SleDliCallbacks> g_sleDliCallbacks = nullptr;

class SleDliDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    void OnRemoteDied(const wptr<IRemoteObject> &object) override {
        HILOGE("SleDli service is dead");
        kill(getpid(), SIGKILL);
    }
};

static sptr<SleDliDeathRecipient> g_deathRecipient = nullptr;

bool RegisterSleDliDeathRecipient()
{
    g_deathRecipient = sptr<SleDliDeathRecipient>::MakeSptr();
    if (g_deathRecipient == nullptr) {
        HILOGE("Failed to create death recipient");
        return false;
    }

    sptr<IRemoteObject> remoteObj = OHOS::HDI::hdi_objcast<ISleHciInterface>(g_iSleDli);
    if (remoteObj == nullptr) {
        HILOGE("Failed to get remote object");
        g_deathRecipient = nullptr;
        return false;
    }

    bool isAdded = remoteObj->AddDeathRecipient(g_deathRecipient);
    if (!isAdded) {
        HILOGE("Failed to add death recipient");
        g_deathRecipient = nullptr;
        return false;
    }

    return true;
}

void UnRegisterSleDliDeathRecipient()
{
    if (g_iSleDli != nullptr) {
        sptr<IRemoteObject> remoteObj = OHOS::HDI::hdi_objcast<ISleHciInterface>(g_iSleDli);
        if (remoteObj != nullptr && g_deathRecipient != nullptr) {
            remoteObj->RemoveDeathRecipient(g_deathRecipient);
            g_deathRecipient = nullptr;
        }
    }
}

int SleHalInit(SleDliCallbackFunc *callbacks)
{
    HILOGI("enter");

    if (callbacks == nullptr) {
        return INITIALIZATION_ERROR;
    }

    g_iSleDli = ISleHciInterface::Get();
    if (g_iSleDli == nullptr) {
        HILOGI("iSleDli is null");
        return INITIALIZATION_ERROR;
    }

    g_sleDliCallbacks = new (std::nothrow) SleDliCallbacks(callbacks);
    if (g_sleDliCallbacks == nullptr) {
        return INITIALIZATION_ERROR;
    }

    auto ret = g_iSleDli->SleHalInit(g_sleDliCallbacks);
    if (ret != SleStatus::SUCCESS) {
        HILOGE("SleDli is fail");
        return INITIALIZATION_ERROR;
    }

    if (!RegisterSleDliDeathRecipient()) {
        return INITIALIZATION_ERROR;
    }

    return SUCCESS;
}

void SleReset()
{
    kill(getpid(), SIGKILL);
}


int SleSendDliPacket(const SlePacket *packet)
{
    if (packet == nullptr) {
        return TRANSPORT_ERROR;
    }
    if (g_iSleDli == nullptr) {
        return INITIALIZATION_ERROR;
    }

    std::vector<uint8_t> data;
    data.assign(packet->data, packet->data + packet->size);
    uint32_t type = static_cast<uint32_t>(data[0]);
    SleDliSnoop::GetInstance().DliSnoopCapture(type, data, false);
    int32_t ret = g_iSleDli->SleSendHciPacket(data);
    if (ret != SleStatus::SUCCESS) {
        HILOGE("SleDli is fail");
        return TRANSPORT_ERROR;
    }

    return SUCCESS;
}

void SleHalClose(void)
{
    UnRegisterSleDliDeathRecipient();

    if (g_sleDliCallbacks != nullptr) {
        g_sleDliCallbacks = nullptr;
    }

    if (g_iSleDli != nullptr) {
        g_iSleDli->Close();
        g_iSleDli = nullptr;
    }
}

int GetDliVersion(void)
{
    if (g_iSleDli == nullptr) {
        HILOGI("g_iSleDli == nullptr");
        return DLI_VERSION_1_0;
    }
    auto iSleHci_1_1 = OHOS::HDI::Nearlink::Hci::V1_1::ISleHciInterface::CastFrom(g_iSleDli);
    if (iSleHci_1_1 != nullptr) {
        HILOGI("dliVersion is DLI_VERSION_1_1");
        return DLI_VERSION_1_1;
    }
    HILOGI("dliVersion is DLI_VERSION_1_0");
    return DLI_VERSION_1_0;
}
