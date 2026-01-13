cmake_minimum_required(VERSION 3.22)
project(ConwaysGameOfLife LANGUAGES C)

add_executable(${PROJECT_NAME} main.c font.c)

find_package(SDL3 REQUIRED)
target_link_libraries(${PROJECT_NAME} SDL3)
