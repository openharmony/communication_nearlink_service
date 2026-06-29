aux_source_directory(src SFWK_SRC)

add_library(sdf_fwk OBJECT ${SFWK_SRC})

target_include_directories(sdf_fwk PUBLIC include)

target_link_libraries(sdf_fwk PUBLIC sdf_memm sdf_log ${SECUREC_LIB})