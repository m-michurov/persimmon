#pragma once

#include "call_checked.h"
#include "collections/map.h"

#include "runtime/object.h"

typedef struct Scope Scope;

struct Scope {
    Map_Of(char const *, RuntimeObject) Vars;

    Scope *Parent;
};

Scope Scope_Empty();

Scope Scope_WithParent(Scope parent[static 1]);

void Scope_Free(Scope scope[static 1]);

bool Scope_TryResolve(Scope, char const *, RuntimeObject object[static 1]);

void Scope_Put(Scope scope[static 1], char const *, RuntimeObject);

void Scope_Update(Scope scope[static 1], char const *, RuntimeObject);
