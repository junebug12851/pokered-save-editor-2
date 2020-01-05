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
#include "playerbasics.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/names.h"
#include "../../../db/pokemon.h"

#include <QRandomGenerator>

PlayerBasics::PlayerBasics(SaveFile* saveFile)
{
  load(saveFile);
}

PlayerBasics::~PlayerBasics()
{}

void PlayerBasics::load(SaveFile* saveFile)
{
  if(saveFile == nullptr) {
    reset();
    return;
  }

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator();

  playerName = toolset->getStr(0x2598, 0xB, 7+1);
  playerID = toolset->getWord(0x2605);
  money = toolset->getBCD(0x25F3, 3);
  coins = toolset->getBCD(0x2850, 2);

  // We only pull from the first duplicate, there are 2 identical sets
  // of badges in the save file
  it->offsetTo(0x2602);

  // Load and copy over badges
  bool tmpBadges[8] = {
    it->getBit(1, 0),
    it->getBit(1, 1),
    it->getBit(1, 2),
    it->getBit(1, 3),
    it->getBit(1, 4),
    it->getBit(1, 5),
    it->getBit(1, 6),
    it->getBit(1, 7)
  };

  for(var8 i = 0; i < 8; i++)
    badges[i] = tmpBadges[i];

  playerStarter = toolset->getByte(0x29C3);

  delete it;
}

void PlayerBasics::save(SaveFile* saveFile)
{
  auto toolset = saveFile->toolset;

  toolset->setStr(0x2598, 0xB, 7+1, playerName);
  toolset->setWord(0x2605, playerID);
  toolset->setBCD(0x25F3, 3, money);
  toolset->setBCD(0x2850, 2, coins);
  toolset->setByte(0x29C3, playerStarter);

  // Badges have to be duplicated on the save file
  setBadges(saveFile, 0x2602);
  setBadges(saveFile, 0x29D6);
}

void PlayerBasics::reset()
{
  playerName = "";
  playerID = 0;
  money = 0;
  coins = 0;
  for(var8 i = 0; i < 8; i++)
    badges[i] = false;
  playerStarter = 0;
}

void PlayerBasics::randomize()
{
  // Random name and ID
  playerName = NamesDB::randomName();
  playerID = QRandomGenerator::global()->bounded(0x0000, 0xFFFF);

  // Figure out random money and coins that are reasonable
  // We want a minimum of 100 money and 0 coins and a maximum of the chosen
  // maximum
  money = QRandomGenerator::global()->bounded(100, 6000);
  coins = QRandomGenerator::global()->bounded(0, 100);

  // Zero out all badges, it's far too complicated in gen 1 games to properly
  // progress in the game randomly and takes away from fun of a new game
  for(var8 i = 0; i < 8; i++)
    badges[i] = false;

  // Determine a random starter
  var8 starter[3] = {
    0x99, // Bulbasaur
    0xB0, // Charmander
    0xB1 // Squirtle
  };

  var8 pick = QRandomGenerator::global()->bounded(0, 3);

  playerStarter = starter[pick];
}

void PlayerBasics::setBadges(SaveFile* saveFile, var16 offset)
{
  // Prepare iterator
  auto it = saveFile->iterator()->offsetTo(offset);

  // Save Badges
  it->setBit(1, 0, badges[0]);
  it->setBit(1, 1, badges[1]);
  it->setBit(1, 2, badges[2]);
  it->setBit(1, 3, badges[3]);
  it->setBit(1, 4, badges[4]);
  it->setBit(1, 5, badges[5]);
  it->setBit(1, 6, badges[6]);
  it->setBit(1, 7, badges[7]);

  delete it;
}

PokemonDBEntry* PlayerBasics::toStarter()
{
  return PokemonDB::ind.value(QString::number(playerStarter));
}
