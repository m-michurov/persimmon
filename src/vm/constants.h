#pragma once

#include "object/object.h"
#include "vm/virtual_machine.h"

bool try_define_constants(VirtualMachine *vm, Object *env);
