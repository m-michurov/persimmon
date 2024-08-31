#pragma once

#define MACROS__concat_(A, B) A ## B
#define concat_identifiers(A, B) MACROS__concat_(A, B)
