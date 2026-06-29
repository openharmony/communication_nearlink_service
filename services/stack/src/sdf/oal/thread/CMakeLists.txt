aux_source_directory(src THREAD_SRC)

add_library(sdf_thread OBJECT ${THREAD_SRC})

target_include_directories(sdf_thread PUBLIC include)

target_link_libraries(sdf_thread PUBLIC sdf_memm sdf_errno sdf_dsl sdf_lock ${SECUREC_LIB})