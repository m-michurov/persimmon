cmake_minimum_required(VERSION 3.26)
project(_persimmon C)

set(CMAKE_C_STANDARD 17)

add_compile_options(-Wall -Werror -Wextra)
add_executable(persimmon
        main.c
        lexer/token.c
        lexer/lexer.c
        common/file_utils/file_utils.c
)
target_include_directories(persimmon PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external)
