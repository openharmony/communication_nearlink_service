aux_source_directory(src DSL_SRC)

add_library(sdf_dsl OBJECT ${DSL_SRC})

target_include_directories(sdf_dsl PUBLIC include)

target_link_libraries(sdf_dsl sdf_fwk)