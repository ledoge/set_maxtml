cmake_minimum_required(VERSION 3.23)
project(set_maxtml C)

set(CMAKE_C_STANDARD 17)

add_executable(set_maxtml main.c)
target_link_libraries(set_maxtml "$<$<CONFIG:Release>:-s -static>")
