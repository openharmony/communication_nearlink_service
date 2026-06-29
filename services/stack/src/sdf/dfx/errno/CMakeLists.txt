aux_source_directory(src ERRNO_SRC)

add_library(sdf_errno OBJECT ${ERRNO_SRC})

target_include_directories(sdf_errno PUBLIC include)