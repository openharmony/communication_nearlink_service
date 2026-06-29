aux_source_directory(src STM_SRC)

add_library(sdf_stm OBJECT ${STM_SRC})

target_include_directories(sdf_stm PUBLIC include)

target_link_libraries(sdf_stm PUBLIC sdf_memm sdf_errno sdf_fwk sdf_dsl ${SECUREC_LIB})