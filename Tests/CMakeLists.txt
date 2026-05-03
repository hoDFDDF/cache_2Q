enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests unit_tests/unit_tests.cpp)

 target_link_libraries(unit_tests
    PRIVATE
    cache2q_lib
    GTest::gtest_main
 )

target_compile_options(unit_tests PRIVATE
    $<$<CONFIG:DEBUG>:
        -Wall
        -WINDOWS_EXPORT_ALL_SYMBOLS
        -Wpedantic
        -DDEBUG
        -O3
        -Wmissing-declarations
        -Wcast-align
        -Wshadow
        -Wsign-conversion>
)
if (ENABLE_ASAN)
    target_compile_options(unit_tests PRIVATE
        $<$<CONFIG:DEBUG>: -fsanitize=address -fno-omit-frame-pointer>
    )
    target_link_options(unit_tests PRIVATE
        $<$<CONFIG:DEBUG>: -fsanitize=address>
    )
endif()

include(GoogleTest)
gtest_discover_tests(unit_tests)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(
    NAME e2e_tests
    COMMAND Python3::Interpreter run.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/e2e_tests
)