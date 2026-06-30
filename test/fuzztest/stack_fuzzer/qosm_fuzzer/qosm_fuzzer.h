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
#ifndef TEST_QOSM_FUZZER_H
#define TEST_QOSM_FUZZER_H


#include <sys/socket.h>


#ifdef __cplusplus
extern "C" {
#endif
ssize_t FuzzRecvMsgStub(int sockfd, struct msghdr *msg, int flags);
#ifdef __cplusplus
}
#endif

#define FUZZ_PROJECT_NAME "qosm_fuzzer"

#endif  // TEST_QOSM_FUZZER_H
