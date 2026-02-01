#include "math/random.h"

#include "debug/assert.h"
#include "platform/time.h"

// ============================================================================
// Вспомогательные функции
// ============================================================================

static u64 rotl64(const u64 x, const int k)
{
    return (x << k) | (x >> (64 - k));
}

// ============================================================================
// SplitMix64 реализация
// ============================================================================

// Для инициализации других генераторов.
static u64 splitmix64_next_impl(u64* state)
{
    u64 result = (*state += 0x9e3779b97f4a7c15ULL);
    result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9ULL;
    result = (result ^ (result >> 27)) * 0x94d049bb133111ebULL;
    return result ^ (result >> 31);
}

static u64 splitmix64_next(math_random_generator* gen)
{
    return splitmix64_next_impl(&gen->state.splitmix64.state);
}

static void splitmix64_init(math_random_generator* gen, u64 seed)
{
    gen->state.splitmix64.state = seed ? seed : 1;
}

// ============================================================================
// WyRand реализация
// ============================================================================

static u64 wyrand_next(math_random_generator* gen)
{
    u64 result = (gen->state.wyrand.state += 0x9e3779b97f4a7c15UL);
    result = (result ^ (result >> 32)) * 0x9e3779b97f4a7c15ULL;
    result = (result ^ (result >> 32)) * 0x9e3779b97f4a7c15ULL;
    return result ^ (result >> 32);
}

static void wyrand_init(math_random_generator* gen, u64 seed)
{
    gen->state.wyrand.state = seed ? seed : 1;
}

// ============================================================================
// XOSHIRO256** реализация
// ============================================================================

static u64 xoshiro256_next(math_random_generator* gen)
{
    const u64 result = rotl64(gen->state.xoshiro.s[1] * 5, 7) * 9;
    const u64 t = gen->state.xoshiro.s[1] << 17;

    gen->state.xoshiro.s[2] ^= gen->state.xoshiro.s[0];
    gen->state.xoshiro.s[3] ^= gen->state.xoshiro.s[1];
    gen->state.xoshiro.s[1] ^= gen->state.xoshiro.s[2];
    gen->state.xoshiro.s[0] ^= gen->state.xoshiro.s[3];

    gen->state.xoshiro.s[2] ^= t;
    gen->state.xoshiro.s[3] = rotl64(gen->state.xoshiro.s[3], 45);

    return result;
}

static void xoshiro256_jump_impl(math_random_generator* gen, const u64* jump_table)
{
    u64 s0 = 0, s1 = 0, s2 = 0, s3 = 0;

    for(int i = 0; i < 4; i++)
    {
        for(int b = 0; b < 64; b++)
        {
            if(jump_table[i] & (1ULL << b))
            {
                s0 ^= gen->state.xoshiro.s[0];
                s1 ^= gen->state.xoshiro.s[1];
                s2 ^= gen->state.xoshiro.s[2];
                s3 ^= gen->state.xoshiro.s[3];
            }
            xoshiro256_next(gen);
        }
    }

    gen->state.xoshiro.s[0] = s0;
    gen->state.xoshiro.s[1] = s1;
    gen->state.xoshiro.s[2] = s2;
    gen->state.xoshiro.s[3] = s3;
}

static void xoshiro256_init(math_random_generator* gen, u64 seed)
{
    u64 z = seed;

    for(int i = 0; i < 4; i++)
    {
        z += 0x9e3779b97f4a7c15ULL;
        u64 t = z;
        t = (t ^ (t >> 30)) * 0xbf58476d1ce4e5b9ULL;
        t = (t ^ (t >> 27)) * 0x94d049bb133111ebULL;
        gen->state.xoshiro.s[i] = t ^ (t >> 31);
    }
}

// ============================================================================
// PCG реализация
// ============================================================================

static u32 pcg_next(math_random_generator* gen)
{
    const u64 oldstate = gen->state.pcg.state;
    gen->state.pcg.state = oldstate * 6364136223846793005ULL + gen->state.pcg.inc;

    const u32 xorshifted = (u32)(((oldstate >> 18) ^ oldstate) >> 27);
    const u32 rot = (u32)(oldstate >> 59);

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static void pcg_init(math_random_generator* gen, u64 seed)
{
    gen->state.pcg.state = 0;
    gen->state.pcg.inc = (seed << 1) | 1;

    pcg_next(gen);
    gen->state.pcg.state += seed;
    pcg_next(gen);
}

// ============================================================================
// SFC64 реализация
// ============================================================================

static u64 sfc64_next(math_random_generator* gen)
{
    u64 tmp = gen->state.sfc64.a + gen->state.sfc64.b + gen->state.sfc64.w++;
    gen->state.sfc64.a = gen->state.sfc64.b ^ (gen->state.sfc64.b >> 11);
    gen->state.sfc64.b = gen->state.sfc64.c + (gen->state.sfc64.c << 3);
    gen->state.sfc64.c = ((gen->state.sfc64.c << 24) | (gen->state.sfc64.c >> 40)) + tmp;
    return tmp;
}

static void sfc64_init(math_random_generator* gen, u64 seed)
{
    u64 temp_state = seed;
    gen->state.sfc64.a = splitmix64_next_impl(&temp_state);
    gen->state.sfc64.b = splitmix64_next_impl(&temp_state);
    gen->state.sfc64.c = splitmix64_next_impl(&temp_state);
    gen->state.sfc64.w = 1;
}

// ============================================================================
// Philox реализация (упрощенная 2x64 версия)
// ============================================================================

static u64 philox_next(math_random_generator* gen)
{
    u64 hi = gen->state.philox.counter[1];
    u64 lo = gen->state.philox.counter[0];
    u64 key_hi = gen->state.philox.key[1];
    u64 key_lo = gen->state.philox.key[0];

    // 10 раундов.
    for(int i = 0; i < 10; i++)
    {
        // Умножение с переносом (64x64 -> 128).
        u64 mul_lo, mul_hi;
        {
            u64 x  = lo;
            u64 y  = 0xD2B74407B1CE6E93ULL; // Константа Philox.
            u64 x0 = x & 0xFFFFFFFFULL;
            u64 x1 = x >> 32;
            u64 y0 = y & 0xFFFFFFFFULL;
            u64 y1 = y >> 32;

            u64 p0 = x0 * y0;
            u64 p1 = x0 * y1;
            u64 p2 = x1 * y0;
            u64 p3 = x1 * y1;

            u64 carry = (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);
            mul_lo = (carry << 32) | (p0 & 0xFFFFFFFFULL);
            mul_hi = (p1 >> 32) + (p2 >> 32) + (p3) + (carry >> 32);
        }

        // XOR с ключом и раундовой константой.
        lo = mul_hi ^ key_lo ^ i;
        hi = mul_lo ^ key_hi;

        // Обновление ключа.
        key_lo += 0x9E3779B97F4A7C15ULL;
        key_hi += 0xBB67AE8584CAA73BULL;
    }

    // Инкремент счетчика.
    gen->state.philox.counter[0]++;

    if(gen->state.philox.counter[0] == 0)
    {
        gen->state.philox.counter[1]++;
    }

    return hi; // Возвращаем старшие 64 бита.
}

static void philox_init(math_random_generator* gen, u64 seed)
{
    gen->state.philox.counter[0] = 0;
    gen->state.philox.counter[1] = 0;

    u64 temp_state = seed;
    gen->state.philox.key[0] = splitmix64_next_impl(&temp_state);
    gen->state.philox.key[1] = splitmix64_next_impl(&temp_state);
}

// ============================================================================
// Общедоступные функции
// ============================================================================

void math_random_generator_init(math_random_generator* gen, math_random_generator_type type, u64 seed)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(type < MATH_RANDOM_GENERATOR_TYPE_COUNT, "Type must be less than MATH_RANDOM_GENERATOR_TYPE_COUNT.");

    // Задание типа генератора.
    gen->type = type;

    // Если seed = 0, используется системное время.
    if(seed == 0)
    {
        seed = platform_time_seed();
    }

    switch(type)
    {
        case MATH_RANDOM_GENERATOR_TYPE_XOSHIRO256:
            xoshiro256_init(gen, seed);
            break;
        case MATH_RANDOM_GENERATOR_TYPE_PCG:
            pcg_init(gen, seed);
            break;
        case MATH_RANDOM_GENERATOR_TYPE_SPLITMIX64:
            splitmix64_init(gen, seed);
            break;
        case MATH_RANDOM_GENERATOR_TYPE_WYRAND:
            wyrand_init(gen, seed);
            break;
        case MATH_RANDOM_GENERATOR_TYPE_SFC64:
            sfc64_init(gen, seed);
            break;
        case MATH_RANDOM_GENERATOR_TYPE_PHILOX:
            philox_init(gen, seed);
            break;
        default:
            return;
    }
}

void math_random_generator_jump(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    if(gen->type != MATH_RANDOM_GENERATOR_TYPE_XOSHIRO256)
    {
        return;
    }

    static const u64 JUMP_TABLE[] = {
        0x180ec6d33cfd0aba, 0xd5a61266f0c9392c,
        0xa9582618e03fc9aa, 0x39abdc4529b1661c
    };

    xoshiro256_jump_impl(gen, JUMP_TABLE);
}

void math_random_generator_long_jump(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    if(gen->type != MATH_RANDOM_GENERATOR_TYPE_XOSHIRO256)
    {
        return;
    }

    static const u64 LONG_JUMP_TABLE[] = {
        0x76e15d3efefdcbbf, 0xc5004e441c522fb3,
        0x77710069854ee241, 0x39109bb02acbe635
    };

    xoshiro256_jump_impl(gen, LONG_JUMP_TABLE);
}

u32 math_random_u32(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    switch(gen->type)
    {
        case MATH_RANDOM_GENERATOR_TYPE_XOSHIRO256:
            return (u32)(xoshiro256_next(gen) >> 32);
        case MATH_RANDOM_GENERATOR_TYPE_PCG:
            return pcg_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_SPLITMIX64:
            return (u32)(splitmix64_next(gen) >> 32);
        case MATH_RANDOM_GENERATOR_TYPE_WYRAND:
            return (u32)(wyrand_next(gen) >> 32);
        case MATH_RANDOM_GENERATOR_TYPE_SFC64:
            return (u32)(sfc64_next(gen) >> 32);
        case MATH_RANDOM_GENERATOR_TYPE_PHILOX:
            return (u32)(philox_next(gen) >> 32);
        default:
            return 0;
    }
}

u32 math_random_u32_range(math_random_generator* gen, u32 min, u32 max)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(min < max, "Min value must be less than max value.");

    const u32 range = max - min;
    const u32 bias_limit = (U32_MAX - range + 1) % range; // Деление с отбрасыванием для равномерного распределения.

    u32 result;
    while((result = math_random_u32(gen)) < bias_limit);

    return min + (result % range);
}

i32 math_random_i32(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    return (i32)math_random_u32(gen);
}

i32 math_random_i32_range(math_random_generator* gen, i32 min, i32 max)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(min < max, "Min value must be less than max value.");

    const u32 range = (u32)(max - min);
    const u32 bias_limit = (U32_MAX - range + 1) % range;

    u32 result;
    while((result = math_random_u32(gen)) < bias_limit);

    return min + (i32)(result % range);
}

u64 math_random_u64(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    switch(gen->type)
    {
        case MATH_RANDOM_GENERATOR_TYPE_XOSHIRO256:
            return xoshiro256_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_PCG:
            return ((u64)pcg_next(gen) << 32) | (u64)pcg_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_SPLITMIX64:
            return splitmix64_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_WYRAND:
            return wyrand_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_SFC64:
            return sfc64_next(gen);
        case MATH_RANDOM_GENERATOR_TYPE_PHILOX:
            return philox_next(gen);
        default:
            return 0;
    }
}

u64 math_random_u64_range(math_random_generator* gen, u64 min, u64 max)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(min < max, "Min value must be less than max value.");

    const u64 range = max - min;
    const u64 bias_limit = (U64_MAX - range + 1) % range;

    u64 result;
    while((result = math_random_u64(gen)) < bias_limit);

    return min + (result % range);
}

i64 math_random_i64(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    return (i64)math_random_u64(gen);
}

i64 math_random_i64_range(math_random_generator* gen, i64 min, i64 max)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(min < max, "Min value must be less than max value.");

    const u64 range = (u64)(max - min);
    const u64 bias_limit = (U64_MAX - range + 1) % range;

    u64 result;
    while((result = math_random_u64(gen)) < bias_limit);

    return min + (i64)(result % range);
}

f32 math_random_f32(math_random_generator* gen)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");

    // TODO: Эффективная?
    // const f32 scale = 1.0f / 4294967296.0f;
    // return (f32)math_random_u32(gen) * scale;

    const u32 mantissa_bits = math_random_u32(gen) >> 9;  // Получение старших 23 бита, положительный знак (0).
    const u32 float_bits    = 0x3F800000 | mantissa_bits; // Установка экспоненты (получаем числа в диапазоне [1, 2)).

    union { f32 f; u32 m; } conv;
    conv.m = float_bits;

    return conv.f - 1.0f;                                 // Получаем [0, 1).
}

void math_random_f32_bulk(math_random_generator* gen, u32 count, f32* out)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(out != nullptr, "Output array must be non-null.");
    ASSERT(count > 0, "Number must be greater than zero.");

    for(u32 i = 0; i < count; i++)
    {
        out[i] = math_random_f32(gen);
    }
}

f32 math_random_f32_range(math_random_generator* gen, f32 min, f32 max)
{
    ASSERT(gen != nullptr, "Pointer must be non-null.");
    ASSERT(min < max, "Min value must be less than max value.");

    const f32 t = math_random_f32(gen);
    return min + t * (max - min);
}
