#include "scope.h"

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

bool Scope_TryResolve(Scope scope, char const *name, RuntimeObject object[static 1]) {
    if (Map_TryGet(scope.Vars, name, object)) {
        return true;
    }

    if (NULL == scope.Parent) {
        fprintf(stdout, "Unresolved identifier `%s`\n", name);
        return false;
    }

    return Scope_TryResolve(*scope.Parent, name, object);
}

void Scope_Put(Scope scope[static 1], char const *name, RuntimeObject object) {
    Map_Put(&scope->Vars, strdup(name), object);
}

void Scope_Update(Scope scope[static 1], char const *name, RuntimeObject object) {
    RuntimeObject existingValue;
    if (Map_TryGet(scope->Vars, name, &existingValue)) {
        Map_Put(&scope->Vars, name, object);
        return;
    }

    if (NULL == scope->Parent) {
        fprintf(stdout, "Unresolved identifier `%s`\n", name);
        return;
    }

    Scope_Update(scope->Parent, name, object);
}

void Scope_Free(Scope scope[static 1]) {
    Map_ForEach(p, scope->Vars) {
        free((void *) p->Key);
        RuntimeObject_Free(&p->Value);
    }
    Map_Free(&scope->Vars);
    scope->Parent = NULL;
}
