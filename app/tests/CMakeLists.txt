set(TEST_TARGET promo_tests)

find_package(GTest REQUIRED)

add_executable(${TEST_TARGET}
    test_campaign_service.cpp
)

target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/app
)

target_link_libraries(${TEST_TARGET} PRIVATE
    GTest::gtest
    Threads::Threads
)

target_compile_options(${TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic)
