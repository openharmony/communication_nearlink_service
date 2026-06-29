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

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>

#include "nearlink_host.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"

#include "cJSON.h"

namespace OHOS {
namespace Nearlink {

#define CLI_LOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define CLI_ERROR(fmt, ...) fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

typedef std::function<int(int, char**)> CommandHandler;

struct Command {
    const char* name;
    const char* description;
    const char* usage;
    const char* parameters;
    const char* examples;
    CommandHandler handler;
};

constexpr int32_t MIN_ARGC_WITH_COMMAND = 2;
constexpr size_t HELP_ARGV_SIZE = 2;
constexpr int32_t ARGV_CMD_PARAM_START_INDEX = 2;
constexpr int32_t MAX_CMD_HELP_LENGTH = 6;
constexpr int32_t MAX_ENABLE_PARAM_LENGTH = 16;
constexpr int32_t MAX_ARGV_LENGTH = 255;

static std::unordered_map<std::string, Command> gCommands;
static const char* gProgramName = "";
static bool gHasSubcommands = false;
static const char* gToolDescription =
    "NearLink control utility for enabling and disabling NearLink functionality";

#define REGISTER_CMD(name, desc, usage, params, examples, handler) \
    gCommands[name] = {name, desc, usage, params, examples, handler}

static int OutputSuccess(const std::string& message)
{
    cJSON* response = cJSON_CreateObject();
    if (response == nullptr) {
        return 0;
    }
    cJSON_AddStringToObject(response, "type", "result");
    cJSON_AddStringToObject(response, "status", "success");
    cJSON* data = cJSON_CreateObject();
    if (data == nullptr) {
        cJSON_Delete(response);
        return 0;
    }
    cJSON_AddStringToObject(data, "message", message.c_str());
    cJSON_AddItemToObject(response, "data", data);
    char* jsonStr = cJSON_PrintUnformatted(response);
    if (jsonStr == nullptr) {
        cJSON_Delete(data);
        cJSON_Delete(response);
        return 0;
    }
    std::cout << jsonStr << std::endl;
    cJSON_free(jsonStr);
    cJSON_Delete(data);
    cJSON_Delete(response);
    return 0;
}

static int OutputError(const std::string& code, const std::string& message,
    const std::string& suggestion = "")
{
    cJSON* response = cJSON_CreateObject();
    if (response == nullptr) {
        return 1;
    }
    cJSON_AddStringToObject(response, "type", "result");
    cJSON_AddStringToObject(response, "status", "failed");
    cJSON_AddStringToObject(response, "errCode", code.c_str());
    cJSON_AddStringToObject(response, "errMsg", message.c_str());
    cJSON_AddStringToObject(response, "suggestion", suggestion.c_str());
    char* jsonStr = cJSON_PrintUnformatted(response);
    if (jsonStr == nullptr) {
        cJSON_Delete(response);
        return 1;
    }
    std::cout << jsonStr << std::endl;
    cJSON_free(jsonStr);
    cJSON_Delete(response);
    return 1;
}

static void PrintFullHelp()
{
    CLI_LOG("%s - %s", gProgramName, gToolDescription);
    CLI_LOG("");
    CLI_LOG("Usage:");
    CLI_LOG("  %s <command> [options]", gProgramName);
    CLI_LOG("");
    CLI_LOG("Parameters:");
    CLI_LOG("  --help             Display this help message");
    CLI_LOG("");
    CLI_LOG("SubCommands:");
    for (const auto& pair : gCommands) {
        CLI_LOG("  %-18s %s", pair.first.c_str(),
            pair.second.description ? pair.second.description : "");
    }
    CLI_LOG("");
    CLI_LOG("Examples:");
    CLI_LOG("  %s --help", gProgramName);
    CLI_LOG("  %s enable --help", gProgramName);
    CLI_LOG("  %s disable --help", gProgramName);
}

static void PrintCommandHelp(const Command& cmd)
{
    CLI_LOG("%s %s - %s", gProgramName, cmd.name,
        cmd.description ? cmd.description : "N/A");
    if (cmd.usage) {
        CLI_LOG("");
        CLI_LOG("Usage:");
        CLI_LOG("  %s", cmd.usage);
    }
    if (cmd.parameters) {
        CLI_LOG("");
        CLI_LOG("Parameters:");
        CLI_LOG("%s", cmd.parameters);
    }
    CLI_LOG("    %-18s %s", "--help", "Display this help message");
    if (cmd.examples) {
        CLI_LOG("");
        CLI_LOG("Examples:");
        CLI_LOG("%s", cmd.examples);
    }
}

static int CmdHelp(int argc, char** argv)
{
    std::string targetCmd;
    for (int i = 0; i < argc; i++) {
        if (argv[i] == nullptr || strlen(argv[i]) > MAX_ARGV_LENGTH) {
            return 1;
        }
        if (strlen(argv[i]) != 0 && argv[i][0] != '-') {
            targetCmd = argv[i];
            break;
        }
    }

    if (targetCmd.empty()) {
        PrintFullHelp();
        return 0;
    }

    auto it = gCommands.find(targetCmd);
    if (it == gCommands.end()) {
        return OutputError("ERR_NL_INVALID_COMMAND", "Unknown command: " + targetCmd,
            "Run '" + std::string(gProgramName) + " --help' to see available commands.");
    }
    PrintCommandHelp(it->second);
    return 0;
}

static std::string GetErrorMsg(NlErrCode ret)
{
    switch (ret) {
        case NL_ERR_PERMISSION_FAILED:
            return "Permission denied: ACCESS_NEARLINK or MANAGE_NEARLINK permission required";
        case NL_ERR_INVALID_PARAM:
            return "Invalid parameter provided";
        case NL_ERR_API_NOT_SUPPORT:
            return "NearLink is not supported on this device";
        case NL_ERR_SERVICE_DISCONNECTED:
            return "NearLink service is not available or disconnected";
        case NL_ERR_INTERNAL_ERROR:
            return "Internal error occurred in NearLink service";
        default:
            return "Unknown error occurred, error code: " + std::to_string(ret);
    }
}

static std::string GetErrorCode(NlErrCode ret)
{
    switch (ret) {
        case NL_ERR_PERMISSION_FAILED:
            return "ERR_NL_PERMISSION_DENIED";
        case NL_ERR_INVALID_PARAM:
            return "ERR_NL_INVALID_PARAM";
        case NL_ERR_API_NOT_SUPPORT:
            return "ERR_NL_API_NOT_SUPPORT";
        case NL_ERR_SERVICE_DISCONNECTED:
            return "ERR_NL_SERVICE_DISCONNECTED";
        case NL_ERR_INTERNAL_ERROR:
            return "ERR_NL_INTERNAL_ERROR";
        default:
            return "ERR_NL_UNKNOWN";
    }
}

static std::string GetSuggestion(NlErrCode ret)
{
    switch (ret) {
        case NL_ERR_PERMISSION_FAILED:
            return "Please add required permissions in module.json5 and ensure user has granted them";
        case NL_ERR_INVALID_PARAM:
            return "Please check the parameter values and try again";
        case NL_ERR_API_NOT_SUPPORT:
            return "This device does not support NearLink functionality";
        case NL_ERR_SERVICE_DISCONNECTED:
            return "Please ensure NearLink service is running. Try restarting the device.";
        case NL_ERR_INTERNAL_ERROR:
            return "Please try again or restart the device";
        default:
            return "Please check the system logs for more details";
    }
}

static int HandleNlResult(NlErrCode ret)
{
    if (ret == NL_NO_ERROR) {
        return 0;
    }
    return OutputError(GetErrorCode(ret), GetErrorMsg(ret), GetSuggestion(ret));
}

static int CmdEnable(int argc, char** argv)
{
    int32_t autoConnPolicy = 0;

    for (int i = 0; i < argc; i++) {
        if (argv[i] == nullptr || strlen(argv[i]) > MAX_ARGV_LENGTH) {
            return 1;
        }
        if (strncmp(argv[i], "--autoConnPolicy", MAX_ENABLE_PARAM_LENGTH) == 0 && 
            argv[i][MAX_ENABLE_PARAM_LENGTH] == '\0' && i + 1 < argc &&
            argv[i + 1] != nullptr && strlen(argv[i + 1]) <= MAX_ARGV_LENGTH) {
            autoConnPolicy = atoi(argv[i + 1]);
        } else if (strncmp(argv[i], "--help", MAX_CMD_HELP_LENGTH) == 0 && argv[i][MAX_CMD_HELP_LENGTH] == '\0') {
            return CmdHelp(argc, argv);
        }
    }

    if (autoConnPolicy < 0 || autoConnPolicy > 2) {
        return OutputError("ERR_NL_INVALID_PARAM",
            "Invalid autoConnPolicy value: " + std::to_string(autoConnPolicy),
            "autoConnPolicy must be 0 (AUTO_CONN_GENERAL), 1 (AUTO_CONN_EXCEPT_AUDIO_DEVICES), "
            "or 2 (AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES). Example: --autoConnPolicy 0");
    }

    auto& host = OHOS::Nearlink::NearlinkHost::GetInstance();
    OHOS::Nearlink::NlErrCode ret = host.EnableNl(
        static_cast<OHOS::Nearlink::SleAutoConnectPolicy>(autoConnPolicy));

    if (ret == OHOS::Nearlink::NL_NO_ERROR) {
        std::ostringstream oss;
        oss << "NearLink enabled successfully, autoConnPolicy: " << autoConnPolicy;
        return OutputSuccess(oss.str());
    }

    return HandleNlResult(ret);
}

static int CmdDisable(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        if (argv[i] == nullptr || strlen(argv[i]) > MAX_ARGV_LENGTH) {
            return 1;
        }
        if (strncmp(argv[i], "--help", MAX_CMD_HELP_LENGTH) == 0 && argv[i][MAX_CMD_HELP_LENGTH] == '\0') {
            return CmdHelp(argc, argv);
        }
    }

    auto& host = OHOS::Nearlink::NearlinkHost::GetInstance();
    OHOS::Nearlink::NlErrCode ret = host.DisableNl();

    if (ret == OHOS::Nearlink::NL_NO_ERROR) {
        return OutputSuccess("NearLink disabled successfully");
    }

    return HandleNlResult(ret);
}

static void InitCommands()
{
    REGISTER_CMD("help", "Show this help message",
        "ohos-nearlinkControl help [command]",
        "    command          Optional. Show detailed help for a specific command.",
        "    ohos-nearlinkControl help\n"
        "    ohos-nearlinkControl help enable\n"
        "    ohos-nearlinkControl help disable",
        CmdHelp);

    REGISTER_CMD("enable", "Enable NearLink functionality",
        "ohos-nearlinkControl enable [--autoConnPolicy <number>]",
        "    --autoConnPolicy  Optional integer. Auto-connect policy (0: AUTO_CONN_GENERAL, "
        "1: AUTO_CONN_EXCEPT_AUDIO_DEVICES, 2: AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES). "
        "Default: 0",
        "    # Enable NearLink with default policy\n"
        "    ohos-nearlinkControl enable\n"
        "    # Enable NearLink with policy to except audio devices\n"
        "    ohos-nearlinkControl enable --autoConnPolicy 1",
        CmdEnable);

    REGISTER_CMD("disable", "Disable NearLink functionality",
        "ohos-nearlinkControl disable",
        "    (no additional parameters)",
        "    # Disable NearLink\n"
        "    ohos-nearlinkControl disable",
        CmdDisable);

    gHasSubcommands = (gCommands.size() > 1);
}

static void PrintUsage(const char* prog)
{
    CLI_ERROR("Usage: %s <command> [options...]", prog);
    CLI_ERROR("Run '%s help' for more information", prog);
}

} // namespace Nearlink
} // namespace OHOS

int main(int argc, char** argv)
{
    using namespace OHOS::Nearlink;
    // 命令行参数个数必须大于等于2个: ./ohos-nearlinkControl <command> [options...]
    if (argv == nullptr || argv[0] == nullptr || strlen(argv[0]) > MAX_ARGV_LENGTH) {
        return 1;
    }

    gProgramName = argv[0];
    if (argc < MIN_ARGC_WITH_COMMAND) {
        PrintUsage(gProgramName);
        return 1;
    }
    if (argv[1] == nullptr || strlen(argv[1]) > MAX_ARGV_LENGTH) {
        PrintUsage(gProgramName);
        return 1;
    }

    if (argc >= MIN_ARGC_WITH_COMMAND && strncmp(argv[1], "--help", MAX_CMD_HELP_LENGTH) == 0  &&
        argv[1][MAX_CMD_HELP_LENGTH] == '\0') {
        InitCommands();
        char helpArgv[HELP_ARGV_SIZE] = {0};
        char* helpArgvPtr[HELP_ARGV_SIZE] = {helpArgv, nullptr};
        CmdHelp(1, helpArgvPtr);
        return 0;
    }

    InitCommands();

    std::string cmdName = argv[1];

    for (int i = ARGV_CMD_PARAM_START_INDEX; i < argc; i++) {
        if (argv[i] == nullptr || strlen(argv[i]) > MAX_ARGV_LENGTH) {
            PrintUsage(gProgramName);
            return 1;
        }
        if (strncmp(argv[i], "--help", MAX_CMD_HELP_LENGTH) == 0 && argv[i][MAX_CMD_HELP_LENGTH] == '\0') {
            char helpArgv[HELP_ARGV_SIZE] = {0};
            char* helpArgvPtr[HELP_ARGV_SIZE] = {helpArgv, const_cast<char*>(cmdName.c_str())};
            CmdHelp(2, helpArgvPtr);
            return 0;
        }
    }

    auto it = gCommands.find(cmdName);
    if (it == gCommands.end()) {
        return OutputError("ERR_NL_INVALID_COMMAND", std::string("Unknown command: ") + cmdName,
            std::string("Run ") + gProgramName + " --help to see available commands.");
    }

    return it->second.handler(argc - MIN_ARGC_WITH_COMMAND, argv + ARGV_CMD_PARAM_START_INDEX);
}
