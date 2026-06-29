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
#include "gtest/gtest.h"
#include "ssap_manager.h"
#include "sdf_mem.h"
#include "securec.h"

static SLE_Addr_S g_linkAddr = {.type = PUBLIC_ADDRESS, .addr = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};
static uint16_t g_linkId = 1;

class UT_SSAP_CONTROL : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
    }
};

static void MockSendCbk(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    SSAP_LinkSetTask(link, buff, opcode);
}

static void MockProcessTask(SSAP_Link_S *link, void *arg)
{
    (void)arg;
    uint32_t buffSize = 6; // buff size
    SDF_Buff_S *buff = SDF_BuffNew(buffSize);
    (void)memcpy_s(SDF_BuffAppend(buff, buffSize), SLE_ADDR_LEN, &g_linkAddr.addr, SLE_ADDR_LEN);
    link->sendFunc(link, buff, SSAP_FIND_STRUCTURE_REQ);
}

TEST_F(UT_SSAP_CONTROL, LINK_CONTROL)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_linkAddr, g_linkId, NULL);
    EXPECT_TRUE(link == NULL);
    link = SSAP_CreateSsapLink(&g_linkAddr, g_linkId, MockSendCbk);
    EXPECT_TRUE(link != NULL);
    link = SSAP_FindSsapLinkByAddr(&g_linkAddr);
    EXPECT_TRUE(link != NULL);
    EXPECT_EQ(link->lcid, g_linkId);
    link = SSAP_FindSsapLinkByLcid(g_linkId);
    EXPECT_TRUE(link != NULL);
    EXPECT_EQ(memcmp(&link->addr, &g_linkAddr, sizeof(SLE_Addr_S)), 0);
    SSAP_DeleteSsapLinkByAddr(&g_linkAddr);
    link = SSAP_FindSsapLinkByLcid(g_linkId);
    EXPECT_TRUE(link == NULL);
}

TEST_F(UT_SSAP_CONTROL, TASK_CONTROL)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_linkAddr, g_linkId, MockSendCbk);
    SSAP_TaskParam_S *param = SSAP_LinkGetFirstTaskParam(link);
    EXPECT_TRUE(param == NULL);
    param = SSAP_AllocTaskParam(NULL);
    EXPECT_TRUE(param == NULL);
    SSAP_TaskParam_S taskParam = {.func = MockProcessTask};
    param = SSAP_AllocTaskParam(&taskParam);
    EXPECT_TRUE(param != NULL);
    SSAP_AddTaskParamToLink(link, param);
    param = SSAP_LinkGetFirstTaskParam(link);
    EXPECT_TRUE(param != NULL);
    SSAP_AddTaskParamToLink(link, param);
    SSAP_ExcuteProcessTask(link);
    EXPECT_EQ(memcmp(SDF_DataOffset(SSAP_GetLastBuff(link)), &g_linkAddr.addr, SLE_ADDR_LEN), 0);
    EXPECT_EQ(SSAP_CheckOpcode(link, SSAP_READ_RSP), SSAP_ERRCODE_INVALID_PDU);
    EXPECT_EQ(SSAP_CheckOpcode(link, SSAP_FIND_STRUCTURE_RSP), SSAP_ERRCODE_SUCCESS);
    SSAP_LinkClearCurrentTask(link);
    SSAP_DeleteSsapLinkByAddr(&g_linkAddr);
}