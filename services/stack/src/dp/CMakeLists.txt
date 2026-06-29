set(DP_COMMON_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/dpfwk/errcode
    ${CMAKE_CURRENT_SOURCE_DIR}/dpfwk/log
    ../sdf/errno/include
    ../sdf/log/include
    )
add_subdirectory(dtap)
add_subdirectory(transport)