aux_source_directory(src MEMM_SRC)
 
add_library(sdf_memm OBJECT ${MEMM_SRC})

target_include_directories(sdf_memm PUBLIC include)

target_link_libraries(sdf_memm PUBLIC sdf_errno ${SECUREC_LIB})