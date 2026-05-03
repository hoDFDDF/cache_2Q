add_library(cache2q_lib INTERFACE)

target_include_directories(cache2q_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(2Q-cache source/2Q_cache.cpp)

target_link_libraries(2Q-cache PRIVATE cache2q_lib)


target_compile_options(2Q-cache PRIVATE
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
    target_compile_options(2Q-cache PRIVATE
        $<$<CONFIG:DEBUG>: -fsanitize=address -fno-omit-frame-pointer>
    )
    target_link_options(2Q-cache PRIVATE
        $<$<CONFIG:DEBUG>: -fsanitize=address>
    )
endif()

