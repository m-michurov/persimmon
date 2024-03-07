#include "object.h"

#include <inttypes.h>

#include "common/macros.h"

char const *RuntimeType_Name(RuntimeType type) {
    switch (type) {
        case RUNTIME_TYPE_UNDEFINED:
            return "RUNTIME_TYPE_UNDEFINED";
        case RUNTIME_TYPE_INT:
            return "RUNTIME_TYPE_INT";
        case RUNTIME_TYPE_STRING:
            return "RUNTIME_TYPE_STRING";
        case RUNTIME_TYPE_NATIVE_FUNCTION:
            return "RUNTIME_TYPE_NATIVE_FUNCTION";
        default:
            Unreachable("%d", type);
    }
}

void RuntimeObject_Free(RuntimeObject object[static 1]) {
    switch (object->Type) {
        case RUNTIME_TYPE_INT:
        case RUNTIME_TYPE_NATIVE_FUNCTION:
        case RUNTIME_TYPE_UNDEFINED:
            return;
        case RUNTIME_TYPE_STRING: {
            free((void *) object->AsString);
            object->AsString = NULL;
            return;
        }
    }
    Unreachable("%d", object->Type);
}

void RuntimeObject_Print(FILE file[static 1], RuntimeObject object) {
    switch (object.Type) {
        case RUNTIME_TYPE_INT: {
            fprintf(file, "%" PRId64, object.AsInt);
            return;
        }
        case RUNTIME_TYPE_STRING: {
            fprintf(file, "%s", object.AsString);
            return;
        }
        case RUNTIME_TYPE_NATIVE_FUNCTION: {
            fprintf(file, "<NativeFunction at %p>", object.AsNativeFunction);
            return;
        }
        case RUNTIME_TYPE_UNDEFINED: {
            fprintf(file, "<undefined>");
            return;
        }
    }
    Unreachable("%d", object.Type);
}

void RuntimeObject_Repr(FILE file[static 1], RuntimeObject object) {
    fprintf(file, "(RuntimeObject) {.Type=%s", RuntimeType_Name(object.Type));
    switch (object.Type) {
        case RUNTIME_TYPE_INT: {
            fprintf(file, ", .AsInt=%" PRId64 "}", object.AsInt);
            return;
        }
        case RUNTIME_TYPE_STRING: {
            fprintf(file, ", .AsString=\"%s\"}", object.AsString);
            return;
        }
        case RUNTIME_TYPE_NATIVE_FUNCTION: {
            fprintf(
                    file, ", .AsNativeFunction=<NativeFunction at %p>}",
                    object.AsNativeFunction
            );
            return;
        }
        case RUNTIME_TYPE_UNDEFINED: {
            fprintf(file, "}");
            return;
        }
    }
    Unreachable("%d", object.Type);
}
