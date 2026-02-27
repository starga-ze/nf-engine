file(GLOB_RECURSE SHARED_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(nf-shared ${SHARED_SRC})

target_include_directories(nf-shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nf-shared PUBLIC
    spdlog::spdlog
)
