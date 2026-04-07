set(TARGET_NAME promo_notify)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
# Исключаем все файлы из папки tests
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/tests/.*\\.cpp$")

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(${TARGET_NAME} PRIVATE
    Boost::program_options
    Boost::system
    Boost::date_time
    CURL::libcurl
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    Threads::Threads
    Crow::Crow
)

target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)