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
#ifndef POKEMONPARTY_H
#define POKEMONPARTY_H

#include "./pokemonbox.h"
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

class SAVEFILE_AUTOPORT PokemonParty : public PokemonBox
{
  Q_OBJECT

  Q_PROPERTY(int maxHP MEMBER maxHP NOTIFY maxHPChanged)
  Q_PROPERTY(int attack MEMBER attack NOTIFY attackChanged)
  Q_PROPERTY(int defense MEMBER defense NOTIFY defenseChanged)
  Q_PROPERTY(int speed MEMBER speed NOTIFY speedChanged)
  Q_PROPERTY(int special MEMBER special NOTIFY specialChanged)

public:
  PokemonParty(SaveFile* saveFile = nullptr,
               var16 offset = 0,
               var16 nicknameStartOffset = 0,
               var16 otNameStartOffset = 0,
               var8 index = 0);

  virtual ~PokemonParty();

  virtual SaveFileIterator* load(SaveFile* saveFile = nullptr,
                         var16 offset = 0,
                         var16 nicknameStartOffset = 0,
                         var16 otNameStartOffset = 0,
                         var8 index = 0,
                         var8 recordSize = 0x2C) override;

  virtual SaveFileIterator* save(SaveFile* saveFile,
                         var16 offset,
                         svar32 speciesStartOffset,
                         var16 nicknameStartOffset,
                         var16 otNameStartOffset,
                         var8 index,
                         var8 recordSize = 0x2C) override;

  virtual void copyFrom(PokemonBox* pkmn) override;

  static PokemonBox* convertToBox(PokemonParty* data);
  static PokemonParty* convertToParty(PokemonBox* data);
  PokemonBox* toBoxData();

signals:
  void maxHPChanged();
  void attackChanged();
  void defenseChanged();
  void speedChanged();
  void specialChanged();

public slots:
  virtual void reset() override;
  virtual void randomize(PlayerBasics* basics = nullptr) override;
  virtual void update(bool resetHp = false,
                      bool resetExp = false,
                      bool resetType = false,
                      bool resetCatchRate = false,
                      bool correctMoves = false) override;
  virtual bool isBoxMon() override;

  void regenStats();

public:
  // Pre-generated stats when not in box
  int maxHP;
  int attack;
  int defense;
  int speed;
  int special;
};

#endif // POKEMONPARTY_H
