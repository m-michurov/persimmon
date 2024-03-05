#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "call_checked.h"
#include "collections/map.h"

typedef enum RuntimeType {
    RUNTIME_TYPE_UNDEFINED,
    RUNTIME_TYPE_INT,
    RUNTIME_TYPE_STRING,
    RUNTIME_TYPE_NATIVE_FUNCTION,
} RuntimeType;

typedef struct RuntimeObject RuntimeObject;

typedef int64_t RuntimeInt;
typedef char const *RuntimeString;

typedef void (*RuntimeNativeFunction)(size_t, RuntimeObject *, RuntimeObject *);

struct RuntimeObject {
    RuntimeType Type;
    union {
        RuntimeInt AsInt;
        RuntimeString AsString;
        RuntimeNativeFunction AsNativeFunction;
    };
};

#define RuntimeObject_Undefined()   ((RuntimeObject) {.Type=RUNTIME_TYPE_UNDEFINED})
#define RuntimeObject_Int(Value)    ((RuntimeObject) {.Type=RUNTIME_TYPE_INT, .AsInt=(Value)})
#define RuntimeObject_String(Value) ((RuntimeObject) {.Type=RUNTIME_TYPE_STRING, .AsString=strdup((Value))})
#define RuntimeObject_NativeFunction(Value) \
    ((RuntimeObject) {.Type=RUNTIME_TYPE_NATIVE_FUNCTION, .AsNativeFunction=(Value)})

void RuntimeObject_Free(RuntimeObject object[static 1]);

void RuntimeObject_Print(FILE file[static 1], RuntimeObject);
