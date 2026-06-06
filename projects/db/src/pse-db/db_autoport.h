/*
  * Copyright 2020 Twilight
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

#if defined(DB_LIBRARY)
#  define DB_AUTOPORT Q_DECL_EXPORT
#else
#  define DB_AUTOPORT Q_DECL_IMPORT
#endif


// ── Opaque pointer declarations ───────────────────────────────────────────────
// Qt 6 requires Q_PROPERTY pointer types to be fully defined OR declared here.
// All DB entry pointer types used in Q_PROPERTY across the library are
// listed once here so they can be shared without redefinition errors.
// The actual struct/class definitions live in their respective headers.

struct MapDBEntry;
struct MissableDBEntry;
struct SpriteDBEntry;
struct SpriteSetDBEntry;
struct MusicDBEntry;
struct TilesetDBEntry;
struct FlyDBEntry;
struct ScriptDBEntry;
struct PokemonDBEntry;
struct ItemDBEntry;
struct MoveDBEntry;
struct MapDBEntrySpriteItem;
struct PokemonDBEntryEvolution;
struct GameCornerDBEntry;
struct TrainerDBEntry;
struct MapDBEntryWarpIn;
struct MapDBEntrySprite;
class FontSearch;
class MapSearch;
class NamesPlayer;
class NamesPokemon;

Q_DECLARE_OPAQUE_POINTER(MapDBEntry*)
Q_DECLARE_OPAQUE_POINTER(MissableDBEntry*)
Q_DECLARE_OPAQUE_POINTER(SpriteDBEntry*)
Q_DECLARE_OPAQUE_POINTER(SpriteSetDBEntry*)
Q_DECLARE_OPAQUE_POINTER(MusicDBEntry*)
Q_DECLARE_OPAQUE_POINTER(TilesetDBEntry*)
Q_DECLARE_OPAQUE_POINTER(FlyDBEntry*)
Q_DECLARE_OPAQUE_POINTER(ScriptDBEntry*)
Q_DECLARE_OPAQUE_POINTER(PokemonDBEntry*)
Q_DECLARE_OPAQUE_POINTER(ItemDBEntry*)
Q_DECLARE_OPAQUE_POINTER(MoveDBEntry*)
Q_DECLARE_OPAQUE_POINTER(MapDBEntrySpriteItem*)
Q_DECLARE_OPAQUE_POINTER(PokemonDBEntryEvolution*)
Q_DECLARE_OPAQUE_POINTER(GameCornerDBEntry*)
Q_DECLARE_OPAQUE_POINTER(TrainerDBEntry*)
Q_DECLARE_OPAQUE_POINTER(MapDBEntryWarpIn*)
Q_DECLARE_OPAQUE_POINTER(MapDBEntrySprite*)
Q_DECLARE_OPAQUE_POINTER(FontSearch*)
Q_DECLARE_OPAQUE_POINTER(MapSearch*)
Q_DECLARE_OPAQUE_POINTER(NamesPlayer*)
Q_DECLARE_OPAQUE_POINTER(NamesPokemon*)
