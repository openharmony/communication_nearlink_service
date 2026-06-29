aux_source_directory(src SLEM_SRC)

add_library(nai_slem OBJECT ${SLEM_SRC})

target_link_libraries(nai_slem nai_api)

set(NAI_SLEM_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(nai_slem PUBLIC ${NAI_SLEM_HEADER})