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
#include "pokemonparty.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"

PokemonParty::PokemonParty(SaveFile* saveFile,
                           var16 offset,
                           var16 nicknameStartOffset,
                           var16 otNameStartOffset,
                           var8 index)
{
  load(saveFile, offset, nicknameStartOffset, otNameStartOffset, index);
}

PokemonParty::~PokemonParty() {}

SaveFileIterator* PokemonParty::load(SaveFile* saveFile,
                        var16 offset,
                        var16 nicknameStartOffset,
                        var16 otNameStartOffset,
                        var8 index)
{
  if(saveFile == nullptr) {
    reset();
    return nullptr;
  }

  auto it = PokemonBox::load(saveFile,
                             offset,
                             nicknameStartOffset,
                             otNameStartOffset,
                             index,
                             0x2C);

  level = it->getByte();
  maxHP = it->getWord();
  attack = it->getWord();
  defense = it->getWord();
  speed = it->getWord();
  special = it->getWord();

  return it;
}

SaveFileIterator* PokemonParty::save(SaveFile* saveFile,
                        var16 offset,
                        svar32 speciesStartOffset,
                        var16 nicknameStartOffset,
                        var16 otNameStartOffset,
                        var8 index)
{
  auto it = PokemonBox::save(saveFile,
                             offset,
                             speciesStartOffset,
                             nicknameStartOffset,
                             otNameStartOffset,
                             index,
                             0x2C);

  it->setByte(level);
  it->setWord(maxHP);
  it->setWord(attack);
  it->setWord(defense);
  it->setWord(speed);
  it->setWord(special);

  return it;
}

void PokemonParty::reset()
{
  PokemonBox::reset();
  maxHP = 0;
  attack = 0;
  defense = 0;
  speed = 0;
  special = 0;
}

void PokemonParty::randomize(PlayerBasics* basics)
{
  // We can't randomize these stats, they're just cached pre-gen stats
  // Tell the base class to randomize it's data
  PokemonBox::randomize(basics);

  // After that we need to re-gen these stats to reflect base class
  regenStats();
}

void PokemonParty::regenStats()
{
  if(!isValid())
    return;

  maxHP = hpStat();
  attack = atkStat();
  defense = defStat();
  speed = spdStat();
  special = spStat();
}

void PokemonParty::update(bool resetHp, bool resetExp, bool resetType, bool resetCatchRate)
{
  PokemonBox::update(resetHp, resetExp, resetType, resetCatchRate);
  regenStats();
}

void PokemonParty::copyFrom(PokemonBox* pkmn)
{
  PokemonBox::copyFrom(pkmn);
  regenStats();
}
