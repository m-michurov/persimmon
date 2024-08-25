#include "variadic.h"

#include <strings.h>

bool is_ampersand(Object *obj) {
    return TYPE_ATOM == obj->type && 0 == strcmp("&", obj->as_atom);
}
