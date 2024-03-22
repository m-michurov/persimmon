#include "functions.h"

#include "common/macros.h"

#include "runtime/object.h"

RuntimeObject *SumInts(RuntimeObjectsSlice args) {
    RuntimeInt acc = 0;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_INT != arg->Type) {
            return RuntimeObject_Undefined();
        }

        acc += arg->AsInt;
    }

    return RuntimeObject_NewInt(acc);
}

RuntimeObject *MulInts(RuntimeObjectsSlice args) {
    RuntimeInt acc = 1;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_INT != arg->Type) {
            return RuntimeObject_Undefined();
        }

        acc *= arg->AsInt;
    }

    return RuntimeObject_NewInt(acc);
}

RuntimeObject *ConcatStrings(RuntimeObjectsSlice args) {
    size_t totalLength = 0;
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        if (RUNTIME_TYPE_STRING != arg->Type) {
            return RuntimeObject_Undefined();
        }

        totalLength += strlen(arg->AsString);
    }

    auto buffer = (char *) calloc(totalLength + 1, sizeof(char));
    Vector_ForEach(argPtr, args) {
        auto const arg = *(argPtr);
        strcat(buffer, arg->AsString);
    }

    auto result = RuntimeObject_NewString(buffer);
    free(buffer);

    return result;
}

RuntimeObject *Subtract(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_NewInt(0);
    }
    RuntimeInt acc = args.Items[0]->AsInt;
    Vector_ForEach(argPtr, Vector_SliceFrom(RuntimeObjectsSlice, args, 1)) {
        if (RUNTIME_TYPE_INT != (*argPtr)->Type) {
            return RuntimeObject_Undefined();
        }

        acc -= (*argPtr)->AsInt;
    }

    return RuntimeObject_NewInt(acc);
}

RuntimeObject *Add(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    if (RUNTIME_TYPE_INT == args.Items[0]->Type) {
        return SumInts(args);
    }

    if (RUNTIME_TYPE_STRING == args.Items[0]->Type) {
        return ConcatStrings(args);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject *Mul(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    if (RUNTIME_TYPE_INT == args.Items[0]->Type) {
        return MulInts(args);
    }

    return RuntimeObject_Undefined();
}

RuntimeObject *Print(RuntimeObjectsSlice args) {
    if (Vector_Empty(args)) {
        return RuntimeObject_Undefined();
    }

    size_t i = 0;
    Vector_ForEach(argPtr, args) {
        RuntimeObject_Print(stdout, **argPtr);
        if (i + 1 < args.Size) {
            printf(" ");
        }
        i++;
    }

    printf("\n");

    return RuntimeObject_Undefined();
}

RuntimeObject *Equals(RuntimeObjectsSlice args) {
    auto equals = true;

    for (size_t i = 0; i + 1 < args.Size; i++) {
        equals = equals && RuntimeObject_Equals(args.Items[i], args.Items[i + 1]);
    }

    return
            equals
            ? RuntimeObject_NewInt(1)
            : RuntimeObject_NewInt(0);
}

RuntimeObject *NotEquals(RuntimeObjectsSlice args) {
    for (size_t i = 0; i + 1 < args.Size; i++) {
        if (false == RuntimeObject_Equals(args.Items[i], args.Items[i + 1])) {
            return RuntimeObject_NewInt(1);
        }
    }

    return RuntimeObject_NewInt(0);
}

void DefineBuiltinFunctions(Scope scope[static 1]) {
    Scope_Put(scope, "+", RuntimeObject_NewNativeFunction(Add));
    Scope_Put(scope, "-", RuntimeObject_NewNativeFunction(Subtract));
    Scope_Put(scope, "*", RuntimeObject_NewNativeFunction(Mul));
    Scope_Put(scope, "print", RuntimeObject_NewNativeFunction(Print));
    Scope_Put(scope, "==", RuntimeObject_NewNativeFunction(Equals));
    Scope_Put(scope, "!=", RuntimeObject_NewNativeFunction(NotEquals));
}
