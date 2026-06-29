aux_source_directory(link/src CM_SRC)
aux_source_directory(signaling/src CM_SRC)

add_library(ncp_cm OBJECT ${CM_SRC})

target_include_directories(
    ncp_cm PUBLIC
    interface
    common/include
    link/include
    link/internal
    signaling/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../dp/dtap/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../../nbc/include
)

target_link_libraries(ncp_cm PUBLIC sdf_dsl sdf_memm sdf_log sdf_utils dli cp_nbc ${SECUREC_LIB})