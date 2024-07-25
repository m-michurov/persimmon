#pragma once

#include "object/object.h"
#include "virtual_machine.h"

[[nodiscard]]

bool try_eval(VirtualMachine *vm, Object *env, Object *expr);
