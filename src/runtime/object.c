#include "object.h"

#include <inttypes.h>

#include "common/macros.h"

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
        case RUNTIME_TYPE_UNDEFINED:{
            fprintf(file, "<undefined>");
            return;
        }
    }
    Unreachable("%d", object.Type);
}
