aux_source_directory(src TRACE_SRC)

add_library(sdf_trace OBJECT ${TRACE_SRC})

target_include_directories(sdf_trace PUBLIC include)