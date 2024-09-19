#include "errors.h"

#include <string.h>

#include "utility/guards.h"
#include "utility/string_builder.h"
#include "object/constructors.h"
#include "object/list.h"
#include "object/dict.h"
#include "object/accessors.h"
#include "object/repr.h"
#include "traceback.h"
#include "variadic.h"

#define ERROR_FIELD_EXPECTED  "expected"
#define ERROR_FIELD_GOT       "got"
#define ERROR_FIELD_TYPE      "type"
#define ERROR_FIELD_NAME      "name"

#define MESSAGE_MIN_CAPACITY 512

Object *const ERROR_KEY_TYPE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = ERROR_KEY_NAME_TYPE};
Object *const ERROR_KEY_MESSAGE = &(Object) {.type = TYPE_SYMBOL, .as_symbol = ERROR_KEY_NAME_MESSAGE};
Object *const ERROR_KEY_TRACEBACK = &(Object) {.type = TYPE_SYMBOL, .as_symbol = ERROR_KEY_NAME_TRACEBACK};

extern Object *const OBJECT_ERROR_OUT_OF_MEMORY;

bool error_try_unpack(Object *error, Object **type, Object **message, Object **traceback) {
    guard_is_not_null(error);
    guard_is_not_null(type);
    guard_is_not_null(message);
    guard_is_not_null(traceback);

    return TYPE_DICT == error->type
           && object_dict_try_get(error, ERROR_KEY_TYPE, type)
           && TYPE_SYMBOL == (*type)->type
           && object_dict_try_get(error, ERROR_KEY_MESSAGE, message)
           && TYPE_STRING == (*message)->type
           && object_dict_try_get(error, ERROR_KEY_TRACEBACK, traceback);
}

bool error_try_unpack_type(Object *error, Object **type) {
    guard_is_not_null(error);
    guard_is_not_null(type);

    return TYPE_DICT == error->type
           && 1 == error->as_dict.size
           && object_dict_try_get(error, ERROR_KEY_TYPE, type)
           && TYPE_SYMBOL == (*type)->type;
}

static void out_of_memory(VirtualMachine *vm, char const *error_type) {
    guard_is_not_null(vm);
    guard_is_not_null(error_type);

    fprintf(
            stderr,
            "ERROR: VM heap capacity exceeded when creating an exception of type %s.\n",
            error_type
    );

    allocator_print_statistics(&vm->allocator, stderr);
    traceback_print_from_stack(&vm->stack, stderr);
}

static void system_error(VirtualMachine *vm, errno_t error_code, char const *error_type) {
    guard_is_not_null(vm);
    guard_is_not_null(error_type);

    fprintf(
            stderr,
            "ERROR: An error occurred when creating an exception of type %s: %s.\n",
            error_type, strerror(error_code)
    );

    allocator_print_statistics(&vm->allocator, stderr);
    traceback_print_from_stack(&vm->stack, stderr);
}

static bool try_make_children_roots(ObjectAllocator *a, Object **base_root, Object ***_1, Object ***_2) {
    guard_is_not_null(a);
    guard_is_not_null(base_root);
    guard_is_not_null(_1);
    guard_is_not_null(_2);

    if (false == object_try_make_list_of(a, base_root, OBJECT_NIL, OBJECT_NIL)) {
        return false;
    }

    *_1 = object_list_nth_mutable(0, *base_root);
    *_2 = object_list_nth_mutable(1, *base_root);

    return true;
}

static void set_error(VirtualMachine *vm, Object *error_type, char const *message) {
    guard_is_not_null(vm);
    guard_is_not_null(error_type);
    guard_is_not_null(message);
    guard_is_equal(error_type->type, TYPE_SYMBOL);

    auto const a = &vm->allocator;

    Object **tmp_error, **tmp_value;
    auto const fields_ok =
            try_make_children_roots(a, &vm->error, &tmp_error, &tmp_value)
            && object_dict_try_put(a, *tmp_error, ERROR_KEY_TYPE, error_type, tmp_error)
            && object_try_make_string(a, message, tmp_value)
            && object_dict_try_put(a, *tmp_error, ERROR_KEY_MESSAGE, *tmp_value, tmp_error)
            && traceback_try_get(a, &vm->stack, tmp_value)
            && object_dict_try_put(a, *tmp_error, ERROR_KEY_TRACEBACK, *tmp_value, tmp_error);

    if (false == fields_ok) {
        out_of_memory(vm, error_type->as_symbol);
        vm->error = OBJECT_ERROR_OUT_OF_MEMORY;
        return;
    }

    vm->error = *tmp_error;
}

static auto const SYMBOL_OS_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "OSError"};

void set_os_error(VirtualMachine *vm, errno_t error_code) {
    guard_is_not_null(vm);

    set_error(vm, SYMBOL_OS_ERROR, strerror(error_code));
}

static auto const SYMBOL_TYPE_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "TypeError"};

static bool try_format_type_expected_types_message(
        StringBuilder *sb,
        errno_t *error_code,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
) {
    guard_is_not_null(sb);
    guard_is_not_null(error_code);
    guard_is_not_null(expected);

    if (0 == expected_count) {
        return sb_try_printf(sb, error_code, "unsupported type: %s", object_type_str(got));
    }

    if (false ==
        sb_try_printf(sb, error_code, "unsupported type (expected %s", object_type_str(expected[0]))) {
        return false;
    }

    for (size_t i = 1; i + 1 < expected_count; i++) {
        if (false == sb_try_printf(sb, error_code, ", %s", object_type_str(expected[i]))) {
            return false;
        }
    }

    if (expected_count > 1) {
        if (false ==
            sb_try_printf(sb, error_code, " or %s", object_type_str(expected[expected_count - 1]))) {
            return false;
        }
    }

    return sb_try_printf(sb, error_code, ", got %s)", object_type_str(got));
}

void set_type_error_(
        VirtualMachine *vm,
        Object_Type got,
        size_t expected_count,
        Object_Type *expected
) {
    guard_is_not_null(vm);
    guard_is_not_null(expected);

    auto const error_type = SYMBOL_TYPE_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    if (false == try_format_type_expected_types_message(&sb, &error_code, got, expected_count, expected)) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

static auto const SYMBOL_SYNTAX_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "SyntaxError"};

static bool try_pad(StringBuilder *sb, errno_t *error_code, size_t padding) {
    guard_is_not_null(sb);
    guard_is_not_null(error_code);

    for (size_t i = 0; i < padding; i++) {
        if (false == sb_try_printf(sb, error_code, " ")) {
            return false;
        }
    }

    return true;
}

static bool try_print_highlight(StringBuilder *sb, errno_t *error_code, size_t offset, size_t count) {
    guard_is_not_null(sb);
    guard_is_not_null(error_code);

    if (false == try_pad(sb, error_code, offset)) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        if (false == sb_try_printf(sb, error_code, "^")) {
            return false;
        }
    }

    return true;
}

void set_syntax_error(
        VirtualMachine *vm,
        SyntaxError error,
        char const *file,
        char const *text
) {
    guard_is_not_null(vm);
    guard_is_not_null(file);
    guard_is_not_null(text);

    auto const error_type = SYMBOL_SYNTAX_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    auto const padding = 2 + guard_succeeds(snprintf, (nullptr, 0, "%zu", error.pos.lineno));
    auto const message_ok =
            sb_try_printf(&sb, &error_code, "%s\n", syntax_error_str(error.code))
            && try_pad(&sb, &error_code, padding - 1)
            && sb_try_printf(&sb, &error_code, "--> %s\n", file)
            && try_pad(&sb, &error_code, padding)
            && sb_try_printf(&sb, &error_code, "|\n")
            && sb_try_printf(&sb, &error_code, " %zu | %s", error.pos.lineno, text)
            && try_pad(&sb, &error_code, padding)
            && sb_try_printf(&sb, &error_code, "| ")
            && try_print_highlight(&sb, &error_code, error.pos.col, error.pos.end_col - error.pos.col + 1);

    if (false == message_ok) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

void set_variadic_syntax_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    set_error(
            vm,
            SYMBOL_SYNTAX_ERROR,
            "'" VARIADIC_AMPERSAND "' must be followed by exactly one expression"
    );
}

static bool try_format_special_syntax_error_message(
        StringBuilder *sb,
        errno_t *error_code,
        char const *special_name,
        size_t signatures_count,
        char const **signatures
) {
    guard_is_not_null(sb);
    guard_is_not_null(error_code);
    guard_is_not_null(special_name);
    guard_is_not_null(signatures);

    if (false == sb_try_printf(sb, error_code, "invalid '%s' syntax\nUsage:", special_name)) {
        return false;
    }

    for (size_t i = 0; i < signatures_count; i++) {
        auto const next_indent = i + 1 < signatures_count ? "\n       " : "";
        if (false == sb_try_printf(sb, error_code, " %s%s", signatures[i], next_indent)) {
            return false;
        }
    }

    return true;
}

void set_special_syntax_error_(
        VirtualMachine *vm,
        char const *name,
        size_t signatures_count,
        char const **signatures
) {
    guard_is_not_null(vm);
    guard_is_not_null(name);
    guard_is_greater(signatures_count, 0);
    guard_is_not_null(signatures);

    auto const error_type = SYMBOL_SYNTAX_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;
    if (false == try_format_special_syntax_error_message(&sb, &error_code, name, signatures_count, signatures)) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

static auto const SYMBOL_CALL_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "CallError"};

void set_call_args_count_error(VirtualMachine *vm, char const *name, size_t expected, size_t got) {
    guard_is_not_null(vm);
    guard_is_not_null(name);

    auto const error_type = SYMBOL_CALL_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    if (false == sb_try_printf(&sb, &error_code, "%s takes %zu arguments (got %zu)", name, expected, got)) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

static auto const SYMBOL_NAME_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "NameError"};

void set_name_error(VirtualMachine *vm, char const *name) {
    guard_is_not_null(vm);
    guard_is_not_null(name);

    auto const error_type = SYMBOL_NAME_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    if (false == sb_try_printf(&sb, &error_code, "name '%s' is not defined", name)) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

static auto const SYMBOL_ZERO_DIVISION_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "ZeroDivisionError"};

void set_zero_division_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    set_error(vm, SYMBOL_ZERO_DIVISION_ERROR, "division by zero");
}

void set_out_of_memory_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    vm->error = OBJECT_ERROR_OUT_OF_MEMORY;

    object_print(OBJECT_ERROR_OUT_OF_MEMORY->as_dict.value, stdout);
    printf("\n");
    allocator_print_statistics(&vm->allocator, stderr);
    traceback_print_from_stack(&vm->stack, stderr);
}

static auto const SYMBOL_STACK_OVERFLOW_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "StackOverflowError"};

void set_stack_overflow_error(VirtualMachine *vm) {
    guard_is_not_null(vm);

    set_error(vm, SYMBOL_STACK_OVERFLOW_ERROR, "stack capacity exceeded");
}

static auto const SYMBOL_BINDING_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "BindingError"};

static void set_binding_count_error(VirtualMachine *vm, size_t expected, bool is_variadic, size_t got) {
    guard_is_not_null(vm);

    auto const error_type = SYMBOL_BINDING_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    auto const message_ok = sb_try_printf(
            &sb, &error_code,
            "cannot bind values (expected %s%zu, got %zu)",
            (is_variadic ? "at least " : ""), expected, got
    );
    if (false == message_ok) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

static void set_binding_target_type_error(VirtualMachine *vm, Object_Type target_type) {
    guard_is_not_null(vm);

    auto const error_type = SYMBOL_BINDING_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    if (false == sb_try_printf(&sb, &error_code, "cannot bind to %s", object_type_str(target_type))) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}

void set_binding_error(VirtualMachine *vm, BindingError error) {
    guard_is_not_null(vm);

    switch (error.type) {
        case BINDING_INVALID_TARGET: {
            auto const target_error = error.as_target_error;
            switch (target_error.type) {
                case BINDING_INVALID_TARGET_TYPE: {
                    set_binding_target_type_error(vm, target_error.as_invalid_target.target_type);
                    return;
                }
                case BINDING_INVALID_VARIADIC_SYNTAX: {
                    set_variadic_syntax_error(vm);
                    return;
                }
            }

            break;
        }
        case BINDING_INVALID_VALUE: {
            auto const value_error = error.as_value_error;
            switch (value_error.type) {
                case BINDING_VALUES_COUNT_MISMATCH: {
                    set_binding_count_error(
                            vm,
                            value_error.as_count_mismatch.expected,
                            value_error.as_count_mismatch.is_variadic,
                            value_error.as_count_mismatch.got
                    );
                    return;
                }
                case BINDING_CANNOT_UNPACK_VALUE: {
                    set_type_error(vm, error.as_value_error.as_cannot_unpack.value_type, TYPE_LIST);
                    return;
                }
            }

            break;
        }
        case BINDING_ALLOCATION_FAILED: {
            set_out_of_memory_error(vm);
            return;
        }
    }

    guard_unreachable();
}

static auto const SYMBOL_KEY_ERROR = &(Object) {.type = TYPE_SYMBOL, .as_symbol = "KeyError"};

void set_key_error(VirtualMachine *vm, Object *key) {
    guard_is_not_null(vm);
    guard_is_not_null(key);

    auto const error_type = SYMBOL_KEY_ERROR;

    auto sb = (StringBuilder) {0};
    errno_t error_code;

    if (false == object_try_repr(key, &sb, &error_code)) {
        sb_free(&sb);
        system_error(vm, error_code, error_type->as_symbol);
        vm->error = error_type;
        return;
    }

    set_error(vm, error_type, sb.str);
    sb_free(&sb);
}
