aux_source_directory(src TIMER_SRC)

add_library(sdf_timer OBJECT ${TIMER_SRC})

target_include_directories(sdf_timer PUBLIC include)

target_link_libraries(sdf_timer PUBLIC sdf_memm sdf_evc)
