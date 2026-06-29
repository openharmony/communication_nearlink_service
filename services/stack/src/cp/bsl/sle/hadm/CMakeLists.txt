aux_source_directory(src HADM_SRC)

add_library(cp_hadm OBJECT ${HADM_SRC})

target_include_directories(cp_hadm PUBLIC include)

target_link_libraries(cp_hadm PUBLIC ncp_cm sdf_log sdf_utils dli cp_nbc)