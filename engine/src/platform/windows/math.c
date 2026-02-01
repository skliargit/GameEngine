#include "platform/math.h"

#ifdef PLATFORM_WINDOWS_FLAG

    #include <math.h>
    #include <stdlib.h>
    #include <windows.h>

    f32 platform_math_sin(f32 x)
    {
        return sinf(x);
    }

    f32 platform_math_asin(f32 x)
    {
        return asinf(x);
    }

    f32 platform_math_cos(f32 x)
    {
        return cosf(x);
    }

    f32 platform_math_acos(f32 x)
    {
        return acosf(x);
    }

    f32 platform_math_tan(f32 x)
    {
        return tanf(x);
    }

    f32 platform_math_atan(f32 x)
    {
        return atanf(x);
    }

    f32 platform_math_atan2(f32 y, f32 x)
    {
        return atan2f(y, x);
    }

    f32 platform_math_sqrt(f32 x)
    {
        return sqrtf(x);
    }

    f64 platform_math_abs(f64 x)
    {
        return fabs(x);
    }

    f32 platform_math_absf(f32 x)
    {
        return fabsf(x);
    }

    f32 platform_math_floor(f32 x)
    {
        return floorf(x);
    }

    f32 platform_math_ceil(f32 x)
    {
        return ceilf(x);
    }

    f32 platform_math_log2(f32 x)
    {
        return log2f(x);
    }

    f32 platform_math_pow(f32 x, f32 p)
    {
        return powf(x, p);
    }

#endif
