cmake_minimum_required(VERSION 3.20)
project(project_mayhem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(project_mayhem src/project_mayhem.cpp)
target_compile_options(project_mayhem PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -O2>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -O2>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)
