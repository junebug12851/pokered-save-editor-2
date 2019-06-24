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
using uvar8s = std::uint_least8_t;
using uvar16s = std::uint_least16_t;
using uvar32s = std::uint_least32_t;
using uvar64s = std::uint_least64_t;

// Smallest variables signed
using svar8s = std::int_least8_t;
using svar16s = std::int_least16_t;
using svar32s = std::int_least32_t;
using svar64s = std::int_least64_t;

// Fastest variables unsigned
using uvar8f = std::uint_fast8_t;
using uvar16f = std::uint_fast16_t;
using uvar32f = std::uint_fast32_t;
using uvar64f = std::uint_fast64_t;

// Fastest variables signed
using svar8f = std::int_fast8_t;
using svar16f = std::int_fast16_t;
using svar32f = std::int_fast32_t;
using svar64f = std::int_fast64_t;

// Exact variables unsigned
using uvar8e = std::uint8_t;
using uvar16e = std::uint16_t;
using uvar32e = std::uint32_t;
using uvar64e = std::uint64_t;

// Exact variables signed
using svar8e = std::int8_t;
using svar16e = std::int16_t;
using svar32e = std::int32_t;
using svar64e = std::int64_t;

// Smallest variables default signing (Unsigned)
using var8s = uvar8s;
using var16s = uvar16s;
using var32s = uvar32s;
using var64s = uvar64s;

// Smallest variable default signing of default size (8-bits)
using vars = var8s;

// Fastest variables default signing (Unsigned)
using var8f = uvar8f;
using var16f = uvar16f;
using var32f = uvar32f;
using var64f = uvar64f;

// Fastest variable default signing of default size (8-bits)
using varf = var8f;

// Exact variables default signing (Unsigned)
using var8e = uvar8e;
using var16e = uvar16e;
using var32e = uvar32e;
using var64e = uvar64e;

// Exact variable default signing of default size (8-bits)
using vare = var8e;

// Default ratio (Smallest) variables default signing
using var8 = var8s;
using var16 = var16s;
using var32 = var32s;
using var64 = var64s;

// Default ratio variables default signing of default
// size (8-bits)
using var = var8;

#endif // VARS_H
