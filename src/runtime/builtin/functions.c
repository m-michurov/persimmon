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

RuntimeObject *List_New(RuntimeObjectsSlice args) {
    auto result = RuntimeObject_Null();
    for (size_t i = 0; i < args.Size; i++) {
        result = RuntimeObject_NewList(args.Items[args.Size - i - 1], result);
    }

    return result;
}

RuntimeObject *List_Head(RuntimeObjectsSlice args) {
    if (1 != args.Size || RUNTIME_TYPE_LIST != args.Items[0]->Type) {
        return RuntimeObject_Undefined();
    }

    return args.Items[0]->AsList.Head;
}

RuntimeObject *List_Tail(RuntimeObjectsSlice args) {
    if (1 != args.Size || RUNTIME_TYPE_LIST != args.Items[0]->Type) {
        return RuntimeObject_Undefined();
    }

    return args.Items[0]->AsList.Tail;
}

RuntimeObject *List_Prepend(RuntimeObjectsSlice args) {
    if (2 != args.Size
        || (RUNTIME_TYPE_LIST != args.Items[1]->Type && RUNTIME_TYPE_NULL != args.Items[1]->Type)) {
        return RuntimeObject_Undefined();
    }

    return RuntimeObject_NewList(args.Items[0], args.Items[1]);
}

void DefineBuiltinFunctions(Scope scope[static 1]) {
    Scope_Put(scope, "+", RuntimeObject_NewNativeFunction(Add));
    Scope_Put(scope, "-", RuntimeObject_NewNativeFunction(Subtract));
    Scope_Put(scope, "*", RuntimeObject_NewNativeFunction(Mul));
    Scope_Put(scope, "print", RuntimeObject_NewNativeFunction(Print));
    Scope_Put(scope, "==", RuntimeObject_NewNativeFunction(Equals));
    Scope_Put(scope, "!=", RuntimeObject_NewNativeFunction(NotEquals));
    Scope_Put(scope, "list", RuntimeObject_NewNativeFunction(List_New));
    Scope_Put(scope, "head", RuntimeObject_NewNativeFunction(List_Head));
    Scope_Put(scope, "tail", RuntimeObject_NewNativeFunction(List_Tail));
    Scope_Put(scope, "prepend", RuntimeObject_NewNativeFunction(List_Prepend));
}
