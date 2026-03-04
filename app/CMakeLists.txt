
set(TARGET_NAME promo_notify)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${TARGET_NAME} ${SOURCES})

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
target_sources(${TARGET_NAME} PRIVATE ${HEADERS})

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}           
    ${CMAKE_SOURCE_DIR}/app
)


target_link_libraries(${TARGET_NAME} PRIVATE
    nlohmann_json::nlohmann_json          # JSON
    spdlog::spdlog_header_only            # Логирование (header-only)
    Crow::Crow                            # Web-фреймворк
    CURL::libcurl                         # HTTP-клиент
    SQLite::SQLite3                       # БД
    Threads::Threads                      # Потоки (кроссплатформенная замена pthread)
)

target_compile_options(${TARGET_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX->
)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)


if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${TARGET_NAME} PRIVATE -O3 -DNDEBUG)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
    if(IPO_SUPPORTED)
        set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "DB_PATH=${CMAKE_SOURCE_DIR}/data/dev.db"
)