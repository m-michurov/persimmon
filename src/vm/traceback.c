#include "traceback.h"

#include "utility/guards.h"
#include "object/list.h"
#include "object/repr.h"
#include "object/constructors.h"

#define PRINT_HEADER "Traceback (most recent call last):\n"
#define PRINT_FOOTER "Some calls may be missing due to tail call optimization.\n"
#define PRINT_INDENT "    "

bool traceback_try_get(ObjectAllocator *a, Stack s, Object **traceback) {
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
        object_repr(expr, file);
        fprintf(file, "\n");
    }
    fprintf(file, PRINT_FOOTER);
}

void traceback_print_from_stack(Stack s, FILE *file) {
    guard_is_not_null(file);
    guard_is_false(stack_is_empty(s));

    fprintf(file, PRINT_HEADER);
    stack_for_reversed(frame, s) {
        fprintf(file, PRINT_INDENT);
        object_repr(frame->expr, file);
        fprintf(file, "\n");
    }
    fprintf(file, PRINT_FOOTER);
}
