aux_source_directory(src CRYPTO_SRC)

add_library(nai_crypto OBJECT ${CRYPTO_SRC})

target_include_directories(nai_crypto PUBLIC
    include
    ../../naifwk/include
)

target_link_libraries(nai_crypto PUBLIC nai_api)