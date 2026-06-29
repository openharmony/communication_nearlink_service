aux_source_directory(src DP_TRANSPORT_SRC)

add_library(dp_transport OBJECT ${DP_TRANSPORT_SRC})

target_include_directories(
    dp_transport PUBLIC 
    include
    interface
    ${DP_COMMON_INCLUDE}
)

target_link_libraries(dp_transport PUBLIC dp_dtap sdf_dsl sdf_memm sdf_log ${SECUREC_LIB})