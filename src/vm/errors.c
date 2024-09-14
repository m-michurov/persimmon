#include "errors.h"

#include <string.h>

#include "utility/guards.h"
#include "utility/string_builder.h"
#include "object/constructors.h"
#include "object/list.h"
#include "object/accessors.h"
#include "object/repr.h"
#include "traceback.h"

#define ERROR_FIELD_EXPECTED  "expected"
#define ERROR_FIELD_GOT       "got"
#define ERROR_FIELD_TYPE      "type"
#define ERROR_FIELD_NAME      "name"

#define MESSAGE_MIN_CAPACITY 512

static void report_out_of_system_memory(VirtualMachine *vm, Object *error_type) {
    fprintf(
            stderr,
            "ERROR: ran out of system memory when creating an exception of type %s.\n",
            object_as_atom(error_type)
    );
    traceback_print_from_stack(*vm_stack(vm), stderr);
}

static void report_out_of_memory(VirtualMachine *vm, Object *error_type) {
    fprintf(
            stderr,
            "ERROR: VM heap capacity exceeded when creating an exception of type %s.\n",
            object_as_atom(error_type)
    );
    allocator_print_statistics(vm_allocator(vm), stderr);
    traceback_print_from_stack(*vm_stack(vm), stderr);
}

static bool try_create_int_field(VirtualMachine *vm, char const *key, int64_t value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, OBJECT_NIL, OBJECT_NIL)
           && object_try_make_atom(a, key, object_list_nth_mutable(0, *field))
           && object_try_make_int(a, value, object_list_nth_mutable(1, *field));
}

static bool try_create_string_field(VirtualMachine *vm, char const *key, char const *value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, OBJECT_NIL, OBJECT_NIL)
           && object_try_make_atom(a, key, object_list_nth_mutable(0, *field))
           && object_try_make_string(a, value, object_list_nth_mutable(1, *field));
}

static bool try_create_atom_field(VirtualMachine *vm, char const *key, char const *value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, OBJECT_NIL, OBJECT_NIL)
           && object_try_make_atom(a, key, object_list_nth_mutable(0, *field))
           && object_try_make_atom(a, value, object_list_nth_mutable(1, *field));
}

static bool try_create_object_field(VirtualMachine *vm, char const *key, Object *value, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, OBJECT_NIL, value)
           && object_try_make_atom(a, key, object_list_nth_mutable(0, *field));
}

static bool try_create_traceback(VirtualMachine *vm, Object **field) {
    guard_is_not_null(vm);
    guard_is_not_null(field);

    auto const a = vm_allocator(vm);
    return object_try_make_list(a, field, OBJECT_NIL, OBJECT_NIL)
           && object_try_make_atom(a, ERROR_FIELD_TRACEBACK, object_list_nth_mutable(0, *field))
           && traceback_try_get(a, *vm_stack(vm), object_list_nth_mutable(1, *field));
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
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

// TODO dont create unnecessary fields, only use message and traceback
// TODO use dicts for errors?
void create_os_error(VirtualMachine *vm, errno_t error_code) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_OS_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(
                    vm,
                    ERROR_FIELD_MESSAGE,
                    strerror(error_code),
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_int_field(vm, "errno", error_code, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

#define snprintf_checked(Dst, Remaining, Format, ...)                                                   \
do {                                                                                                    \
    auto const _written = guard_succeeds(snprintf, (*(Dst), *(Remaining), (Format), ##__VA_ARGS__));    \
    guard_is_less((size_t) _written, *(Remaining));                                                     \
    *(Dst) += _written;                                                                                 \
    *(Remaining) -= _written;                                                                           \
} while (false)

static void create_message_type_error(
        size_t capacity,
        char *message,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
) {
    guard_is_not_null(message);
    guard_is_greater(capacity, 0);
    guard_is_not_null(expected);
    guard_is_greater(expected_count, 0);

    snprintf_checked(&message, &capacity, "unsupported type (expected %s", object_type_str(expected[0]));

    for (size_t i = 1; i + 1 < expected_count; i++) {
        snprintf_checked(&message, &capacity, ", %s", object_type_str(expected[i]));
    }

    if (expected_count > 1) {
        snprintf_checked(&message, &capacity, " or %s", object_type_str(expected[expected_count - 1]));
    }
    snprintf_checked(&message, &capacity, ", got %s)", object_type_str(got));
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

    char message[MESSAGE_MIN_CAPACITY] = {0};
    create_message_type_error(sizeof(message), message, got, expected_count, expected);

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)));

    auto const expected_types_list = object_list_nth_mutable(++field_index, *vm_error(vm));
    for (size_t i = 0; ok && i < expected_count; i++) {
        ok = object_try_make_cons(a, OBJECT_NIL, *expected_types_list, expected_types_list)
             && object_try_make_atom(a, object_type_str(expected[i]), object_list_nth_mutable(0, *expected_types_list));
    }

    ok = ok
         && object_try_make_cons(a, OBJECT_NIL, *expected_types_list, expected_types_list)
         && object_try_make_atom(a, ERROR_FIELD_EXPECTED, object_list_nth_mutable(0, *expected_types_list))
         &&
         try_create_atom_field(vm, ERROR_FIELD_GOT, object_type_str(got), object_list_nth_mutable(++field_index, *vm_error(vm)))
         && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_type_error_unexpected(VirtualMachine *vm, Object_Type got) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_TYPE_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    size_t capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "unsupported type (got %s)", object_type_str(got));

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)));

    ok = ok
         && try_create_atom_field(vm, ERROR_FIELD_GOT, object_type_str(got), object_list_nth_mutable(++field_index, *vm_error(vm)))
         && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

static void pad(char **message, size_t *capacity, size_t padding) {
    for (size_t i = 0; i < padding; i++) {
        snprintf_checked(message, capacity, " ");
    }
}

static void create_message_syntax_error(
        size_t capacity,
        char *message,
        SyntaxError base,
        char const *file,
        char const *text
) {
    guard_is_not_null(message);
    guard_is_greater(capacity, 0);
    guard_is_not_null(file);
    guard_is_not_null(text);

    snprintf_checked(&message, &capacity, "%s\n", syntax_error_str(base.code));
    auto const padding = 2 + guard_succeeds(snprintf, (nullptr, 0, "%zu", base.pos.lineno));

    pad(&message, &capacity, padding - 1);
    snprintf_checked(&message, &capacity, "--> %s\n", file);

    pad(&message, &capacity, padding);
    snprintf_checked(&message, &capacity, "|\n");

    snprintf_checked(&message, &capacity, " %zu | %s", base.pos.lineno, text);

    pad(&message, &capacity, padding);
    snprintf_checked(&message, &capacity, "| ");

    pad(&message, &capacity, base.pos.col);
    for (size_t i = 0; i < base.pos.end_col - base.pos.col + 1; i++) {
        snprintf_checked(&message, &capacity, "^");
    }
}

void create_syntax_error(
        VirtualMachine *vm,
        SyntaxError error,
        char const *file,
        char const *text
) {
    guard_is_not_null(vm);
    guard_is_not_null(file);
    guard_is_not_null(text);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_SYNTAX_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    size_t const capacity = MESSAGE_MIN_CAPACITY + 2 * strlen(text);
    char *message = calloc(capacity, sizeof(char));
    if (nullptr == message) {
        *vm_error(vm) = default_error;
        report_out_of_system_memory(vm, error_type);
        return;
    }
    create_message_syntax_error(capacity, message, error, file, text);

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(
                    vm,
                    ERROR_FIELD_MESSAGE, message,
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_string_field(vm, "file", file, object_list_nth_mutable(++field_index, *vm_error(vm)))
            &&
            try_create_int_field(vm, "line", (int64_t) error.pos.lineno, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_int_field(vm, "col", (int64_t) error.pos.col, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_int_field(
                    vm,
                    "end_col",
                    (int64_t) error.pos.end_col,
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_string_field(vm, "text", text, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_call_args_count_error(VirtualMachine *vm, char const *name, size_t expected, bool is_variadic, size_t got) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_CALL_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    size_t capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(
            &buf, &capacity,
            "%s takes %s%zu arguments (got %zu)",
            name, (is_variadic ? "at least " : ""), expected, got
    );

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_string_field(vm, ERROR_FIELD_NAME, name, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_int_field(
                    vm,
                    ERROR_FIELD_EXPECTED,
                    (int64_t) expected,
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_int_field(vm, ERROR_FIELD_GOT, (int64_t) got, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_call_args_parity_error(VirtualMachine *vm, char const *name, bool expected_even) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_CALL_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    size_t capacity = sizeof(message);
    auto buf = message;

    auto const expected_str = expected_even ? "even" : "odd";
    snprintf_checked(&buf, &capacity, "%s takes an %s number of arguments", name, expected_str);

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_string_field(vm, ERROR_FIELD_NAME, name, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_atom_field(
                    vm,
                    ERROR_FIELD_EXPECTED,
                    expected_str,
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

void create_call_ampersand_before_error(VirtualMachine *vm) {
    create_error_with_message(
            vm,
            vm_get(vm, STATIC_CALL_ERROR_DEFAULT),
            "'.' must be preceded by at least one expression"
    );
}

void create_call_ampersand_after_error(VirtualMachine *vm) {
    create_error_with_message(
            vm,
            vm_get(vm, STATIC_CALL_ERROR_DEFAULT),
            "'.' must be followed by exactly one expression"
    );
}

void create_call_extra_args_type_error(VirtualMachine *vm, Object_Type extras_type) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_CALL_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    size_t capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "extra arguments must be a list (got %s)", object_type_str(extras_type));

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_atom_field(
                    vm,
                    ERROR_FIELD_GOT,
                    object_type_str(extras_type),
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
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

    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "name '%s' is not defined", name);

    auto field_index = 0;
    auto const ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_string_field(vm, ERROR_FIELD_NAME, name, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
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

    object_print(error_type, stdout);
    printf("\n");
    allocator_print_statistics(vm_allocator(vm), stderr);
    traceback_print_from_stack(*vm_stack(vm), stderr);
}

void create_stack_overflow_error(VirtualMachine *vm) {
    create_error_with_message(vm, vm_get(vm, STATIC_STACK_OVERFLOW_ERROR_DEFAULT), "stack capacity exceeded");
}

void create_special_too_few_args_error(VirtualMachine *vm, char const *name) {
    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "too few arguments for %s", name);

    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_special_too_many_args_error(VirtualMachine *vm, char const *name) {
    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "too many arguments for %s", name);

    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_special_args_count_error(VirtualMachine *vm, char const *name, size_t expected) {
    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(
            &buf, &capacity,
            "%s takes exactly %zu argument%s",
            name, expected, (1 == expected % 10) ? "" : "s"
    );

    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

void create_parameters_declaration_error(VirtualMachine *vm, BindingTargetError error) {
    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;

    switch (error.type) {
        case BINDING_INVALID_TARGET_TYPE: {
            snprintf_checked(
                    &buf, &capacity,
                    "parameters declaration is invalid (got %s)",
                    object_type_str(error.as_invalid_target.target_type)
            );
            break;
        }
        case BINDING_INVALID_VARIADIC_SYNTAX: {
            snprintf_checked(&buf, &capacity, "invalid variadic parameters syntax");
            break;
        }
    }

    guard_is_not_equal(*message, '\0');

    create_error_with_message(vm, vm_get(vm, STATIC_SPECIAL_ERROR_DEFAULT), message);
}

static void message_create_bind_count_error(
        size_t capacity,
        char *message,
        size_t expected,
        bool is_varargs,
        size_t got
) {
    guard_is_not_null(message);
    guard_is_greater(capacity, 0);

    snprintf_checked(
            &message, &capacity,
            "cannot bind values (expected %s%zu, got %zu)",
            (is_varargs ? "at least " : ""), expected, got
    );
}

static void create_binding_count_error(VirtualMachine *vm, size_t expected, bool is_variadic, size_t got) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_BINDING_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    message_create_bind_count_error(sizeof(message), message, expected, is_variadic, got);

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            && try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_int_field(
                    vm,
                    ERROR_FIELD_EXPECTED,
                    (int64_t) expected,
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_atom_field(
                    vm,
                    "variadic",
                    (is_variadic ? "true" : "false"),
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_int_field(vm, ERROR_FIELD_GOT, (int64_t) got, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

static void create_binding_unpack_error(VirtualMachine *vm, Object_Type value_type) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_BINDING_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "cannot unpack an object of type %s", object_type_str(value_type));

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            &&
            try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_atom_field(
                    vm,
                    ERROR_FIELD_TYPE,
                    object_type_str(value_type),
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

static void create_binding_target_error(VirtualMachine *vm, Object_Type target_type) {
    guard_is_not_null(vm);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_BINDING_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    char message[MESSAGE_MIN_CAPACITY] = {0};
    auto capacity = sizeof(message);
    auto buf = message;
    snprintf_checked(&buf, &capacity, "cannot bind to %s", object_type_str(target_type));

    auto field_index = 0;
    auto ok =
            object_try_make_list(
                    a, vm_error(vm),
                    error_type,
                    OBJECT_NIL,
                    OBJECT_NIL,
                    OBJECT_NIL
            )
            &&
            try_create_string_field(vm, ERROR_FIELD_MESSAGE, message, object_list_nth_mutable(++field_index, *vm_error(vm)))
            && try_create_string_field(
                    vm,
                    ERROR_FIELD_TYPE,
                    object_type_str(target_type),
                    object_list_nth_mutable(++field_index, *vm_error(vm))
            )
            && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    *vm_error(vm) = default_error;
    report_out_of_memory(vm, error_type);
}

static void create_binding_variadic_syntax_error(VirtualMachine *vm) {
    create_error_with_message(
            vm, vm_get(vm, STATIC_BINDING_ERROR_DEFAULT),
            "invalid variadic binding syntax"
    );
}

void create_binding_error(VirtualMachine *vm, BindingError error) {
    guard_is_not_null(vm);

    switch (error.type) {
        case BINDING_INVALID_TARGET: {
            auto const target_error = error.as_target_error;
            switch (target_error.type) {
                case BINDING_INVALID_TARGET_TYPE: {
                    create_binding_target_error(vm, target_error.as_invalid_target.target_type);
                    return;
                }
                case BINDING_INVALID_VARIADIC_SYNTAX: {
                    create_binding_variadic_syntax_error(vm);
                    return;
                }
            }

            break;
        }
        case BINDING_INVALID_VALUE: {
            auto const value_error = error.as_value_error;
            switch (value_error.type) {
                case BINDING_VALUES_COUNT_MISMATCH: {
                    create_binding_count_error(
                            vm,
                            value_error.as_count_mismatch.expected,
                            value_error.as_count_mismatch.is_variadic,
                            value_error.as_count_mismatch.got
                    );
                    return;
                }
                case BINDING_CANNOT_UNPACK_VALUE: {
                    create_binding_unpack_error(vm, value_error.as_cannot_unpack.value_type);
                    return;
                }
            }

            break;
        }
        case BINDING_ALLOCATION_FAILED: {
            create_out_of_memory_error(vm);
            return;
        }
    }

    guard_unreachable();
}

void create_key_error(VirtualMachine *vm, Object *key) {
    guard_is_not_null(vm);
    guard_is_not_null(key);

    auto const a = vm_allocator(vm);
    auto const default_error = vm_get(vm, STATIC_KEY_ERROR_DEFAULT);
    auto const error_type = object_as_cons(default_error).first;

    auto sb = (StringBuilder) {0};
    errno_t error_code;
    auto ok =
            sb_try_printf_realloc(&sb, &error_code, "key does not exist: ")
            && object_try_repr(key, &sb, &error_code);

    auto field_index = 0;
    ok = ok
         && object_try_make_list(
                 a, vm_error(vm),
                 error_type,
                 OBJECT_NIL,
                 OBJECT_NIL,
                 OBJECT_NIL
         )
         && try_create_string_field(vm, ERROR_FIELD_MESSAGE, sb.str, object_list_nth_mutable(++field_index, *vm_error(vm)))
         && try_create_object_field(vm, "key", key, object_list_nth_mutable(++field_index, *vm_error(vm)))
         && try_create_traceback(vm, object_list_nth_mutable(++field_index, *vm_error(vm)));
    if (ok) {
        return;
    }

    guard_is_equal(error_code, ENOMEM);

    *vm_error(vm) = default_error;
    report_out_of_system_memory(vm, error_type);
}

