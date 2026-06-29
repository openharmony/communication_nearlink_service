aux_source_directory(testcase/nai/nlm TEST_SRC)
# aux_source_directory(testcase/nai/api TEST_SRC)
aux_source_directory(testcase/cp/cpfwk/dfx TEST_SRC)
aux_source_directory(testcase/cp/ncp/devd TEST_SRC)
aux_source_directory(testcase/cp/ncp/sm TEST_SRC)
aux_source_directory(testcase/cp/ssap TEST_SRC)
aux_source_directory(testcase/demo TEST_SRC)
aux_source_directory(testcase/sdf/bsl/lock TEST_SRC)
aux_source_directory(testcase/sdf/bsl/thread TEST_SRC)
aux_source_directory(testcase/sdf/bsl/evc TEST_SRC)
aux_source_directory(testcase/sdf/bsl/dsl TEST_SRC)
aux_source_directory(testcase/sdf/dfx/errno TEST_SRC)
aux_source_directory(testcase/sdf/dfx/log TEST_SRC)
aux_source_directory(testcase/dp/dpfwk TEST_SRC)
aux_source_directory(testcase/dp/dtap TEST_SRC)
aux_source_directory(testcase/dli/layer/thread TEST_SRC)
aux_source_directory(testcase/dli/layer/sapi TEST_SRC)
aux_source_directory(testcase/dli/layer/inner TEST_SRC)
aux_source_directory(testcase/dli TEST_SRC)
aux_source_directory(testcase/cp/signaling TEST_SRC)
aux_source_directory(testcase/cp/cm TEST_SRC)

if (BUILD_MODE STREQUAL "stack")
    add_compile_options(-fsanitize=cfi -fvisibility=hidden -flto)
endif()

aux_source_directory(stub TEST_SRC)

if (BUILD_MODE STREQUAL "ut_asan")
    set(EXECUTABLE_NAME "stack_ut_asan")
    set(SANITIZE_OPTION "-fsanitize=address")
elseif(BUILD_MODE STREQUAL "ut_tsan")
    set(EXECUTABLE_NAME "stack_ut_tsan")
    set(SANITIZE_OPTION "-fsanitize=thread")
elseif(BUILD_MODE STREQUAL "ut")
    set(EXECUTABLE_NAME "stack_ut")
    set(SANITIZE_OPTION "")
endif()

add_executable(${EXECUTABLE_NAME} ${TEST_SRC})

target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ${GTEST_HEAD}
    ${SECUREC_HEAD}
    ${CMAKE_CURRENT_SOURCE_DIR}/stub
    ${SOURCE_DIR}/cp/cpfwk/dfx/include
    # ${SOURCE_DIR}/cp/ncp/devd/include
    ${SOURCE_DIR}/dp/dpfwk/log
    ${SOURCE_DIR}/dp/dpfwk/errcode
    ${SOURCE_DIR}/dli/common
    ${SOURCE_DIR}/dli/layer/thread
    ${SOURCE_DIR}/dli/layer/sapi
    ${SOURCE_DIR}/dli/layer/inner
)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    stack
    pthread
    ${SECUREC_LIB}
    ${GTEST_LIB}
)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -fno-sanitize=undefined)

if (SANITIZE_OPTION)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC ${SANITIZE_OPTION})
    target_link_options(${EXECUTABLE_NAME} PUBLIC ${SANITIZE_OPTION})
endif()