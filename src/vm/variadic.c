#include "variadic.h"

#include <strings.h>

bool is_ampersand(Object *obj) {
    return TYPE_SYMBOL == obj->type && 0 == strcmp("&", obj->as_symbol);
}
