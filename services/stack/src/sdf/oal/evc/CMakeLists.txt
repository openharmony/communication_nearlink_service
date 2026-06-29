aux_source_directory(src EVC_SRC)

add_library(sdf_evc OBJECT ${EVC_SRC})

target_include_directories(sdf_evc PUBLIC include)

target_link_libraries(sdf_evc PUBLIC sdf_memm sdf_thread sdf_lock ${SECUREC_LIB})
