#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "call_checked.h"
#include "collections/map.h"
#include "collections/vector.h"

#include "ast/ast.h"
#include "runtime/scope.h"

typedef enum RuntimeType {
    RUNTIME_TYPE_NULL,
    RUNTIME_TYPE_UNDEFINED,
    RUNTIME_TYPE_INT,
    RUNTIME_TYPE_STRING,
    RUNTIME_TYPE_NATIVE_FUNCTION,
    RUNTIME_TYPE_FUNCTION,
    RUNTIME_TYPE_LIST,
} RuntimeType;

char const *RuntimeType_Name(RuntimeType);

typedef struct RuntimeObject RuntimeObject;

typedef int64_t RuntimeInt;
typedef char const *RuntimeString;

typedef VectorSlice_Of(RuntimeObject *) RuntimeObjectsSlice;

typedef RuntimeObject *(*RuntimeNativeFunction)(RuntimeObjectsSlice);

typedef Vector_Of(char const *) ArgumentNames;
typedef Vector_Of(AstNode) FunctionBody;

typedef struct RuntimeFunction {
    ArgumentNames ArgumentNames;
    FunctionBody Body;
    Scope CapturedVariables;
} RuntimeFunction;

typedef struct RuntimeList {
    RuntimeObject *Head;
    RuntimeObject *Tail;
} RuntimeList;

struct RuntimeObject {
    RuntimeType Type;
    size_t ReferencesCount;

    union {
        RuntimeInt AsInt;
        RuntimeString AsString;
        RuntimeNativeFunction AsNativeFunction;
        RuntimeFunction AsFunction;
        RuntimeList AsList;
    };
};

extern int64_t RuntimeObject_DanglingPointers;

RuntimeObject *RuntimeObject_Null();

RuntimeObject *RuntimeObject_Undefined();

RuntimeObject *RuntimeObject_NewInt(RuntimeInt value);

RuntimeObject *RuntimeObject_NewString(RuntimeString value);

RuntimeObject *RuntimeObject_NewNativeFunction(RuntimeNativeFunction functionPtr);

RuntimeObject *RuntimeObject_NewFunction(ArgumentNames, FunctionBody, Scope);

RuntimeObject *RuntimeObject_NewList(RuntimeObject *head, RuntimeObject *tail);

RuntimeObject *RuntimeObject_ReferenceCreated(RuntimeObject object[static 1]);

void RuntimeObject_ReferenceDeleted(RuntimeObject object[static 1]);

bool RuntimeObject_Equals(RuntimeObject const object[static 1], RuntimeObject const other[static 1]);

void RuntimeObject_Print(FILE file[static 1], RuntimeObject);

void RuntimeObject_Repr(FILE file[static 1], RuntimeObject);
