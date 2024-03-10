#pragma once

#include "ast/ast.h"
#include "runtime/object.h"
#include "runtime/scope.h"

RuntimeObject *Evaluate(Scope scope[static 1], AstNode);
