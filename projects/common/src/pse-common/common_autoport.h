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
#include <QtCore/qglobal.h>

/**
 * @file common_autoport.h
 * @brief Shared-library import/export macro for the @c common library.
 *
 * Each sub-library (common, db, savefile) carries one of these "autoport"
 * headers. When the library is being @e built, @c COMMON_LIBRARY is defined and
 * its public symbols are marked @c Q_DECL_EXPORT; when another target @e consumes
 * the library, the same symbols resolve to @c Q_DECL_IMPORT. Classes opt in by
 * tagging their declaration with the @ref COMMON_AUTOPORT macro
 * (e.g. `class COMMON_AUTOPORT Utility ...`).
 */

/**
 * @def COMMON_AUTOPORT
 * @brief Expands to the correct dllexport/dllimport decoration for this library.
 */
#if defined(COMMON_LIBRARY)
#  define COMMON_AUTOPORT Q_DECL_EXPORT
#else
#  define COMMON_AUTOPORT Q_DECL_IMPORT
#endif
