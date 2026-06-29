aux_source_directory(src UTILS_SRC)
add_library(sdf_utils OBJECT ${UTILS_SRC})
target_include_directories(sdf_utils PUBLIC include)
target_link_libraries(sdf_utils sdf_dsl)