file(GLOB_RECURSE SERVER_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(nf-serverd ${SERVER_SRC_FILES})

target_include_directories(nf-serverd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(nf-serverd PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(nf-serverd PRIVATE
    spdlog::spdlog
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    odbc
    nlohmann_json::nlohmann_json
)

