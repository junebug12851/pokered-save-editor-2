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

#include <QtDebug>

#include "./itemmarketentrygcpokemon.h"
#include <pse-db/gamecorner.h>
#include "../../data/file/expanded/storage.h"
#include "../../data/file/expanded/player/playerbasics.h"
#include "../../data/file/expanded/player/playerpokemon.h"
#include "../../data/file/expanded/fragments/pokemonbox.h"
#include "../../data/file/expanded/fragments/pokemonparty.h"
#include "../../data/file/expanded/fragments/pokemonstorageset.h"
#include "../../data/file/expanded/fragments/pokemonstoragebox.h"

// Pokemon cannot be bought with anything other than coins to keep
// to keep compatibility with the games. They also cannot be sold back
// for ethical reasons nor can they be bought in money, again, ethical
// reasons. I don't want a PR nightmare and have this whole thing get
// shutdown by GameFreak because of a PR nightmare.

ItemMarketEntryGCPokemon::ItemMarketEntryGCPokemon(GameCornerDBEntry* toGameCorner, PlayerPokemon* party, Storage* storage)
  : ItemMarketEntry(CompatNo, CompatYes), // Only Coins, Only Buying
    toGameCorner(toGameCorner),
    party(party),
    storage(storage)
{
  finishConstruction();
}

ItemMarketEntryGCPokemon::~ItemMarketEntryGCPokemon() {}

QString ItemMarketEntryGCPokemon::_name()
{
  if(!requestFilter())
    return "";

  return toGameCorner->name + " Lv." + QString::number(*toGameCorner->level);
}

int ItemMarketEntryGCPokemon::_inStockCount()
{
  return 0;
}

bool ItemMarketEntryGCPokemon::_canSell()
{
  return true;
}

int ItemMarketEntryGCPokemon::_itemWorth()
{
  if(!requestFilter())
    return 0;

  // We are only dealing with one currency and one mode
  return toGameCorner->price;
}

QString ItemMarketEntryGCPokemon::_whichType()
{
  return type;
}

int ItemMarketEntryGCPokemon::onCartLeft()
{
  if(!requestFilter())
    return 0;

  // Calculate stacks this item takes up
  auto stk = stackCount();

  // Final value to return, a representation of items left that can go onto the
  // cart
  int ret = 0;

  // Calculate stacks others take up
  int totalStackFromOthers = totalStackCount() - stk;

  // Calculate inventory space used and free combined from both bag and box
  int combinedBoxSpace = party->pokemonMax() + (maxPokemonBoxes * boxMaxPokemon);
  int combinedBoxUsed = party->pokemonCount();

  if(storage->boxesFormatted) {
    for(int i = 0; i < maxPokemonBoxes; i++) {
      combinedBoxUsed += storage->boxAt(i)->pokemonCount();
    }
  }
  else {
    combinedBoxUsed += storage->boxAt(storage->curBox)->pokemonCount();
  }

  // Stack space left before requested transaction
  int stackSpaceBefore = combinedBoxSpace - combinedBoxUsed;

  // Stack space after requested transactions
  int stackSpaceLeftAfter = stackSpaceBefore - totalStackFromOthers - stk;

  // Calculate whole stack remaining. There is 1 pokemon max per stack
  // This is the whole new stacks converted to pokemon count.
  ret = stackSpaceLeftAfter * 1;

  // Ret Now contains the maximum pokemon possible left but it doesn't take
  // into consideration how much money is left. We do a quick calculation for
  // that
  int maxAmountFromMoney = moneyLeftover() / itemWorth();

  // Return the smallest of the calculations
  return qMin(ret, maxAmountFromMoney);
}

int ItemMarketEntryGCPokemon::stackCount()
{
  // Pokemon have a stack size of 1
  return onCart;
}

void ItemMarketEntryGCPokemon::checkout() {
  if(!requestFilter() ||
     !canCheckout())
    return;

  // Add in requested Pokemon
  for(int i = 0; i < onCart; i++) {
    // Prepare Pokemon
    auto mon = PokemonBox::newPokemon(toGameCorner->toPokemon, player);
    mon->level = *toGameCorner->level;
    mon->update(true, true, true, true);

    // Find Free Storage in case needed
    auto box = storage->freeSpace();

    // Space in party ?
    if(party->pokemonCount() < party->pokemonMax()) {
      party->pokemon.append(PokemonParty::convertToParty(mon));
      party->pokemonChanged();
    }
    else if(box != nullptr) {
      box->pokemon.append(mon);
      box->pokemonChanged();
    }
    else
      qDebug() << "Mon was not able to be added?";
  }

  onCart = 0;
  player->coins -= cartWorth();
  onCartChanged();
}
