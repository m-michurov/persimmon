#pragma once

#include "object/object.h"
#include "vm/virtual_machine.h"

[[nodiscard]]
bool try_define_constants(VirtualMachine *vm, Object *env);
