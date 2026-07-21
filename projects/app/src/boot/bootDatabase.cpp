/*
  * Copyright 2019 Fairy Fox
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

/**
 * @file bootDatabase.cpp
 * @brief Brings up the entire game database -- just `(void)DB::inst();`, which
 *        constructs, loads, indexes, and deep-links every DB in order.
 */

#include <pse-db/db.h>

// Bootstrap the entire game database.
// DB::inst() handles all sub-database construction, loading, indexing,
// and deep-linking internally in the correct dependency order.
extern void bootDatabase()
{
  (void)DB::inst();
}

