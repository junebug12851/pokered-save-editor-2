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
#include "../../../../common/types.h"
class SaveFile;

class PokemonParty : public PokemonBox
{
  Q_OBJECT

  Q_PROPERTY(var16 maxHP_ MEMBER maxHP NOTIFY maxHPChanged)
  Q_PROPERTY(var16 attack_ MEMBER attack NOTIFY attackChanged)
  Q_PROPERTY(var16 defense_ MEMBER defense NOTIFY defenseChanged)
  Q_PROPERTY(var16 speed_ MEMBER speed NOTIFY speedChanged)
  Q_PROPERTY(var16 special_ MEMBER special NOTIFY specialChanged)

public:
  PokemonParty(SaveFile* saveFile = nullptr,
               var16 offset = 0,
               var16 nicknameStartOffset = 0,
               var16 otNameStartOffset = 0,
               var8 index = 0);

  virtual ~PokemonParty();

  Q_INVOKABLE virtual void copyFrom(PokemonBox* pkmn);

signals:
  void maxHPChanged();
  void attackChanged();
  void defenseChanged();
  void speedChanged();
  void specialChanged();

public slots:
  SaveFileIterator* load(SaveFile* saveFile = nullptr,
            var16 offset = 0,
            var16 nicknameStartOffset = 0,
            var16 otNameStartOffset = 0,
            var8 index = 0);

  SaveFileIterator* save(SaveFile* saveFile,
            var16 offset,
            svar32 speciesStartOffset,
            var16 nicknameStartOffset,
            var16 otNameStartOffset,
            var8 index);

  void reset();
  void randomize(PlayerBasics* basics = nullptr);
  void regenStats();
  virtual void update(bool resetHp, bool resetExp, bool resetType, bool resetCatchRate);

public:
  // Pre-generated stats when not in box
  var16 maxHP;
  var16 attack;
  var16 defense;
  var16 speed;
  var16 special;
};

#endif // POKEMONPARTY_H
