cmake_minimum_required(VERSION 3.16)
project(http_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -O2)

add_executable(http_server
    src/main.cpp
    src/server.cpp
    src/http.cpp
)

target_include_directories(http_server PRIVATE src)
target_link_libraries(http_server pthread)
