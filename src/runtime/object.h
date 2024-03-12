#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "call_checked.h"
#include "collections/map.h"
#include "collections/vector.h"

typedef enum RuntimeType {
    RUNTIME_TYPE_UNDEFINED,
    RUNTIME_TYPE_INT,
    RUNTIME_TYPE_STRING,
    RUNTIME_TYPE_NATIVE_FUNCTION,
} RuntimeType;

char const *RuntimeType_Name(RuntimeType);

typedef struct RuntimeObject RuntimeObject;

typedef int64_t RuntimeInt;
typedef char const *RuntimeString;

typedef Slice_Of(RuntimeObject *) RuntimeObjectsSlice;

typedef RuntimeObject *(*RuntimeNativeFunction)(RuntimeObjectsSlice);

struct RuntimeObject {
    RuntimeType Type;
    size_t ReferencesCount;

    union {
        RuntimeInt AsInt;
        RuntimeString AsString;
        RuntimeNativeFunction AsNativeFunction;
    };
};

extern int64_t RuntimeObject_DanglingPointers;

RuntimeObject *RuntimeObject_Undefined();

RuntimeObject *RuntimeObject_NewInt(RuntimeInt value);

RuntimeObject *RuntimeObject_NewString(RuntimeString value);

RuntimeObject *RuntimeObject_NewNativeFunction(RuntimeNativeFunction functionPtr);

RuntimeObject * RuntimeObject_ReferenceCreated(RuntimeObject object[static 1]);

void RuntimeObject_ReferenceDeleted(RuntimeObject object[static 1]);

bool RuntimeObject_Equals(RuntimeObject const object[static 1], RuntimeObject const other[static 1] );

void RuntimeObject_Print(FILE file[static 1], RuntimeObject);

void RuntimeObject_Repr(FILE file[static 1], RuntimeObject);
