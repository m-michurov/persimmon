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
        case RUNTIME_TYPE_FUNCTION:
            return "RUNTIME_TYPE_FUNCTION";
        case RUNTIME_TYPE_NULL:
            return "RUNTIME_TYPE_NULL";
        case RUNTIME_TYPE_LIST:
            return "RUNTIME_TYPE_LIST";
    }

    Unreachable("%d", type);
}

int64_t RuntimeObject_DanglingPointers = 0;

static auto RuntimeUndefined = (RuntimeObject) {.Type = RUNTIME_TYPE_UNDEFINED};
static auto RuntimeNull = (RuntimeObject) {.Type = RUNTIME_TYPE_NULL};

static bool IsSingletonType(RuntimeType type) {
    if (RUNTIME_TYPE_UNDEFINED == type) {
        return true;
    }

    if (RUNTIME_TYPE_NULL == type) {
        return true;
    }

    return false;
}

RuntimeObject *RuntimeObject_Null() { return &RuntimeNull; }

RuntimeObject *RuntimeObject_Undefined() { return &RuntimeUndefined; }

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

RuntimeObject *RuntimeObject_NewFunction(ArgumentNames argumentNames, FunctionBody body, Scope capturedVariables) {
    auto object = (RuntimeObject *) calloc(1, sizeof(RuntimeObject));
    *object = (RuntimeObject) {
            .Type = RUNTIME_TYPE_FUNCTION,
            .AsFunction = (RuntimeFunction) {
                    .ArgumentNames = argumentNames,
                    .Body = body,
                    .CapturedVariables = capturedVariables
            }
    };

    RuntimeObject_DanglingPointers++;
    return object;
}

RuntimeObject *RuntimeObject_NewList(RuntimeObject *head, RuntimeObject *tail) {
    auto object = (RuntimeObject *) calloc(1, sizeof(RuntimeObject));
    *object = (RuntimeObject) {
            .Type = RUNTIME_TYPE_LIST,
            .AsList = (RuntimeList) {
                    .Head = head,
                    .Tail = tail
            }
    };

    RuntimeObject_ReferenceCreated(head);
    RuntimeObject_ReferenceCreated(tail);

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
        case RUNTIME_TYPE_NULL:
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
        case RUNTIME_TYPE_FUNCTION: {
            RuntimeObject_DanglingPointers--;
            auto fn = object->AsFunction;

            Vector_ForEach(argNamePtr, fn.ArgumentNames) {
                free((void *) *argNamePtr);
            }
            Vector_Free(&fn.ArgumentNames);

            Vector_ForEach(bodyNodePtr, fn.Body) {
                AstNode_Free(bodyNodePtr);
            }
            Vector_Free(&fn.Body);

            Scope_Free(&fn.CapturedVariables);

            return;
        }
        case RUNTIME_TYPE_LIST: {
            RuntimeObject_DanglingPointers--;

            auto list = object->AsList;
            RuntimeObject_ReferenceDeleted(list.Head);
            RuntimeObject_ReferenceDeleted(list.Tail);

            list.Head = NULL;
            list.Tail = NULL;

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
        case RUNTIME_TYPE_NULL:
        case RUNTIME_TYPE_UNDEFINED:
            return true;
        case RUNTIME_TYPE_INT:
            return object->AsInt == other->AsInt;
        case RUNTIME_TYPE_STRING:
            return 0 == strcmp(object->AsString, other->AsString);
        case RUNTIME_TYPE_NATIVE_FUNCTION:
            return object->AsNativeFunction == other->AsNativeFunction;
        case RUNTIME_TYPE_FUNCTION:
            return object == other;
        case RUNTIME_TYPE_LIST:
            return RuntimeObject_Equals(object->AsList.Head, other->AsList.Head)
                   && RuntimeObject_Equals(object->AsList.Tail, other->AsList.Tail);
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
        case RUNTIME_TYPE_FUNCTION: {
            fprintf(file, "<Function>");
            return;
        }
        case RUNTIME_TYPE_NULL: {
            fprintf(file, "<null>");
            return;
        }
        case RUNTIME_TYPE_LIST: {
            auto list = object.AsList;
            fprintf(file, "(");
            while (true) {
                RuntimeObject_Print(file, *list.Head);

                if (RUNTIME_TYPE_LIST != list.Tail->Type) {
                    if (RUNTIME_TYPE_NULL != list.Tail->Type) {
                        RuntimeObject_Print(file, *list.Tail);
                    }
                    break;
                }

                fprintf(file, " ");
                list = list.Tail->AsList;
            }
            fprintf(file, ")");
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
        case RUNTIME_TYPE_NULL:
        case RUNTIME_TYPE_UNDEFINED: {
            fprintf(file, "}");
            return;
        }
        case RUNTIME_TYPE_FUNCTION: {
            fprintf(file, ", .AsFunction=<Function>}");
            return;
        }
        case RUNTIME_TYPE_LIST: {
            fprintf(file, ", .AsList={.Head=");
            RuntimeObject_Repr(file, *object.AsList.Head);
            fprintf(file, ", .Tail=");
            RuntimeObject_Repr(file, *object.AsList.Tail);
            fprintf(file, "}}");
            return;
        }
    }
    Unreachable("%d", object.Type);
}
