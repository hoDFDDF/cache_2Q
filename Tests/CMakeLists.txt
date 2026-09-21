enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_library(cache_test_settings INTERFACE)

target_link_libraries(cache_test_settings
    INTERFACE
    cache2q_lib
    GTest::gmock_main
)

target_compile_options(cache_test_settings
    INTERFACE
    $<$<CONFIG:Debug>:-Wall>
    $<$<CONFIG:Debug>:-Wextra>
    $<$<CONFIG:Debug>:-Wpedantic>
    $<$<CONFIG:Debug>:-Wmissing-declarations>
    $<$<CONFIG:Debug>:-Wcast-align>
    $<$<CONFIG:Debug>:-Wshadow>
    $<$<CONFIG:Debug>:-Wsign-conversion>
)

if(ENABLE_ASAN)
    target_compile_options(cache_test_settings
        INTERFACE
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
    )

    target_link_options(cache_test_settings
        INTERFACE
        "$<$<CONFIG:Debug>:-fsanitize=address>"
    )
endif()

function(addCacheTest target_name)
    add_executable(${target_name} ${ARGN})

    target_link_libraries(${target_name}
        PRIVATE
        cache_test_settings
    )

    gtest_discover_tests(${target_name}
        TEST_PREFIX "${target_name}."
    )
endfunction()

addCacheTest(
    unit_tests_2q
    unit_tests/unit_tests.cpp
)

addCacheTest(
    unit_tests_lfu
    unit_tests/unit_tests_LFU.cpp
)
