aux_source_directory(src SM_SRC)

add_library(cp_sm OBJECT ${SM_SRC})

target_include_directories(cp_sm PUBLIC include)

target_link_libraries(cp_sm PUBLIC ncp_cm sdf_dsl sdf_log sdf_memm sdf_stm sdf_utils dli)