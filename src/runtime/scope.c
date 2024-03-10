#include "scope.h"

#include "common/macros.h"

static size_t StrHash(char const *s) {
    unsigned long hash = 5381;
    int c;

    while ('\0' != (c = (unsigned char) *s++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

static bool StrEquals(char const *s1, char const *s2) {
    return 0 == strcmp(s1, s2);
}

Scope Scope_Empty() {
    return (Scope) {
            .Parent = NULL,
            .Vars = {
                    .KeyEquals = StrEquals,
                    .Hash = StrHash
            }
    };
}

Scope Scope_WithParent(Scope parent[static 1]) {
    auto scope = Scope_Empty();
    scope.Parent = parent;

    return scope;
}

void Scope_Free(Scope scope[static 1]) {
    Map_ForEach(p, scope->Vars) {
        free((void *) p->Key);
        RuntimeObject_ReferenceDeleted(p->Value);
    }
    Map_Free(&scope->Vars);
    scope->Parent = NULL;
}

bool Scope_TryResolve(Scope scope, char const *name, RuntimeObject *object[static 1]) {
    for (auto currentScope = &scope; NULL != currentScope; currentScope = currentScope->Parent) {
        if (Map_TryGet(currentScope->Vars, name, object)) {
            return true;
        }
    }

    fprintf(stdout, "Unresolved identifier `%s`\n", name);
    return false;
}

void Scope_Put(Scope scope[static 1], char const *name, RuntimeObject object[static 1]) {
    auto oldValuePtr = Map_At(scope->Vars, name);

    if (NULL != oldValuePtr) {
        fprintf(stdout, "Redefinition of `%s`\n", name);
        return;
    }

    Map_Put(&scope->Vars, strdup(name), object);
    RuntimeObject_ReferenceCreated(object);
}

void ReplaceObjectReference(RuntimeObject *p1[1], RuntimeObject p2[1]) {
    if (*p1 == p2) { return; }

    RuntimeObject_ReferenceDeleted(*p1);
    *p1 = p2;
    RuntimeObject_ReferenceCreated(p2);
}

void Scope_Update(Scope scope[static 1], char const *name, RuntimeObject newValue[static 1]) {
    for (; NULL != scope; scope = scope->Parent) {
        auto oldValuePtr = Map_At(scope->Vars, name);
        if (NULL == oldValuePtr) {
            continue;
        }

        ReplaceObjectReference(oldValuePtr, newValue);
        return;
    }


    fprintf(stdout, "Unresolved identifier `%s`\n", name);
}
