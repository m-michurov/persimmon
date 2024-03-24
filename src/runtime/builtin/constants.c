#include "constants.h"

#include "runtime/object.h"

void DefineBuiltinConstants(Scope scope[static 1]) {
    Scope_Put(scope, "null", RuntimeObject_Null());
}
