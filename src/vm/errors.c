#include "errors.h"

#include <string.h>

#include "utility/guards.h"
#include "object/constructors.h"
#include "object/lists.h"
#include "object/accessors.h"
#include "traceback.h"

#define FIELD_EXPECTED  "expected"
#define FIELD_GOT       "got"
#define FIELD_MESSAGE   "message"
#define FIELD_TRACEBACK "traceback"

static void report_out_of_memory(VirtualMachine *vm, Object *error_type) {
    fprintf(
            stderr,
            "ERROR: ran out of memory when creating an exception of type %s.\n",
            object_as_atom(error_type)
    );
    allocator_print_statistics(vm_allocator(vm), stderr);
    traceback_print_from_stack(vm_stack(vm), stderr);
}

static bool try_create_int_field(VirtualMachine *vm, char const *key, int64_t value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, object_nil(), object_nil())
           && object_try_make_atom(a, key, object_list_nth(0, *field))
           && object_try_make_int(a, value, object_list_nth(1, *field));
}

static bool try_create_string_field(VirtualMachine *vm, char const *key, char const *value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, object_nil(), object_nil())
           && object_try_make_atom(a, key, object_list_nth(0, *field))
           && object_try_make_string(a, value, object_list_nth(1, *field));
}

static bool try_create_atom_field(VirtualMachine *vm, char const *key, char const *value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, object_nil(), object_nil())
           && object_try_make_atom(a, key, object_list_nth(0, *field))
           && object_try_make_atom(a, value, object_list_nth(1, *field));
}

static bool try_create_traceback(VirtualMachine *vm, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, object_nil(), object_nil())
           && object_try_make_atom(a, FIELD_TRACEBACK, object_list_nth(0, *field))
           && traceback_try_get(a, vm_stack(vm), object_list_nth(1, *field));
}

static void create_error_with_message(VirtualMachine *vm, Object *default_error, char const *message) {
    guard_is_not_null(vm);
    guard_is_not_null(message);

    auto const a = vm_allocator(vm);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(vm, FIELD_MESSAGE, message, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_os_error(VirtualMachine *vm, errno_t error_code) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_OS_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(a, vm_error(vm), error_type, object_nil(), object_nil())
            && try_create_string_field(vm, FIELD_MESSAGE, strerror(error_code),
                                       object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_type_error_(
        VirtualMachine *vm,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
) {
    guard_is_not_null(vm);
    guard_is_not_null(expected);
    guard_is_greater(expected_count, 0);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_TYPE_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(vm, FIELD_MESSAGE, "unsupported type",
                                       object_list_nth(++field_index, *vm_error(vm)));

    auto const expected_types_list = object_list_nth(++field_index, *vm_error(vm));
    for (size_t i = 0; ok && i < expected_count; i++) {
        ok = object_try_make_cons(a, object_nil(), *expected_types_list, expected_types_list)
             && object_try_make_atom(a, object_type_str(expected[i]), object_list_nth(0, *expected_types_list));
    }

    ok = ok
         && object_try_make_cons(a, object_nil(), *expected_types_list, expected_types_list)
         && object_try_make_atom(a, FIELD_EXPECTED, object_list_nth(0, *expected_types_list))
         && try_create_atom_field(vm, FIELD_GOT, object_type_str(got), object_list_nth(++field_index, *vm_error(vm)))
         && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_syntax_error(
        VirtualMachine *vm,
        SyntaxError base,
        char const *file,
        char const *text
) {
    guard_is_not_null(vm);
    guard_is_not_null(file);
    guard_is_not_null(text);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_SYNTAX_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(
                    vm,
                    FIELD_MESSAGE, syntax_error_str(base.code),
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_string_field(vm, "file", file, object_list_nth(++field_index, *vm_error(vm)))
            &&
            try_create_int_field(vm, "line", (int64_t) base.pos.lineno, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_int_field(vm, "col", (int64_t) base.pos.col, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_int_field(vm, "end_col", (int64_t) base.pos.end_col,
                                    object_list_nth(++field_index, *vm_error(vm)))
            && try_create_string_field(vm, "text", text, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_call_error(VirtualMachine *vm, char const *name, size_t expected_args, size_t got_args) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_CALL_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(
                    vm,
                    FIELD_MESSAGE, "wrong number of arguments",
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_string_field(vm, "name", name, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_int_field(vm, FIELD_EXPECTED, (int64_t) expected_args,
                                    object_list_nth(++field_index, *vm_error(vm)))
            && try_create_int_field(vm, FIELD_GOT, (int64_t) got_args, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_name_error(VirtualMachine *vm, char const *name) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_NAME_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(
                    vm,
                    FIELD_MESSAGE, "undeclared identifier",
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_string_field(vm, "name", name, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_zero_division_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_ZERO_DIVISION_ERROR_DEFAULT), "division by zero");
}

void create_out_of_memory_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const default_error = vm_get(vm, STATIC_OUT_OF_MEMORY_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    *vm_error(vm) = default_error;
    object_repr_print(error_type, stdout);
    printf("\n");
    allocator_print_statistics(vm_allocator(vm), stderr);
    traceback_print_from_stack(vm_stack(vm), stderr);
}

void create_stack_overflow_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_STACK_OVERFLOW_ERROR_DEFAULT), "stack capacity exceeded");
}

void create_import_nesting_too_deep_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_TOO_MANY_DEFAULT), "too many nested imports");
}

void create_too_few_args_error(VirtualMachine *vm, char const *name) {
    char message[512] = {0};
    auto const written = snprintf(message, sizeof(message), "too few arguments for %s", name);
    guard_is_less(written, (typeof(written)) sizeof(message));
    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_too_many_args_error(VirtualMachine *vm, char const *name) {
    char message[512] = {0};
    auto const written = snprintf(message, sizeof(message), "too many arguments for %s", name);
    guard_is_less(written, (typeof(written)) sizeof(message));
    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_parameters_type_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), "parameters declaration is invalid");
}

void create_args_count_error(VirtualMachine *vm, char const *name, size_t expected) {
    char message[512] = {0};
    auto const written = snprintf(
            message,
            sizeof(message),
            "%s takes exactly %zu argument%s",
            name, expected, (1 == expected % 10) ? "" : "s"
    );
    guard_is_less(written, (typeof(written)) sizeof(message));
    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_import_path_type_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), "import path must be a string");
}

void create_binding_count_error(VirtualMachine *vm, size_t expected, bool at_least, size_t got) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_BIND_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            && try_create_string_field(
                    vm,
                    FIELD_MESSAGE, "cannot bind values",
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_int_field(
                    vm,
                    at_least ? "expected-at-least" : FIELD_EXPECTED,
                    (int64_t) expected,
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_int_field(vm, FIELD_GOT, (int64_t) got, object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_binding_unpack_error(VirtualMachine *vm, Object_Type value_type) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_TYPE_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            &&
            try_create_string_field(vm, FIELD_MESSAGE, "cannot unpack", object_list_nth(++field_index, *vm_error(vm)))
            && try_create_atom_field(
                    vm,
                    "type",
                    object_type_str(value_type),
                    object_list_nth(++field_index, *vm_error(vm))
            )
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_binding_target_error(VirtualMachine *vm, Object_Type target_type) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_TYPE_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil(),
                    object_nil()
            )
            &&
            try_create_string_field(vm, FIELD_MESSAGE, "cannot bind to target",
                                    object_list_nth(++field_index, *vm_error(vm)))
            && try_create_string_field(vm, "type", object_type_str(target_type),
                                       object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_binding_varargs_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_TYPE_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    object_nil(),
                    object_nil()
            )
            &&
            try_create_string_field(vm, FIELD_MESSAGE, "invalid variadic binding format",
                                    object_list_nth(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}
