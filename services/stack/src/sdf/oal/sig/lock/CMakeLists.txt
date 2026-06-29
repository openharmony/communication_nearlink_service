aux_source_directory(src LOCK_SRC)
 
add_library(sdf_lock OBJECT ${LOCK_SRC})

target_include_directories(sdf_lock PUBLIC include)

target_link_libraries(sdf_lock PUBLIC sdf_errno sdf_fwk ${SECUREC_LIB})
