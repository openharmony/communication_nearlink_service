aux_source_directory(src SSAP_SRC)

add_library(cp_ssap OBJECT ${SSAP_SRC})

target_include_directories(cp_ssap PUBLIC include)

target_link_libraries(cp_ssap PUBLIC sdf_dsl sdf_utils dp_dtap cp_sm ${SECUREC_LIB})