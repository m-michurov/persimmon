cmake_minimum_required(VERSION 3.26)
project(_persimmon C)

set(CMAKE_C_STANDARD 17)

add_compile_options(-Wall -Werror -Wextra)
add_executable(persimmon
        src/main.c
        src/lexer/token.c
        src/lexer/lexer.c
        src/common/file_utils/file_utils.c
        src/parser/parser.c
        src/ast/ast.c
        src/runtime/object.c
        src/runtime/eval.c
        src/runtime/scope.c
        src/ast/match.c
)
target_include_directories(persimmon PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/external)
