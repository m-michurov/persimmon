#pragma once

#define sign(X) ((X) >= 0 ? +1 : -1)

#define min(A, B) ({ typeof(A) const _a = (A); typeof(A) const _b = (B); _a < _b ? _a : _b; })

#define max(A, B) ({ typeof(A) const _a = (A); typeof(A) const _b = (B); _a > _b ? _a : _b; })
