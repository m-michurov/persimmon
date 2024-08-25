cmake_minimum_required(VERSION 3.28)
project(persimmon C)

set(CMAKE_C_STANDARD 23)

add_executable(persimmon
        src/main.c
        src/utility/arena.c
        src/utility/string_builder.c
        src/utility/strings.c
        src/utility/pointers.c
        src/vm/reader/scanner.c
        src/vm/eval.c
        src/vm/reader/syntax_error.c
        src/vm/reader/parser.c
        src/vm/stack.c
        src/vm/primitives.c
        src/vm/reader/line_reader.c
        src/vm/reader/reader.c
        src/vm/virtual_machine.c
        src/vm/traceback.c
        src/vm/errors.c
        src/object/accessors.c
        src/object/constructors.c
        src/object/allocator.c
        src/object/lists.c
        src/object/object.c
        src/vm/env.c
        src/vm/reader/named_file.c
        src/vm/reader/named_file.h
        src/vm/constants.c
        src/vm/bindings.c
        src/object/repr.c
        src/utility/writer.c
        src/object/dict.c
        src/object/dict.h
        src/vm/variadic.c
        src/vm/variadic.h
)

target_compile_options(persimmon PRIVATE
        -Wall
        -Werror
        -Wextra
        -Wno-pointer-arith
)

target_include_directories(persimmon PRIVATE src)
