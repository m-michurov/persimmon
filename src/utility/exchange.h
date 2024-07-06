#pragma once

#define exchange(Dst, Src) ({auto _dst = (Dst); (Dst) = (Src); _dst; })
