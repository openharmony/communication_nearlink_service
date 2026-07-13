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
#include "fuzzer/FuzzedDataProvider.h"
#include <filesystem>
#include <fstream>
#include "xml_parse.h"

namespace OHOS {
namespace Nearlink {

static constexpr const char *TEST_XML_STRING =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<config>"
    "  <T1 section=\"Host\">"
    "    <T1 property=\"CreateConfigFile\">0x1</T1>"
    "    <T1 property=\"Address\">00:00:00:00:00:00</T1>"
    "    <T1 property=\"DeviceName\">TestDevice</T1>"
    "    <T1 property=\"IOCapability\">0x3</T1>"
    "    <T1 property=\"LocalAddrType\">0x0</T1>"
    "  </T1>"
    "  <T1 section=\"Sle Paired Device List\">"
    "    <T1 section=\"AA:BB:CC:DD:EE:01\">"
    "      <T1 property=\"DeviceName\">PeerDevice1</T1>"
    "      <T1 property=\"PeerLk\">0123456789ABCDEF0123456789ABCDEF</T1>"
    "      <T1 property=\"CryptoAlgo\">1</T1>"
    "      <T1 property=\"KeyAlgo\">1</T1>"
    "      <T1 property=\"IntegrChk\">1</T1>"
    "      <T1 property=\"IOCapability\">0x3</T1>"
    "      <T1 property=\"PeerAddrType\">1</T1>"
    "      <T1 property=\"PairDirect\">1</T1>"
    "      <T1 property=\"PeerAppearance\">0</T1>"
    "      <T1 property=\"MusicVolume\">50</T1>"
    "      <T1 property=\"CallVolume\">50</T1>"
    "      <T1 property=\"ModelId\">model_001</T1>"
    "      <T1 property=\"DevType\">type_001</T1>"
    "    </T1>"
    "    <T1 section=\"AA:BB:CC:DD:EE:02\">"
    "      <T1 property=\"DeviceName\">PeerDevice2</T1>"
    "      <T1 property=\"PeerLk\">FEDCBA9876543210FEDCBA9876543210</T1>"
    "    </T1>"
    "  </T1>"
    "  <T1 section=\"Sle Cooperation Device List\">"
    "    <T1 section=\"AA:BB:CC:DD:EE:01\">"
    "      <T1 property=\"CdsmMemberList\">AA:BB:CC:DD:EE:01;AA:BB:CC:DD:EE:02</T1>"
    "      <T1 property=\"IsPrivateDevice\">1</T1>"
    "    </T1>"
    "  </T1>"
    "  <T1 section=\"Sle Cloud Paired Cooperation Device List\">"
    "    <T1 section=\"AA:BB:CC:DD:EE:03\">"
    "      <T1 property=\"BluetoothAddr\">11:22:33:44:55:66</T1>"
    "      <T1 property=\"DeviceName\">CloudDevice1</T1>"
    "      <T1 property=\"Token\">0000000000000000000000000000000000000000000000000000000000000000</T1>"
    "      <T1 property=\"Model\">cloud_model</T1>"
    "    </T1>"
    "  </T1>"
    "  <T1 section=\"Last ASC Active Device\">"
    "    <T1 property=\"RawAddress\">AA:BB:CC:DD:EE:FF</T1>"
    "  </T1>"
    "  <T1 section=\"Sle Background Reconnection Queue\">"
    "    <T1 property=\"QueueInfo\"/>"
    "  </T1>"
    "</config>";

static const std::string TEST_XML_PATH =
    "/data/service/el1/public/nearlink/fuzz_test/fuzz_xml_parse.xml";

static constexpr const char *SECTIONS[] = {
    "Host",
    "Sle Paired Device List",
    "Sle Cooperation Device List",
    "Sle Cloud Paired Cooperation Device List",
    "Last ASC Active Device",
    "Sle Background Reconnection Queue",
    "AA:BB:CC:DD:EE:01",
    "AA:BB:CC:DD:EE:02",
    "AA:BB:CC:DD:EE:03"
};

static constexpr const char *SUBSECTIONS[] = {
    "AA:BB:CC:DD:EE:01",
    "AA:BB:CC:DD:EE:02",
    "AA:BB:CC:DD:EE:03"
};

static constexpr const char *PROPERTIES[] = {
    "CreateConfigFile", "Address", "DeviceName", "IOCapability",
    "LocalAddrType", "PeerLk", "CryptoAlgo", "KeyAlgo", "IntegrChk",
    "PeerAddrType", "PairDirect", "PeerAppearance", "MusicVolume",
    "CallVolume", "ModelId", "DevType", "CdsmMemberList", "IsPrivateDevice",
    "BluetoothAddr", "Token", "Model", "RawAddress", "QueueInfo"
};

static std::string PickSection(uint32_t idx)
{
    return SECTIONS[idx % (sizeof(SECTIONS) / sizeof(SECTIONS[0]))];
}

static std::string PickSubSection(uint32_t idx)
{
    return SUBSECTIONS[idx % (sizeof(SUBSECTIONS) / sizeof(SUBSECTIONS[0]))];
}

static std::string PickProperty(uint32_t idx)
{
    return PROPERTIES[idx % (sizeof(PROPERTIES) / sizeof(PROPERTIES[0]))];
}

static void EnsureTestXml()
{
    std::ofstream out(TEST_XML_PATH);
    out << TEST_XML_STRING;
    out.close();
}

void FuzzXmlParseLoad(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    (void)data;
    (void)size;
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
}

void FuzzXmlParseGetValue(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string subSection = PickSubSection(idx++);
    std::string property = PickProperty(idx++);
    int intVal = 0;
    std::string strVal;
    bool boolVal = false;
    char charBuf[256] = {0};
    xmlParse->GetValue(section, subSection, property, intVal);
    xmlParse->GetValue(section, subSection, property, strVal);
    xmlParse->GetValue(section, subSection, property, charBuf, 255);
    xmlParse->GetValue(section, subSection, property, boolVal);
    std::string section2 = PickSection(idx++);
    std::string property2 = PickProperty(idx++);
    xmlParse->GetValue(section2, property2, intVal);
    xmlParse->GetValue(section2, property2, strVal);
    xmlParse->GetValue(section2, property2, boolVal);
}

void FuzzXmlParseSetValue(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string subSection = PickSubSection(idx++);
    std::string property = PickProperty(idx++);
    std::string strVal = PickProperty(idx++);
    int intVal = provider.ConsumeIntegral<int>();
    bool boolVal = provider.ConsumeBool();
    xmlParse->SetValue(section, subSection, property, intVal);
    xmlParse->SetValue(section, subSection, property, strVal);
    xmlParse->SetValue(section, subSection, property, boolVal);
    xmlParse->SetValue(section, property, strVal);
    xmlParse->SetValue(section, property, intVal);
    xmlParse->Save();
}

void FuzzXmlParseHasProperty(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string subSection = PickSubSection(idx++);
    std::string property = PickProperty(idx++);
    xmlParse->HasProperty(section, subSection, property);
    std::string section2 = PickSection(idx++);
    std::string property2 = PickProperty(idx++);
    xmlParse->HasProperty(section2, property2);
}

void FuzzXmlParseRemoveSection(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string subSection = PickSubSection(idx++);
    xmlParse->RemoveSection(section, subSection);
    std::string section2 = PickSection(idx++);
    xmlParse->RemoveSection(section2);
}

void FuzzXmlParseGetSubSections(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx);
    std::vector<std::string> subSections;
    xmlParse->GetSubSections(section, subSections);
}

void FuzzXmlParseHasSection(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string subSection = PickSubSection(idx++);
    xmlParse->HasSection(section);
    xmlParse->HasSection(section, subSection);
}

void FuzzXmlParseCreateSave(const uint8_t *data, size_t size)
{
    EnsureTestXml();
    std::shared_ptr<utility::XmlParse> xmlParse = std::make_shared<utility::XmlParse>();
    xmlParse->Load(TEST_XML_PATH);
    xmlParse->Parse();
    FuzzedDataProvider provider(data, size);
    uint32_t idx = provider.ConsumeIntegral<uint32_t>();
    std::string section = PickSection(idx++);
    std::string property = PickProperty(idx++);
    std::string strVal = PickProperty(idx++);
    int intVal = provider.ConsumeIntegral<int>();
    bool boolVal = provider.ConsumeBool();
    xmlParse->SetValue(section, property, strVal);
    xmlParse->SetValue(section, property, intVal);
    xmlParse->SetValue(section, property, boolVal);
    xmlParse->Save();
}

}  // namespace Nearlink
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    std::filesystem::create_directories("/data/service/el1/public/nearlink/fuzz_test");
    std::ofstream out(OHOS::Nearlink::TEST_XML_PATH);
    out << OHOS::Nearlink::TEST_XML_STRING;
    out.close();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    using namespace OHOS::Nearlink;
    if (data == nullptr || size == 0) return 0;
    FuzzedDataProvider provider(data, size);
    uint32_t category = provider.ConsumeIntegral<uint32_t>();
    switch (category % 8) {
        case 0: FuzzXmlParseLoad(data, size); break;
        case 1: FuzzXmlParseGetValue(data, size); break;
        case 2: FuzzXmlParseSetValue(data, size); break;
        case 3: FuzzXmlParseHasProperty(data, size); break;
        case 4: FuzzXmlParseRemoveSection(data, size); break;
        case 5: FuzzXmlParseGetSubSections(data, size); break;
        case 6: FuzzXmlParseHasSection(data, size); break;
        case 7: FuzzXmlParseCreateSave(data, size); break;
        default: break;
    }
    return 0;
}
