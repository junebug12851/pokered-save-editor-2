/*
  * Copyright 2020 Fairy Fox
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
 * @file pokemonstoragebox.cpp
 * @brief Implementation of PokemonStorageBox -- a box of Pokemon (the party's
 *        base class), incl. move/remove/relocate. See pokemonstoragebox.h.
 */

#include "./pokemonstoragebox.h"
#include "../../qmlownership.h"
#include "./pokemonbox.h"
#include "../../savefile.h"
#include "../savefileexpanded.h"
#include "../player/player.h"
#include "../player/playerbasics.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-common/random.h>

PokemonStorageBox::PokemonStorageBox(int maxSize, SaveFile* saveFile, var16 boxOffset)
  : maxSize(maxSize)
{
  // Own this box (and, via inheritance, the party PlayerPokemon) in C++ so QML's
  // GC can never free it. The model exposes boxes to QML through public SLOTS
  // (getCurBox()/getBox(), callable from QML exactly like Q_INVOKABLE) and via
  // Storage::boxAt(); a parentless QObject returned that way defaults to
  // JavaScriptOwnership. If QML GC'd a box, its dtor deleteLater()s EVERY mon it
  // holds -- so the whole box's mons became dangling regardless of the mons'
  // own ownership, then a virtual call on a freed mon (e.g. isBoxMon() in
  // PokemonStorageModel::data) crashed intermittently. Owning the box at birth
  // closes that at the source (storage still frees boxes itself). The party path
  // (getBox()->party) never went through the boxAt() wrap, so it was unprotected
  // until now. See qmlownership.h / qt6-patterns.md.
  QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

  load(saveFile, boxOffset);
}

PokemonStorageBox::~PokemonStorageBox()
{
  for(auto mon : pokemon)
    mon->deleteLater();
}

int PokemonStorageBox::pokemonCount()
{
  return pokemon.size();
}

int PokemonStorageBox::pokemonMax()
{
  return maxSize;
}

bool PokemonStorageBox::isFull()
{
  return pokemon.size() >= maxSize;
}

PokemonBox* PokemonStorageBox::pokemonAt(int ind)
{
  return qmlCppOwned(pokemon.at(ind));
}

bool PokemonStorageBox::pokemonMove(int from, int to)
{
  if(pokemon.size() <= 0 ||
     from == to ||
     from >= pokemon.size() ||
     from < 0 ||
     to >= pokemon.size() ||
     to < 0)
    return false;

  // Grab and remove item
  auto eFrom = pokemon.at(from);
  pokemon.removeAt(from);

  // Insert it elsewhere
  pokemon.insert(to, eFrom);

  pokemonMoveChange(from, to);
  pokemonChanged();

  return true;
}

void PokemonStorageBox::pokemonRemove(int ind)
{
  if(pokemon.size() <= 0 ||
     ind < 0 ||
     ind >= pokemon.size())
    return;

  pokemon.at(ind)->deleteLater();
  pokemon.removeAt(ind);
  pokemonRemoveChange(ind);
  pokemonChanged();
}

void PokemonStorageBox::pokemonNew()
{
  if(pokemon.size() >= maxSize)
    return;

  auto mon = PokemonBox::newPokemon(PokemonRandom::Random_Starters, file->dataExpanded->player->basics);
  pokemon.append(mon);
  pokemonInsertChange();
  pokemonChanged();
}

bool PokemonStorageBox::relocateAll(PokemonStorageBox* dst)
{
  bool ret = true;

  while(pokemon.size() > 0 && dst->pokemon.size() < dst->pokemonMax()) {
    if(!relocateOne(dst, 0))
      ret = false;
  }

  return ret;
}

bool PokemonStorageBox::relocateOne(PokemonStorageBox* dst, int ind)
{
  if(pokemon.size() <= 0 ||
     ind < 0 ||
     ind >= pokemon.size() ||
     dst->pokemon.size() >= dst->pokemonMax())
    return false;

  auto el = pokemon.at(ind);
  beforePokemonRelocate(el);

  pokemon.removeAt(ind);
  pokemonRemoveChange(ind);
  pokemonChanged();

  dst->pokemon.append(el);
  dst->pokemonInsertChange();
  dst->pokemonChanged();

  return true;
}

void PokemonStorageBox::load(SaveFile* saveFile, var16 boxOffset)
{
  reset();

  this->file = saveFile;

  if(saveFile == nullptr)
    return;

  auto toolset = saveFile->toolset;

  // Simply read-in and append new Pokemon in box
  for (var8 i = 0; i < toolset->getByte(boxOffset) && i < maxSize; i++) {
    pokemon.append(new PokemonBox(
                     saveFile,
                     boxOffset + 0x16,
                     boxOffset + 0x386,
                     boxOffset + 0x2AA,
                     i));

    pokemonInsertChange();
  }

  pokemonChanged();
}

void PokemonStorageBox::save(SaveFile* saveFile, var16 boxOffset)
{
  auto toolset = saveFile->toolset;

  // Set box size
  toolset->setByte(boxOffset, pokemon.size());

  // Save each Pokemon
  for (var8 i = 0; i < pokemon.size() && i < maxSize; i++) {
    pokemon.at(i)->save(
          saveFile,
          boxOffset + 0x16,
          boxOffset + 0x1,
          boxOffset + 0x386,
          boxOffset + 0x2AA,
          i
          );
  }

  // Mark end of species list if not full box
  if(pokemon.size() >= maxSize)
    return;

  var16 speciesOffset = boxOffset + 1 + pokemon.size();
  toolset->setByte(speciesOffset, 0xFF);
}

void PokemonStorageBox::reset()
{
  for(auto mon : pokemon)
    mon->deleteLater();

  pokemon.clear();

  pokemonResetChange();
  pokemonChanged();
}

void PokemonStorageBox::randomize(PlayerBasics* basics)
{
  reset();

  // Determine fill amount

  // 65% chance the box will remain empty
  if(Random::inst()->flipCoin())
    return;

  // If the box is to be filled, how much so?
  int maxRange[] = {
    (int)(maxSize * 0.25),
    (int)(maxSize * 0.50),
    (int)(maxSize * 0.75),
    (int)(maxSize * 1.00)
  };

  // Start with default of index 1
  int maxRangeSel = 0;

  // Make it incresingly difficult to proceed to higher indexes
  if(Random::inst()->chanceSuccess(65))
    maxRangeSel = 1;
  else if(Random::inst()->chanceSuccess(65))
    maxRangeSel = 2;
  else if(Random::inst()->chanceSuccess(65))
    maxRangeSel = 3;

  // Get end capacity
  var8 count = Random::inst()->rangeInclusive(0, maxRange[maxRangeSel]);

  // Insert Pokemon
  for(var8 i = 0; i < count; i++) {
    auto tmp = new PokemonBox;
    tmp->randomize(basics);
    pokemon.append(tmp);
    pokemonInsertChange();
  }

  pokemonChanged();
}
