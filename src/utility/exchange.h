#pragma once

#define exchange(OldValue, NewValue) ({auto _old = (OldValue); (OldValue) = (NewValue); _old; })
