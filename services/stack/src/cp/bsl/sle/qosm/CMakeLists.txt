aux_source_directory(src QOSM_SRC)

add_library(cp_qosm OBJECT ${QOSM_SRC})

target_include_directories(
    cp_qosm PUBLIC 
    include
    interface
)

target_link_libraries(cp_qosm PUBLIC ncp_cm sdf_log sdf_utils dli)