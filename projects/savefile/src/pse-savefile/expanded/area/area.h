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

#include <QObject>
#include "../../savefile_autoport.h"

class SaveFile;
class Player;
class AreaAudio;
class AreaLoadedSprites;
class AreaGeneral;
class AreaMap;
class AreaNPC;
class AreaPlayer;
class AreaPokemon;
class AreaSign;
class AreaSprites;
class AreaTileset;
class AreaWarps;

class MapDBEntry;

class SAVEFILE_AUTOPORT Area : public QObject
{
  Q_OBJECT

  Q_PROPERTY(AreaAudio* audio MEMBER audio NOTIFY audioChanged)
  Q_PROPERTY(AreaLoadedSprites* preloadedSprites MEMBER preloadedSprites NOTIFY preloadedSpritesChanged)
  Q_PROPERTY(AreaGeneral* general MEMBER general NOTIFY generalChanged)
  Q_PROPERTY(AreaMap* map MEMBER map NOTIFY mapChanged)
  Q_PROPERTY(AreaNPC* npc MEMBER npc NOTIFY npcChanged)
  Q_PROPERTY(AreaPlayer* player MEMBER player NOTIFY playerChanged)
  Q_PROPERTY(AreaPokemon* pokemon MEMBER pokemon NOTIFY pokemonChanged)
  Q_PROPERTY(AreaSign* signs MEMBER signs NOTIFY signsChanged)
  Q_PROPERTY(AreaSprites* sprites MEMBER sprites NOTIFY spritesChanged)
  Q_PROPERTY(AreaTileset* tileset MEMBER tileset NOTIFY tilesetChanged)
  Q_PROPERTY(AreaWarps* warps MEMBER warps NOTIFY warpsChanged)

public:
  Area(SaveFile* saveFile = nullptr);
  virtual ~Area();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

signals:
  // There's actually no need for these, Q_PROPERTY requires them
  void audioChanged();
  void preloadedSpritesChanged();
  void generalChanged();
  void mapChanged();
  void npcChanged();
  void playerChanged();
  void pokemonChanged();
  void signsChanged();
  void spritesChanged();
  void tilesetChanged();
  void warpsChanged();

public slots:
  void reset();
  void randomize();
  void setTo(MapDBEntry* map);

public:
  AreaAudio* audio = nullptr;
  AreaLoadedSprites* preloadedSprites = nullptr;
  AreaGeneral* general = nullptr;
  AreaMap* map = nullptr;
  AreaNPC* npc = nullptr;
  AreaPlayer* player = nullptr;
  AreaPokemon* pokemon = nullptr;
  AreaSign* signs = nullptr;
  AreaSprites* sprites = nullptr;
  AreaTileset* tileset = nullptr;
  AreaWarps* warps = nullptr;
};

#endif // AREA_H
