#pragma once

struct Object;
struct ObjectAllocator;

[[nodiscard]]
bool object_dict_try_put(
        struct ObjectAllocator *a,
        struct Object *dict,
        struct Object *key,
        struct Object *value,
        struct Object **result
);

[[nodiscard]]
bool object_dict_try_get(struct Object *dict, struct Object *key, struct Object **value);
