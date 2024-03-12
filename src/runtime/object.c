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
    }

    Unreachable("%d", type);
}

int64_t RuntimeObject_DanglingPointers = 0;

static auto Undefined = (RuntimeObject) {.Type = RUNTIME_TYPE_UNDEFINED};

static bool IsSingletonType(RuntimeType type) {
    return RUNTIME_TYPE_UNDEFINED == type;
}

RuntimeObject *RuntimeObject_Undefined() { return &Undefined; }

RuntimeObject *RuntimeObject_NewInt(RuntimeInt value) {
    auto object = (RuntimeObject *) calloc(1, sizeof(RuntimeObject));
    *object = (RuntimeObject) {
            .Type = RUNTIME_TYPE_INT,
            .AsInt = value
    };

    RuntimeObject_DanglingPointers++;
    return object;
}

RuntimeObject *RuntimeObject_NewString(RuntimeString value) {
    auto object = (RuntimeObject *) calloc(1, sizeof(RuntimeObject));
    *object = (RuntimeObject) {
            .Type = RUNTIME_TYPE_STRING,
            .AsString = strdup(value)
    };

    RuntimeObject_DanglingPointers++;
    return object;
}

RuntimeObject *RuntimeObject_NewNativeFunction(RuntimeNativeFunction functionPtr) {
    auto object = (RuntimeObject *) calloc(1, sizeof(RuntimeObject));
    *object = (RuntimeObject) {
            .Type = RUNTIME_TYPE_NATIVE_FUNCTION,
            .AsNativeFunction = functionPtr
    };

    RuntimeObject_DanglingPointers++;
    return object;
}

RuntimeObject *RuntimeObject_ReferenceCreated(RuntimeObject object[static 1]) {
    if (IsSingletonType(object->Type)) { return object; }

    object->ReferencesCount++;
    return object;
}

static void FreeObject(RuntimeObject *object) {
    Assert(NULL != object);

    switch (object->Type) {
        case RUNTIME_TYPE_UNDEFINED:
            return;
        case RUNTIME_TYPE_INT:
        case RUNTIME_TYPE_NATIVE_FUNCTION: {
            RuntimeObject_DanglingPointers--;
            free(object);
            return;
        }
        case RUNTIME_TYPE_STRING: {
            RuntimeObject_DanglingPointers--;
            free((void *) object->AsString);
            object->AsString = NULL;

            free(object);
            return;
        }
    }

    Unreachable("%d", object->Type);
}

void RuntimeObject_ReferenceDeleted(RuntimeObject object[static 1]) {
    if (IsSingletonType(object->Type)) { return; }

    if (object->ReferencesCount > 1) {
        object->ReferencesCount--;
        return;
    }

    FreeObject(object);
}

bool RuntimeObject_Equals(RuntimeObject const object[static 1], RuntimeObject const other[static 1]) {
    if (object->Type != other->Type) { return false; }

    switch (object->Type) {
        case RUNTIME_TYPE_UNDEFINED:
            return true;
        case RUNTIME_TYPE_INT:
            return object->AsInt == other->AsInt;
        case RUNTIME_TYPE_STRING:
            return 0 == strcmp(object->AsString, other->AsString);
        case RUNTIME_TYPE_NATIVE_FUNCTION:
            return object->AsNativeFunction == other->AsNativeFunction;
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
