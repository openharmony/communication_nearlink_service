aux_source_directory(src BNL_SRC)

add_library(ncp_bnl OBJECT ${BNL_SRC})

target_include_directories(ncp_bnl PUBLIC interface)

target_link_libraries(ncp_bnl PUBLIC sdf_memm sdf_dsl sdf_lock sdf_utils dli ${SECUREC_LIB})