cmake_minimum_required(VERSION 3.10)
project(GameStore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the chat uses std::thread, so the thread library is needed
find_package(Threads REQUIRED)

add_executable(game_store main.cpp)
target_link_libraries(game_store PRIVATE Threads::Threads)

if (MSVC)
    target_compile_options(game_store PRIVATE /W4)
else()
    target_compile_options(game_store PRIVATE -Wall -Wextra -pedantic)
endif()
