aux_source_directory(src DP_DTAP_SRC)

add_library(dp_dtap OBJECT ${DP_DTAP_SRC})

target_include_directories(
    dp_dtap PUBLIC 
    include
    interface
    ${DP_COMMON_INCLUDE}
)

target_link_libraries(dp_dtap PUBLIC dli ncp_cm sdf_dsl sdf_memm sdf_log ${SECUREC_LIB})
