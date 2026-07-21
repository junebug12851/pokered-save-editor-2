/*
  * Copyright 2020 Fairy Fox
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/
#pragma once
#include <cstdint>

/**
 * @file types.h
 * @brief Project-wide fixed-width integer aliases (`var8`, `var16`, ...).
 *
 * A save-file editor lives and dies by exact byte widths, so the whole project
 * spells out integer sizes explicitly rather than trusting `int`/`short`. These
 * aliases wrap the `<cstdint>` least/fast/exact families behind one short,
 * consistent naming scheme so call sites read as "an N-bit value" instead of a
 * platform-dependent built-in type.
 *
 * @par Naming scheme
 * @code
 * [us]var[#][sfe]
 * @endcode
 * - @b [us] -- @c u nsigned or @c s igned. Omitted => defaults to unsigned.
 * - @b [#]  -- minimum bit width needed: 8, 16, 32, or 64.
 * - @b [sfe] -- @c s mallest, @c f astest, or @c e xact minimum size.
 *   Omitted => defaults to smallest. @e exact means no more or fewer bits than
 *   requested and may not be supported on every platform.
 *
 * The blocks below build up from the fully-spelled-out forms to the everyday
 * shorthand (`var8`/`var16`/...) that the rest of the codebase actually uses.
 */

////////////////////////////////////////////////////////////////////////////////
/// Fully Detailed
////////////////////////////////////////////////////////////////////////////////

// Smallest variables unsigned
using uvar8s = std::uint_least8_t;    ///< Unsigned, at least 8-bit, smallest such type.
using uvar16s = std::uint_least16_t;  ///< Unsigned, at least 16-bit, smallest such type.
using uvar32s = std::uint_least32_t;  ///< Unsigned, at least 32-bit, smallest such type.
using uvar64s = std::uint_least64_t;  ///< Unsigned, at least 64-bit, smallest such type.

// Smallest variables signed
using svar8s = std::int_least8_t;     ///< Signed, at least 8-bit, smallest such type.
using svar16s = std::int_least16_t;   ///< Signed, at least 16-bit, smallest such type.
using svar32s = std::int_least32_t;   ///< Signed, at least 32-bit, smallest such type.
using svar64s = std::int_least64_t;   ///< Signed, at least 64-bit, smallest such type.

// Fastest variables unsigned
using uvar8f = std::uint_fast8_t;     ///< Unsigned, at least 8-bit, fastest such type.
using uvar16f = std::uint_fast16_t;   ///< Unsigned, at least 16-bit, fastest such type.
using uvar32f = std::uint_fast32_t;   ///< Unsigned, at least 32-bit, fastest such type.
using uvar64f = std::uint_fast64_t;   ///< Unsigned, at least 64-bit, fastest such type.

// Fastest variables signed
using svar8f = std::int_fast8_t;      ///< Signed, at least 8-bit, fastest such type.
using svar16f = std::int_fast16_t;    ///< Signed, at least 16-bit, fastest such type.
using svar32f = std::int_fast32_t;    ///< Signed, at least 32-bit, fastest such type.
using svar64f = std::int_fast64_t;    ///< Signed, at least 64-bit, fastest such type.

// Exact variables unsigned
using uvar8e = std::uint8_t;          ///< Unsigned, exactly 8-bit (platform must support it).
using uvar16e = std::uint16_t;        ///< Unsigned, exactly 16-bit (platform must support it).
using uvar32e = std::uint32_t;        ///< Unsigned, exactly 32-bit (platform must support it).
using uvar64e = std::uint64_t;        ///< Unsigned, exactly 64-bit (platform must support it).

// Exact variables signed
using svar8e = std::int8_t;           ///< Signed, exactly 8-bit (platform must support it).
using svar16e = std::int16_t;         ///< Signed, exactly 16-bit (platform must support it).
using svar32e = std::int32_t;         ///< Signed, exactly 32-bit (platform must support it).
using svar64e = std::int64_t;         ///< Signed, exactly 64-bit (platform must support it).

////////////////////////////////////////////////////////////////////////////////
/// Shorthand with default assumptions
////////////////////////////////////////////////////////////////////////////////

// Smallest variables default signing (Unsigned)
using var8s = uvar8s;                 ///< Smallest >=8-bit, default (unsigned) signing.
using var16s = uvar16s;               ///< Smallest >=16-bit, default (unsigned) signing.
using var32s = uvar32s;               ///< Smallest >=32-bit, default (unsigned) signing.
using var64s = uvar64s;               ///< Smallest >=64-bit, default (unsigned) signing.

// Fastest variables default signing (Unsigned)
using var8f = uvar8f;                 ///< Fastest >=8-bit, default (unsigned) signing.
using var16f = uvar16f;               ///< Fastest >=16-bit, default (unsigned) signing.
using var32f = uvar32f;               ///< Fastest >=32-bit, default (unsigned) signing.
using var64f = uvar64f;               ///< Fastest >=64-bit, default (unsigned) signing.

// Exact variables default signing (Unsigned)
using var8e = uvar8e;                 ///< Exactly 8-bit, default (unsigned) signing.
using var16e = uvar16e;               ///< Exactly 16-bit, default (unsigned) signing.
using var32e = uvar32e;               ///< Exactly 32-bit, default (unsigned) signing.
using var64e = uvar64e;               ///< Exactly 64-bit, default (unsigned) signing.

////////////////////////////////////////////////////////////////////////////////
/// Smaller Shorthand with most default assumptions
////////////////////////////////////////////////////////////////////////////////

using svar8 = svar8e;                 ///< Signed, exactly 8-bit (shorthand for svar8e).
using svar16 = svar16e;               ///< Signed, exactly 16-bit (shorthand for svar16e).
using svar32 = svar32e;               ///< Signed, exactly 32-bit (shorthand for svar32e).
using svar64 = svar64e;               ///< Signed, exactly 64-bit (shorthand for svar64e).

using uvar8 = uvar8e;                 ///< Unsigned, exactly 8-bit (shorthand for uvar8e).
using uvar16 = uvar16e;               ///< Unsigned, exactly 16-bit (shorthand for uvar16e).
using uvar32 = uvar32e;               ///< Unsigned, exactly 32-bit (shorthand for uvar32e).
using uvar64 = uvar64e;               ///< Unsigned, exactly 64-bit (shorthand for uvar64e).

// Default ratio (Smallest) variables default signing
// In a different project a small bug was determined with pointers
// When using "fastest" it often may choose "32-bit" whereby
// An 8-bit pointer becomes a 32-bit pointer mistakenly but labeled as an
// 8-bit pointer causing many potential bugs.
using var8 = var8e;                   ///< Everyday 8-bit alias. Exact (not "fastest") to dodge the pointer-width bug noted above.
using var16 = var16e;                 ///< Everyday 16-bit alias. Exact width to avoid the "fastest" widening bug.
using var32 = var32e;                 ///< Everyday 32-bit alias. Exact width to avoid the "fastest" widening bug.
using var64 = var64e;                 ///< Everyday 64-bit alias. Exact width to avoid the "fastest" widening bug.
