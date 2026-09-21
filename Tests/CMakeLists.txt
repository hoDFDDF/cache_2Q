enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# LFU_cache.hpp includes gtest_prod.h for FRIEND_TEST declarations.
target_link_libraries(LFU-cache PRIVATE GTest::gtest)

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

if(UNIX)
    add_executable(lfu_benchmark e2e_tests/lfu_benchmark.cpp)
    target_compile_options(lfu_benchmark PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    add_dependencies(lfu_benchmark LFU-cache)

    function(addLfuE2e test_name)
        add_test(NAME ${test_name}
            COMMAND lfu_benchmark
                --cache-bin $<TARGET_FILE:LFU-cache>
                --output "${CMAKE_CURRENT_BINARY_DIR}/lfu_e2e_results"
                ${ARGN}
        )
        set_tests_properties(${test_name} PROPERTIES LABELS "lfu;e2e" TIMEOUT 120)
    endfunction()

    addLfuE2e(lfu_e2e_edges --self-test)
    addLfuE2e(lfu_e2e_zipf
        --requests 3000 --keys 50 --capacities 0,1,7,50,100
        --alphas 0.8,1,1.5 --seed 42 --runs 2
    )
endif()
