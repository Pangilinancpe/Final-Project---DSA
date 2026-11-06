cmake_minimum_required(VERSION 3.13)
project(GameStore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# the chat uses std::thread, so the thread library is needed
find_package(Threads REQUIRED)
# the headers are listed too, so CLion shows them under the target and rebuilds when they change

add_executable(game_store
        main.cpp
        store.h
        tui.h
        account.h
        browse.h
        cart.h
        chat.h
        friends.h
        game.h
        review.h
        sync.h
        screens.h
        textutil.h
        algorithms.h
        benchmark.h
        stack.h
        queue.h
        ring.h
        history.h
        downloads.h
        genre.h
        graph.h
        dp.h)
        
target_link_libraries(game_store PRIVATE Threads::Threads)

# put main.exe (it replaces the old prebuilt main.exe) in the project folder, next to the steam_*.txt data files.
# the program reads and writes those files in its working directory, so this way the
# exe finds the same data whether it runs from CLion or is opened from File Explorer.
set_target_properties(game_store PROPERTIES
        OUTPUT_NAME                      "main"
        RUNTIME_OUTPUT_DIRECTORY         "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}")

if (MSVC)
    target_compile_options(game_store PRIVATE /W4 /utf-8)
else()
    target_compile_options(game_store PRIVATE -Wall -Wextra -pedantic)
endif()

# Without this, main.exe only runs inside CLion. since we got an error(exit code -1073741515) on Windows.
# Which is the DLL is not found when clion is trying to launch the code
if (WIN32 AND NOT MSVC)
    target_link_options(game_store PRIVATE -static-libgcc -static-libstdc++)

    get_filename_component(COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(WINPTHREAD_DLL "${COMPILER_BIN_DIR}/libwinpthread-1.dll")
    if (EXISTS "${WINPTHREAD_DLL}")
        add_custom_command(TARGET game_store POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINPTHREAD_DLL}" "$<TARGET_FILE_DIR:game_store>")
    endif()
endif()

# Unit tests of the data structures and algorithms (optional):  cmake -DBUILD_TESTS=ON ..
# Each file in tests/ is a small program that stops with an assertion when something is wrong.
option(BUILD_TESTS "Build the unit tests in tests/" OFF)
if (BUILD_TESTS)
    enable_testing()
    foreach(name test_ds test_graph_dp test_tree_hist test_dl test_bench)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE Threads::Threads)
        # the tests use assert(), so keep it on even in a Release build
        target_compile_options(${name} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-UNDEBUG>)
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
        add_test(NAME ${name} COMMAND ${name})
    endforeach()
endif()
