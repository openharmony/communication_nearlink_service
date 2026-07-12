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

#ifndef TEST_FUZZTEST_DEVICE_MANAGER_FUZZER_H
#define TEST_FUZZTEST_DEVICE_MANAGER_FUZZER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define FUZZ_PROJECT_NAME "device_manager_fuzzer"

namespace OHOS {
namespace Nearlink {

/* Fuzz test functions for xml_parse.cpp */
void FuzzXmlParseLoad(const uint8_t *data, size_t size);
void FuzzXmlParseGetValue(const uint8_t *data, size_t size);
void FuzzXmlParseSetValue(const uint8_t *data, size_t size);
void FuzzXmlParseHasProperty(const uint8_t *data, size_t size);
void FuzzXmlParseRemoveSection(const uint8_t *data, size_t size);
void FuzzXmlParseGetSubSections(const uint8_t *data, size_t size);
void FuzzXmlParseHasSection(const uint8_t *data, size_t size);
void FuzzXmlParseCreateSave(const uint8_t *data, size_t size);

/* Fuzz test functions for SleUtils.cpp */
void FuzzSleUtilsConvertHexStringToInt(const uint8_t *data, size_t size);
void FuzzSleUtilsConvertHexCharToInt(const uint8_t *data, size_t size);
void FuzzSleUtilsIntToHexString(const uint8_t *data, size_t size);
void FuzzSleUtilsConvertIntToHexString(const uint8_t *data, size_t size);
void FuzzSleUtilsStringDataToHexString(const uint8_t *data, size_t size);
void FuzzSleUtilsRand16hex(const uint8_t *data, size_t size);
void FuzzSleUtilsGetRandomAddress(const uint8_t *data, size_t size);

/* Fuzz test functions for SleConfig.cpp */
void FuzzSleConfigValidate(const uint8_t *data, size_t size);
void FuzzSleConfigLocalInfo(const uint8_t *data, size_t size);
void FuzzSleConfigPeerInfo(const uint8_t *data, size_t size);
void FuzzSleConfigCryptoInfo(const uint8_t *data, size_t size);
void FuzzSleConfigCdsmInfo(const uint8_t *data, size_t size);
void FuzzSleConfigCloudDevice(const uint8_t *data, size_t size);
void FuzzSleConfigDeviceModel(const uint8_t *data, size_t size);
void FuzzSleConfigVolume(const uint8_t *data, size_t size);
void FuzzSleConfigAscDevice(const uint8_t *data, size_t size);

/* Fuzz test functions for SleHuksTool.cpp */
void FuzzSleHksToolEncrypt(const uint8_t *data, size_t size);
void FuzzSleHksToolDecrypt(const uint8_t *data, size_t size);
void FuzzSleHksToolDeleteKey(const uint8_t *data, size_t size);

/* Fuzz test functions for nearlink_device_manager.cpp */
void FuzzNearlinkDeviceManager(const uint8_t *data, size_t size);

/* Fuzz test functions for SleRemoteDeviceManager.cpp */
void FuzzSleRemoteDeviceManager(const uint8_t *data, size_t size);

/* Fuzz test functions for SleCoexistManager.cpp */
void FuzzSleCoexistManager(const uint8_t *data, size_t size);

/* Fuzz test functions for AdapterDeviceConfig.cpp */
void FuzzAdapterDeviceConfig(const uint8_t *data, size_t size);

/* Fuzz test functions for ManufacturerAbilityLoader.cpp */
void FuzzManufacturerAbilityLoader(const uint8_t *data, size_t size);

}  // namespace Nearlink
}  // namespace OHOS

#endif  // TEST_FUZZTEST_DEVICE_MANAGER_FUZZER_H
