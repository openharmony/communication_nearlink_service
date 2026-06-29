aux_source_directory(src LOG_SRC)

add_library(sdf_log OBJECT ${LOG_SRC})

set(SDF_LOG_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_include_directories(sdf_log PUBLIC ${SDF_LOG_HEADER})

target_link_libraries(sdf_log PUBLIC sdf_errno sdf_fsm ${SECUREC_LIB})