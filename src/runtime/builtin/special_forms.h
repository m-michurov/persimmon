#pragma once

#include "runtime/object.h"
#include "runtime/scope.h"
#include "runtime/eval.h"

typedef RuntimeObject *(*RuntimeSpecialForm)(Scope scope[static 1], AstNode);

bool SpecialForm_TryGet(char const *name, RuntimeSpecialForm specialForm[static 1]);
