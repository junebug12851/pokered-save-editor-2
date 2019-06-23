#ifndef VARS_H
#define VARS_H

#include <cstdint>

/**
 * Variable sizing type
 * [us]var[#][sfe]
 * [us] = Unsigned or Signed (Left off it defaults to unsigned)
 * [#] = Minimum size needed 8,16,32, or 64
 * [sfe] = smallest, fastest, or exact minimum size (Left off it defaults to smallest)
 *         exact meaning no more or less than minimum size and may not be supported.
 */

// Smallest variables unsigned
typedef std::uint_least8_t uvar8s;
typedef std::uint_least16_t uvar16s;
typedef std::uint_least32_t uvar32s;
typedef std::uint_least64_t uvar64s;

// Smallest variables signed
typedef std::int_least8_t svar8s;
typedef std::int_least16_t svar16s;
typedef std::int_least32_t svar32s;
typedef std::int_least64_t svar64s;

// Fastest variables unsigned
typedef std::uint_fast8_t uvar8f;
typedef std::uint_fast16_t uvar16f;
typedef std::uint_fast32_t uvar32f;
typedef std::uint_fast64_t uvar64f;

// Fastest variables signed
typedef std::int_fast8_t svar8f;
typedef std::int_fast16_t svar16f;
typedef std::int_fast32_t svar32f;
typedef std::int_fast64_t svar64f;

// Exact variables unsigned
typedef std::uint8_t uvar8e;
typedef std::uint16_t uvar16e;
typedef std::uint32_t uvar32e;
typedef std::uint64_t uvar64e;

// Exact variables signed
typedef std::int8_t svar8e;
typedef std::int16_t svar16e;
typedef std::int32_t svar32e;
typedef std::int64_t svar64e;

// Smallest variables default signing (Unsigned)
typedef uvar8s var8s;
typedef uvar16s var16s;
typedef uvar32s var32s;
typedef uvar64s var64s;

// Fastest variables default signing (Unsigned)
typedef uvar8f var8f;
typedef uvar16f var16f;
typedef uvar32f var32f;
typedef uvar64f var64f;

// Exact variables default signing (Unsigned)
typedef uvar8e var8e;
typedef uvar16e var16e;
typedef uvar32e var32e;
typedef uvar64e var64e;

// Default ratio (Smallest) variables default signing (Unsigned)
typedef var8s var8;
typedef var16s var16;
typedef var32s var32;
typedef var64s var64;

#endif // VARS_H
