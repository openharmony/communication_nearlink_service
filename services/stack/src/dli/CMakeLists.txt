aux_source_directory(cmd/src DLI_SRC)
aux_source_directory(data/src DLI_SRC)
aux_source_directory(event/src DLI_SRC)
aux_source_directory(sapi/src DLI_SRC)
aux_source_directory(thread/src DLI_SRC)
aux_source_directory(layer/src DLI_SRC)
aux_source_directory(channel/src DLI_SRC)
aux_source_directory(dft/src DLI_SRC)

add_library(
    dli  OBJECT
    dli.c
    ${DLI_SRC}
)

# 头文件
target_include_directories(
    dli PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/cmd/include
    ${CMAKE_CURRENT_SOURCE_DIR}/event/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sapi/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thread/include
    ${CMAKE_CURRENT_SOURCE_DIR}/layer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/channel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dft/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdf/errno/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdf/log/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdf/trace/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdf/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../nai/api/include/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../cp/cpfwk/worker/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cp/cpfwk/dfx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../dp/dtap/include
)

target_link_libraries(
    dli PUBLIC
    sdf_dsl
    sdf_thread
    sdf_evc
    sdf_timer
    sdf_log
    sdf_trace
    sdf_lock
    ${SECUREC_LIB}
)
