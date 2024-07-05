#pragma once

#include "object.h"
#include "virtual_machine.h"

bool try_eval(
        VirtualMachine *vm,
        Object *env,
        Object *expr,
        Object **value
);
