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

#include "./player.h"
#include "./playerbasics.h"
#include "./playerpokemon.h"
#include "../savefileexpanded.h"
#include "../storage.h"
#include "../fragments/pokemonstorageset.h"
#include "../fragments/pokemonstoragebox.h"
#include "../fragments/itemstoragebox.h"
#include "../fragments/pokemonbox.h"
#include "../fragments/pokemonparty.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/names.h"
#include "../../../db/pokemon.h"
#include "../../../../common/random.h"

PlayerBasics::PlayerBasics(SaveFile* saveFile)
{
  load(saveFile);
}

PlayerBasics::~PlayerBasics() {}

void PlayerBasics::load(SaveFile* saveFile)
{
  reset();

  if(saveFile == nullptr) {
    return;
  }

  file = saveFile;

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator();

  playerName = toolset->getStr(0x2598, 0xB, 7+1);
  playerNameChanged();

  playerID = toolset->getWord(0x2605);
  playerIDChanged();

  money = toolset->getBCD(0x25F3, 3);
  moneyChanged();

  coins = toolset->getBCD(0x2850, 2);
  coinsChanged();

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

  badgesChanged();

  playerStarter = toolset->getByte(0x29C3);
  playerStarterChanged();

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
  file = nullptr;

  playerName = "";
  playerNameChanged();

  playerID = 0;
  playerIDChanged();

  money = 0;
  moneyChanged();

  coins = 0;
  coinsChanged();

  for(var8 i = 0; i < 8; i++)
    badges[i] = false;

  badgesChanged();

  playerStarter = 0;
  playerStarterChanged();
}

void PlayerBasics::randomize()
{
  reset();

  // Random name and ID
  playerName = NamesDB::randomName();
  playerNameChanged();

  playerID = Random::rangeInclusive(0x0000, 0xFFFF);
  playerIDChanged();

  // Figure out random money and coins that are reasonable
  // We want a minimum of 100 money and 0 coins and a maximum of the chosen
  // maximum
  money = Random::rangeInclusive(100, 6000);
  moneyChanged();

  coins = Random::rangeInclusive(0, 100);
  coinsChanged();

  // Zero out all badges, it's far too complicated in gen 1 games to properly
  // progress in the game randomly and takes away from fun of a new game
  for(var8 i = 0; i < 8; i++)
    badges[i] = false;

  // An exception will be made to these badges, always enable them because it
  // lets the player move around better with important HMs out of battle. It
  // will still let the player battle the gym for first time - the badge just
  // gets the player HM moves
  badges[(var8)Badges::Thunder] = true;
  badges[(var8)Badges::Cascade] = true;
  badges[(var8)Badges::Soul] = true;
  badges[(var8)Badges::Rainbow] = true;

  badgesChanged();

  randomizeStarter();
}

void PlayerBasics::randomizeStarter()
{
  // Determine a random starter
  var8 starter[3] = {
    PokemonDB::ind.value("Bulbasaur")->ind, // Bulbasaur
    PokemonDB::ind.value("Charmander")->ind, // Charmander
    PokemonDB::ind.value("Squirtle")->ind // Squirtle
  };

  var8 pick = Random::rangeExclusive(0, 3);

  playerStarter = starter[pick];
  playerStarterChanged();
}

void PlayerBasics::randomizeCoins()
{
  coins = Random::rangeExclusive(0, 9999);
  coinsChanged();
}

void PlayerBasics::randomizeMoney()
{
  money = Random::rangeExclusive(0, 999999);
  moneyChanged();
}

void PlayerBasics::randomizeID()
{
  playerID = Random::rangeExclusive(0x0000, 0xFFFF);
  playerIDChanged();
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

int PlayerBasics::badgeCount()
{
  return maxBadges;
}

bool PlayerBasics::badgeAt(int ind)
{
  return badges[ind];
}

void PlayerBasics::badgeSet(int ind, bool val)
{
  badges[ind] = val;
  badgesChanged();
}

void PlayerBasics::fullSetPlayerName(QString val)
{
  // Get List of Non-Trade Mons
  auto nonTradeMons = getNonTradeMons();

  // Change Player Name
  playerName = val;

  // Fix non-Trade Mons to reflect new OT Name
  fixNonTradeMons(nonTradeMons);

  // Announce change
  playerNameChanged();
}

void PlayerBasics::fullSetPlayerId(int id)
{
  // Get List of Non-Trade Mons
  auto nonTradeMons = getNonTradeMons();

  // Change Player Name
  playerID = id;

  // Fix non-Trade Mons to reflect new OT Name
  fixNonTradeMons(nonTradeMons);

  // Announce change
  playerIDChanged();
}

QString PlayerBasics::getPlayerName()
{
  return playerName;
}

int PlayerBasics::getPlayerId()
{
  return playerID;
}

QVector<PokemonBox*> PlayerBasics::getNonTradeMons()
{
  // Do nothing if file is invalid
  if(file == nullptr)
    return QVector<PokemonBox*>();

  // List of mons to fix
  QVector<PokemonBox*> monsToFix;

  // Add all party members to fix
  for(auto el : file->dataExpanded->player->pokemon->pokemon) {
    if(!el->hasTradeStatus(this))
      monsToFix.append(el);
  }

  // Add all box mons to fix
  for(int i = 0; i < maxPokemonBoxes; i++) {

    // Go through each box
    auto box = file->dataExpanded->storage->boxAt(i);
    if(box == nullptr)
      continue;

    // Go through all the Pokemon in each box
    for(auto el : box->pokemon) {
      if(!el->hasTradeStatus(this))
        monsToFix.append(el);
    }
  }

  return monsToFix;
}

void PlayerBasics::fixNonTradeMons(QVector<PokemonBox*> mons)
{
  for(auto el : mons)
    el->changeOtData(true, this);
}
