aux_source_directory(src DEVD_SRC)

add_library(ncp_devd OBJECT ${DEVD_SRC})

target_include_directories(ncp_devd PUBLIC include)

target_link_libraries(ncp_devd PUBLIC sdf_memm sdf_dsl sdf_lock sdf_utils dli ${SECUREC_LIB})