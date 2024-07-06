cmake_minimum_required(VERSION 3.28)
project(persimmon C)

set(CMAKE_C_STANDARD 23)

add_executable(persimmon
        src/main.c
        src/utility/arena.c
        src/vm/reader/scanner.c
        src/object/object.c
        src/utility/string_builder.c
        src/vm/eval.c
        src/vm/reader/syntax_error.c
        src/object/lists.c
        src/vm/reader/parser.c
        src/object/accessors.c
        src/vm/stack.c
        src/object/constructors.c
        src/object/allocator.c
        src/vm/primitives.c
        src/vm/reader/line_reader.c
        src/utility/strings.c
        src/object/env.c
        src/vm/reader/reader.c
        src/vm/eval_errors.c
        src/utility/pointers.c
        src/vm/virtual_machine.c
)

target_compile_options(persimmon PRIVATE
        -Wall
        -Werror
        -Wextra
        -Wno-pointer-arith
)

target_include_directories(persimmon PRIVATE src)
