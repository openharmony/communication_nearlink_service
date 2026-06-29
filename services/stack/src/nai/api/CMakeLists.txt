aux_source_directory(src API_SRC)

add_library(nai_api OBJECT ${API_SRC})

set(NAI_API_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/uapi_func
    ${CMAKE_CURRENT_SOURCE_DIR}/../naifwk/include)

target_include_directories(nai_api PUBLIC ${NAI_API_HEADER})

target_link_libraries(nai_api PUBLIC sdf_memm cp_sm cp_hadm cp_worker ncp_devd nai_init nai_cfg cp_ssap dp_transport cp_nbc)