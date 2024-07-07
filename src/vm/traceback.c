#include "traceback.h"

#include "utility/guards.h"
#include "utility/strings.h"
#include "object/lists.h"
#include "object/constructors.h"
#include "object/accessors.h"

#define PRINT_HEADER "Traceback (most recent call last):\n"
#define PRINT_FOOTER "Some calls may be missing due to tail call optimization.\n"
#define PRINT_INDENT "    "

bool traceback_try_get(ObjectAllocator *a, Stack const *s, Object **traceback) {
    guard_is_not_null(s);
    guard_is_not_null(a);
    guard_is_not_null(traceback);
    guard_is_not_null(*traceback);

    *traceback = object_nil();
    stack_for_reversed(frame, s) {
        if (false == object_try_make_cons(a, frame->expr, *traceback, traceback)) {
            return false;
        }
    }

    object_list_reverse(traceback);
    return true;
}

void traceback_print(Object *traceback, FILE *file) {
    guard_is_not_null(traceback);
    guard_is_not_null(file);

    if (object_nil() == traceback) {
        return;
    }

    fprintf(file, PRINT_HEADER);
    object_list_for(expr, traceback) {
        fprintf(file, PRINT_INDENT);
        object_repr_print(expr, file);
        fprintf(file, "\n");
    }
    fprintf(file, PRINT_FOOTER);
}

[[maybe_unused]]

static void print_env(Object *env, FILE *file) {
    fprintf(file, "      with:\n");
    object_list_for(it, object_as_cons(env).first) {
        Object *obj_name, *obj_value;
        guard_is_true(object_list_try_unpack_2(&obj_name, &obj_value, it));
        fprintf(file, "        ");
        object_repr_print(obj_name, file);
        fprintf(file, ": ");
        if (TYPE_CONS == obj_value->type) {
            size_t i = 1;

            fprintf(file, "(");
            object_repr_print(obj_value->as_cons.first, file);
            object_list_for(elem, obj_value->as_cons.rest) {
                i++;
                fprintf(file, ", ");
                object_repr_print(elem, file);
                if (i > 5) { break; }
            }
            fprintf(file, ", ...)");
        } else if (TYPE_STRING == obj_value->type) {
            size_t i = 0;

            fprintf(file, "\"");
            string_for(c, obj_value->as_atom) {
                i++;
                fprintf(file, "%c", *c);
                if (i > 10) { break; }
            }

            fprintf(file, "\"");
        } else {
            object_repr_print(obj_value, file);
        }
        fprintf(file, "\n");
    }
}

void traceback_print_from_stack(Stack const *s, FILE *file) {
    guard_is_not_null(s);
    guard_is_not_null(file);

    if (stack_is_empty(s)) {
        return;
    }

    fprintf(file, PRINT_HEADER);
    auto is_top = true;
    stack_for_reversed(frame, s) {
        fprintf(file, PRINT_INDENT);
        object_repr_print(frame->expr, file);
        fprintf(file, "\n");
        if (is_top) {
            print_env(frame->env, file);
        }
        is_top = false;
    }
    fprintf(file, PRINT_FOOTER);
}
