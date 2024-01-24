cmake_minimum_required(VERSION 3.26)
project(_persimmon C)

set(CMAKE_C_STANDARD 17)

add_compile_options(-Wall -Werror -Wextra)
add_executable(persimmon main.c lexer/token.c common/dynamic_array/dynamic_array.c)
target_include_directories(persimmon PRIVATE ${CMAKE_SOURCE_DIR})
