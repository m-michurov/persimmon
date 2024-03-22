#pragma once

#include "ast/ast.h"
#include "runtime/object.h"
#include "runtime/scope.h"

typedef struct TemporaryReferences {
    Vector_Of(RuntimeObject *) Objects;
} TemporaryReferences;

#define TemporaryReferences_Empty() ((TemporaryReferences) {0})

RuntimeObject *TemporaryReferences_Add(TemporaryReferences references[static 1], RuntimeObject *);

void TemporaryReferences_Free(TemporaryReferences references[static 1]);

RuntimeObject *Apply(RuntimeObject *fn, RuntimeObjectsSlice args);

RuntimeObject *Evaluate(Scope scope[static 1], AstNode);
