cmake_minimum_required(VERSION 3.28)
project(arena C)

set(CMAKE_C_STANDARD 23)

add_executable(arena
        main.c
        arena.c
        scanner.c
        object.c
        string_builder.c
        eval.c
        syntax_error.c
        object_lists.c
        parser.c
        object_accessors.c
        stack.c
        object_constructors.c
        object_allocator.c
        primitives.c
        line_reader.c
        strings.c
        object_env.c
        reader.c
        eval_errors.c
        eval_errors.h
)

target_compile_options(arena PRIVATE
        -Wall
        -Werror
        -Wextra
)
