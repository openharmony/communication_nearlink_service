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

/*
 * XML 配置解析器测试：产品源码直接编入本测试目标，通过临时文件做
 * 写入→保存→重新加载→读回比对的完整回路，并覆盖各读取入口的
 * 空节点、格式错误、长度不匹配等失败路径。
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "xml_parse.h"

namespace {
/* 五类需要转义的字符一次喂齐，非特殊字符走 default 分支 */
const std::string SPECIAL_TEXT = "a&b<c>d\"e'f-;";
} // namespace

using namespace utility;

class XmlParseTest : public ::testing::Test {
protected:
    /* 临时文件位置一次探测：优先标准临时目录，不可写时退回当前目录 */
    static void SetUpTestCase()
    {
        const char *candidates[] = {"/data/local/tmp", "."};
        for (auto dir : candidates) {
            testFile_ = std::string(dir) + "/nl_dm_xml_parse_test.xml";
            XmlParse probe;
            if (probe.Create(testFile_)) {
                (void)remove(testFile_.c_str());
                return;
            }
        }
        testFile_ = "/data/local/tmp/nl_dm_xml_parse_test.xml";
    }

    static void TearDownTestCase() {}

    /* 每个用例从全新文件和空文档开始，避免用例间文档状态串扰 */
    void SetUp() override
    {
        (void)remove(testFile_.c_str());
        parse_ = std::make_unique<XmlParse>();
        ASSERT_TRUE(parse_->Create(testFile_));
    }

    void TearDown() override
    {
        parse_.reset();
        (void)remove(testFile_.c_str());
    }

    /* 把手工构造的 XML 原文写入临时文件，用于制造公共 API 造不出的文档形态 */
    void WriteRawFile(const std::string &content)
    {
        std::ofstream out(testFile_, std::ios::trunc);
        out << content;
    }

    std::unique_ptr<XmlParse> parse_;
    static std::string testFile_;
};

std::string XmlParseTest::testFile_;

/* 重复加载会先释放旧文档再重新解析，两次加载都应成功 */
TEST_F(XmlParseTest, ReloadReleasesPreviousDocument)
{
    ASSERT_TRUE(parse_->SetValue("host", "name", std::string("first")));
    ASSERT_TRUE(parse_->Save());
    EXPECT_TRUE(parse_->Load(testFile_));
    EXPECT_TRUE(parse_->Load(testFile_));
}

/* 加载不存在的文件返回失败，且后续按属性查找也返回失败 */
TEST_F(XmlParseTest, LoadMissingFileFails)
{
    XmlParse missing;
    EXPECT_FALSE(missing.Load(testFile_ + ".missing"));
    std::string value;
    EXPECT_FALSE(missing.GetValue("host", "name", value));
}

/* 整数按 0x 十六进制文本落盘，重新加载后能原值读回，负数同样成立 */
TEST_F(XmlParseTest, IntValueRoundTrip)
{
    ASSERT_TRUE(parse_->SetValue("host", "count", 255));
    ASSERT_TRUE(parse_->SetValue("host", "negative", -1));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    int value = 0;
    EXPECT_TRUE(reloaded.GetValue("host", "count", value));
    EXPECT_EQ(value, 255);
    EXPECT_TRUE(reloaded.GetValue("host", "negative", value));
    EXPECT_EQ(value, -1);
}

/* 整数读取的失败路径：属性缺失、空节点、内容过短、内容不是十六进制 */
TEST_F(XmlParseTest, IntValueRejectsBadNodes)
{
    ASSERT_TRUE(parse_->SetValue("host", "count", 16));
    ASSERT_TRUE(parse_->Save());
    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));

    int value = -1;
    /* 属性不存在 */
    EXPECT_FALSE(reloaded.GetValue("host", "absent", value));
    /* 内容去掉 0x 前缀后不是十六进制 */
    WriteRawFile("<?xml version=\"1.0\"?><config><T1 section=\"host\">"
        "<T1 property=\"plain\">0xzz</T1></T1></config>");
    ASSERT_TRUE(reloaded.Load(testFile_));
    EXPECT_FALSE(reloaded.GetValue("host", "plain", value));
    /* 内容去掉 0x 后不足一位 */
    WriteRawFile("<?xml version=\"1.0\"?><config><T1 section=\"host\">"
        "<T1 property=\"tiny\">0X</T1></T1></config>");
    ASSERT_TRUE(reloaded.Load(testFile_));
    EXPECT_FALSE(reloaded.GetValue("host", "tiny", value));
    /* 空节点没有任何文本内容 */
    WriteRawFile("<?xml version=\"1.0\"?><config><T1 section=\"host\">"
        "<T1 property=\"empty\"/></T1></config>");
    ASSERT_TRUE(reloaded.Load(testFile_));
    EXPECT_FALSE(reloaded.GetValue("host", "empty", value));
}

/* 字符串值五类特殊字符转义落盘，重新加载后原样读回 */
TEST_F(XmlParseTest, StringValueEscapesAndRoundTrips)
{
    ASSERT_TRUE(parse_->SetValue("host", "memo", SPECIAL_TEXT));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    std::string value;
    EXPECT_TRUE(reloaded.GetValue("host", "memo", value));
    EXPECT_EQ(value, SPECIAL_TEXT);
}

/* 布尔值 true/false 双向，非布尔文本读取失败 */
TEST_F(XmlParseTest, BoolValueRoundTrip)
{
    ASSERT_TRUE(parse_->SetValue("host", "enabled", true));
    ASSERT_TRUE(parse_->SetValue("host", "sub", "disabled", false));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    bool value = false;
    EXPECT_TRUE(reloaded.GetValue("host", "enabled", value));
    EXPECT_TRUE(value);
    EXPECT_TRUE(reloaded.GetValue("host", "sub", "disabled", value));
    EXPECT_FALSE(value);

    /* 用字符串接口写入既非 true 也非 false 的文本 */
    ASSERT_TRUE(reloaded.SetValue("host", "sub", "maybe", std::string("perhaps")));
    value = true;
    EXPECT_FALSE(reloaded.GetValue("host", "sub", "maybe", value));
}

/* 定长字符读取的可用部分：文本读回与属性缺失 */
TEST_F(XmlParseTest, StringValueReadByProperty)
{
    const std::string hexText = "AABBCCDD";
    ASSERT_TRUE(parse_->SetValue("host", "addr", hexText));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    std::string val;
    EXPECT_TRUE(reloaded.GetValue("host", "addr", val));
    EXPECT_EQ(val, hexText);
    std::string absent;
    EXPECT_FALSE(reloaded.GetValue("host", "absent", absent));
}

/* 需要产品 XmlParse 补 GetValue(char*, size) 重载后启用：缓冲长度与内容长度不符应失败 */
TEST_F(XmlParseTest, DISABLED_CharValueLengthMismatch)
{
    GTEST_SKIP() << "product XmlParse lacks GetValue(char*, size) overload";
}

/* 需要产品 XmlParse 补 GetValue(char*, size) 重载后启用：输出缓冲为空应失败 */
TEST_F(XmlParseTest, DISABLED_CharValueNullBuffer)
{
    GTEST_SKIP() << "product XmlParse lacks GetValue(char*, size) overload";
}

/* 首次写入自动创建 section/subsection/property 三级节点，逐级可查可读 */
TEST_F(XmlParseTest, SetValueCreatesNestedNodes)
{
    ASSERT_TRUE(parse_->SetValue("paired", "deviceA", "name", std::string("earbuds")));
    ASSERT_TRUE(parse_->SetValue("paired", "deviceB", "name", std::string("watch")));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    EXPECT_TRUE(reloaded.HasSection("paired"));
    EXPECT_TRUE(reloaded.HasSection("paired", "deviceA"));
    EXPECT_FALSE(reloaded.HasSection("paired", "deviceC"));
    EXPECT_FALSE(reloaded.HasSection("absent"));
    EXPECT_FALSE(reloaded.HasSection("absent", "deviceA"));

    std::string value;
    EXPECT_TRUE(reloaded.GetValue("paired", "deviceB", "name", value));
    EXPECT_EQ(value, "watch");
}

/* 属性存在性与不存在路径；空 section 查属性也返回不存在 */
TEST_F(XmlParseTest, HasPropertyFoundAndMissing)
{
    ASSERT_TRUE(parse_->SetValue("paired", "deviceA", "name", std::string("earbuds")));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    EXPECT_TRUE(reloaded.HasProperty("paired", "deviceA", "name"));
    EXPECT_FALSE(reloaded.HasProperty("paired", "deviceA", "absent"));
    EXPECT_FALSE(reloaded.HasProperty("paired", "absent", "name"));

    /* 空 section 节点：查到 section 但没有子节点可遍历 */
    WriteRawFile("<?xml version=\"1.0\"?><config><T1 section=\"blank\"/></config>");
    ASSERT_TRUE(reloaded.Load(testFile_));
    std::string blank;
    EXPECT_FALSE(reloaded.GetValue("blank", "any", blank));
}

/* 子段枚举只收集带 section 属性的子节点，属性节点被跳过 */
TEST_F(XmlParseTest, GetSubSectionsSkipsPropertyNodes)
{
    ASSERT_TRUE(parse_->SetValue("paired", "deviceA", "name", std::string("earbuds")));
    ASSERT_TRUE(parse_->SetValue("paired", "deviceB", "name", std::string("watch")));
    /* 直接挂在 section 下的属性节点（无子段）也是 section 的子节点 */
    ASSERT_TRUE(parse_->SetValue("paired", "size", 2));
    ASSERT_TRUE(parse_->Save());

    XmlParse reloaded;
    ASSERT_TRUE(reloaded.Load(testFile_));
    std::vector<std::string> subs;
    EXPECT_TRUE(reloaded.GetSubSections("paired", subs));
    EXPECT_EQ(subs.size(), 2U);
    EXPECT_NE(std::find(subs.begin(), subs.end(), "deviceA"), subs.end());
    EXPECT_NE(std::find(subs.begin(), subs.end(), "deviceB"), subs.end());

    std::vector<std::string> none;
    EXPECT_FALSE(reloaded.GetSubSections("absent", none));
}

/* 删除整个子段、删除单个属性、删除不存在的目标均按预期返回 */
TEST_F(XmlParseTest, RemoveSectionAndProperty)
{
    ASSERT_TRUE(parse_->SetValue("paired", "deviceA", "name", std::string("earbuds")));
    ASSERT_TRUE(parse_->SetValue("paired", "deviceB", "name", std::string("watch")));
    ASSERT_TRUE(parse_->Save());
    ASSERT_TRUE(parse_->RemoveProperty("paired", "deviceA", "name"));
    EXPECT_FALSE(parse_->HasProperty("paired", "deviceA", "name"));
    EXPECT_FALSE(parse_->RemoveProperty("paired", "deviceA", "name"));

    ASSERT_TRUE(parse_->RemoveSection("paired", "deviceA"));
    EXPECT_FALSE(parse_->HasSection("paired", "deviceA"));
    EXPECT_FALSE(parse_->RemoveSection("paired", "deviceA"));

    ASSERT_TRUE(parse_->RemoveSection("paired"));
    EXPECT_FALSE(parse_->HasSection("paired"));
    EXPECT_FALSE(parse_->RemoveSection("paired"));
}

/* 属性级删除入口（无子段两参版本）命中与未命中 */
TEST_F(XmlParseTest, RemovePropertyWithoutSubSection)
{
    ASSERT_TRUE(parse_->SetValue("host", "name", std::string("phone")));
    EXPECT_TRUE(parse_->HasProperty("host", "name"));
    EXPECT_TRUE(parse_->RemoveProperty("host", "name"));
    EXPECT_FALSE(parse_->HasProperty("host", "name"));
    EXPECT_FALSE(parse_->RemoveProperty("host", "name"));
}

/* 未加载任何文档时保存失败（文档指针为空） */
TEST_F(XmlParseTest, SaveWithoutDocumentFails)
{
    XmlParse empty;
    EXPECT_FALSE(empty.Save());
}

/* 未加载文档时的查询路径：两参布尔写入与各级缺失目标的读取删除均失败 */
TEST_F(XmlParseTest, MissingTargetsAllFail)
{
    /* 两参布尔写入后读回 */
    ASSERT_TRUE(parse_->SetValue("host", "fast", true));
    ASSERT_TRUE(parse_->SetValue("host", "slow", false));
    bool flag = false;
    EXPECT_TRUE(parse_->GetValue("host", "fast", flag));
    EXPECT_TRUE(flag);
    EXPECT_TRUE(parse_->GetValue("host", "slow", flag));
    EXPECT_FALSE(flag);

    /* 段缺失：带子段查值、删段、判段均失败 */
    std::string text;
    EXPECT_FALSE(parse_->GetValue("absentSec", "absentSub", "p", text));
    EXPECT_FALSE(parse_->RemoveSection("absentSec", "sub"));
    EXPECT_FALSE(parse_->HasSection("absentSec", "sub"));

    /* 未加载文档的实例：根节点为空，查段失败 */
    XmlParse noDoc;
    EXPECT_FALSE(noDoc.HasSection("host"));
}
