/*
  * Copyright 2020 June Hanabi
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
#ifndef AREA_H
#define AREA_H

#include "../expandedinterface.h"

class SaveFile;
class Player;
class AreaAudio;
class AreaLoadedSprites;
class AreaGeneral;
class AreaMap;
class AreaNPC;
class AreaPlayer;
class AreaPokemon;
class AreaPuzzle;
class AreaSign;
class AreaSprites;
class AreaTileset;
class AreaWarps;

class Area : public ExpandedInterface
{
public:
  Area(SaveFile* saveFile = nullptr);
  virtual ~Area();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();

  AreaAudio* audio = nullptr;
  AreaLoadedSprites* preloadedSprites = nullptr;
  AreaGeneral* general = nullptr;
  AreaMap* map = nullptr;
  AreaNPC* npc = nullptr;
  AreaPlayer* player = nullptr;
  AreaPokemon* pokemon = nullptr;
  AreaPuzzle* puzzle = nullptr;
  AreaSign* signs = nullptr;
  AreaSprites* sprites = nullptr;
  AreaTileset* tileset = nullptr;
  AreaWarps* warps = nullptr;
};

#endif // AREA_H
