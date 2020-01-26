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
#include "pokemonbox.h"
#include "../savefileexpanded.h"
#include "../player/player.h"
#include "../player/playerbasics.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include "../../../db/pokemon.h"
#include "../../../db/moves.h"
#include "../../../db/names.h"
#include "../../../db/namesPokemon.h"
#include "../../../db/starterPokemon.h"
#include "../../../db/types.h"
#include "../../../../random.h"

#include <QtMath>

PokemonMove::PokemonMove(var8 move, var8 pp, var8 ppUp)
{
  this->moveID = move;
  this->pp = pp;
  this->ppUp = ppUp;

  if(move == 0) {
    randomize();
    ppUp = 0;
  }
}

MoveDBEntry* PokemonMove::toMove()
{
  return MovesDB::ind.value(QString::number(moveID), nullptr);
}

void PokemonMove::randomize()
{
  var8 moveListSize = MovesDB::store.size();

  for(var8 i = 0; i < 4; i++) {
    MoveDBEntry* moveData;

    do
      moveData = MovesDB::ind.value(
            QString::number(Random::rangeExclusive(0, moveListSize)), nullptr);
    while(moveData == nullptr || moveData->glitch == true);

    moveID = moveData->ind;
    moveIDChanged();

    ppUp = Random::rangeInclusive(0, 3);
    ppUpChanged();

    pp = getMaxPP();
    ppChanged();
  }
}

void PokemonMove::maxPpUp()
{
  ppUp = 3;
  ppUpChanged();
}

bool PokemonMove::isMaxPP()
{
  var8 maxPP = getMaxPP();
  if(maxPP == 0)
    return false;

  return pp >= maxPP;
}

int PokemonMove::getMaxPP()
{
  auto moveData = toMove();
  if(moveData == nullptr || !moveData->pp)
    return 0;

  var8 basePP = *moveData->pp;
  var8 ppUps = ppUp;
  var8 ppUpSteps = basePP / 5;

  return basePP + (ppUpSteps * ppUps);
}

bool PokemonMove::isMaxPpUps()
{
  return ppUp >= 3;
}

void PokemonMove::restorePP()
{
  var8 maxPP = getMaxPP();
  if(maxPP == 0)
    return;

  pp = maxPP;
  ppChanged();
}

PokemonBox::PokemonBox(SaveFile* saveFile,
                       var16 startOffset,
                       var16 nicknameStartOffset,
                       var16 otNameStartOffset,
                       var8 index,
                       var8 recordSize)
{
  load(saveFile,
       startOffset,
       nicknameStartOffset,
       otNameStartOffset,
       index,
       recordSize);
}

PokemonBox::~PokemonBox()
{
  reset();
}

PokemonBox* PokemonBox::newPokemon(PokemonRandom::PokemonRandom_ list, PlayerBasics* basics)
{
  PokemonDBEntry* pkmnData;

  if(list == PokemonRandom::Random_All) {
    auto listSize = PokemonDB::store.size();
    var8 ind = Random::rangeExclusive(0, listSize);
    pkmnData = PokemonDB::store.at(ind);
  }
  else if(list == PokemonRandom::Random_Pokedex) {
    var8 dex = Random::rangeExclusive(1, pokemonDexCount);
    pkmnData = PokemonDB::ind.value("dex" + QString::number(dex));
  }
  else if(list == PokemonRandom::Random_Starters)
    pkmnData = StarterPokemonDB::randomAnyStarter();
  else
    pkmnData = StarterPokemonDB::random3Starter();

  return newPokemon(pkmnData, basics);
}

PokemonBox* PokemonBox::newPokemon(PokemonDBEntry* pkmnData, PlayerBasics* basics)
{
  auto pkmn = new PokemonBox();

  pkmn->species = pkmnData->ind;
  pkmn->level = 5;
  pkmn->reRollDVs();
  pkmn->nickname = pkmnData->name;

  if(basics != nullptr)
    pkmn->changeTrade(true, basics);
  else
    pkmn->changeTrade();

  pkmn->resetPokemon();
  pkmn->update(true, true, true, true);

  return pkmn;
}

SaveFileIterator* PokemonBox::load(SaveFile* saveFile,
                      var16 startOffset,
                      var16 nicknameStartOffset,
                      var16 otNameStartOffset,
                      var8 index,
                      var8 recordSize)
{
  reset();

  if(saveFile == nullptr) {
    return nullptr;
  }

  // Calculate record offset
  var16 offset = (recordSize * index) + startOffset;

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator()->offsetTo(offset);

  species = it->getByte();
  speciesChanged();

  hp = it->getWord();
  hpChanged();

  level = it->getByte();
  levelChanged();

  status = it->getByte();
  statusChanged();

  type1 = it->getByte();
  type1Changed();

  type2 = it->getByte();
  type2Changed();

  // Don't duplicate type 1 to type 2, fill type 2 only if it's different
  // Also mark if it was explicitly marked no in-game
  if (type2 == 0xFF) {
    type2Explicit = true;
    type2ExplicitChanged();
  } else if (type1 == type2) {
    type2 = 0xFF;
    type2Changed();
  }

  catchRate = it->getByte();
  catchRateChanged();

  // Save offset to restore later
  it->push();

  // Temporarily save moves for later
  // PP data which is important to moves has to be be gotten later
  QVector<var8> moveIDList;
  for (var8 i = 0; i < 4; i++) {
    var8 moveID = it->getByte();
    if(moveID == 0)
      break;

    moveIDList.append(moveID);
  }

  // Restore offset to start of moves and move past the moves
  it->pop()->offsetBy(0x4);

  otID = it->getWord();
  otIDChanged();

  // Exp is 3 bytes so it's a bit tricky
  auto expRaw = it->getRange(3);
  exp = expRaw[0];
  exp <<= 8;
  exp |= expRaw[1];
  exp <<= 8;
  exp |= expRaw[2];
  expChanged();

  hpExp = it->getWord();
  hpExpChanged();

  atkExp = it->getWord();
  atkExpChanged();

  defExp = it->getWord();
  defExpChanged();

  spdExp = it->getWord();
  spdExpChanged();

  spExp = it->getWord();
  spExpChanged();

  var16 dvTotal = it->getWord();
  dv[(var8)PokemonStats::Attack] = (dvTotal & 0xF000) >> 12;
  dv[(var8)PokemonStats::Defense] = (dvTotal & 0x0F00) >> 8;
  dv[(var8)PokemonStats::Speed] = (dvTotal & 0x00F0) >> 4;
  dv[(var8)PokemonStats::Special] = dvTotal & 0x000F;
  dvChanged();

  it->push();

  // Next gather PP
  QVector<var8> ppList;
  for (var8 i = 0; i < moveIDList.size(); i++) {
    var8 ppListEntry = it->getByte();
    ppList.append(ppListEntry);
  }

  // Combine together in moves from earlier
  moves.clear();
  for (var8 i = 0; i < moveIDList.size(); i++) {
    var8 moveID = moveIDList.at(i);
    var8 pp = ppList[i];
    moves.append(new PokemonMove(
                    moveID,
                    pp & 0b00111111,
                    (pp & 0b11000000) >> 6
                    ));
  }
  movesChanged();

  // Restore back to before PP and move past PP
  it->pop()->offsetBy(0x4);

  // Now we must gather the OT names and Pokemon names whihc were poorly
  // implemented in sometimes arbitrary spots outside of the data sructure
  var16 otNameOffset = (index * 0xB) + otNameStartOffset;
  otName = toolset->getStr(otNameOffset, 0xB, 7+1);
  otNameChanged();

  var16 nicknameOffset = (index * 0xB) + nicknameStartOffset;
  nickname = toolset->getStr(nicknameOffset, 0xB, 10);
  nicknameChanged();

  // Save the iterator to be picked up by sub-class if present
  return it;
}

SaveFileIterator* PokemonBox::save(SaveFile* saveFile,
                      var16 startOffset,
                      svar32 speciesStartOffset,
                      var16 nicknameStartOffset,
                      var16 otNameStartOffset,
                      var8 index,
                      var8 recordSize)
{
  auto toolset = saveFile->toolset;

  // Retrieve stored internals
  var16 offset = (recordSize * index) + startOffset;
  auto it = saveFile->iterator()->offsetTo(offset);
  var16 otNameOffset = (index * 0xB) + otNameStartOffset;
  var16 nicknameOffset = (index * 0xB) + nicknameStartOffset;

  // Add species to species list if exists
  if(speciesStartOffset > 0) {
    var16 speciesOffset = index + speciesStartOffset;
    toolset->setByte(speciesOffset, species);
  }

  // Re-save back
  it->setByte(species);
  it->setWord(hp);

  // Don't save level to BoxData if this is in the party
  // This honors the global don't touch policy
  // which is don't touch any bits that don't need to be changed
  if (recordSize == 0x21) {
    it->setByte(level);
  } else {
    it->inc();
  }

  it->setByte(status);
  it->setByte(type1);

  // If type 2 explicit no then just write what's in type 2
  if (type2Explicit) {
    it->setByte(type2);

    // If type 2 is not explicitly no but implicitly no (Type 1 and 2 were marked the same)
    // save it as type 1
  } else if (type2 == 0xFF) {
    it->setByte(type1);

    // Else just save type 2
  } else {
    it->setByte(type2);
  }

  it->setByte(catchRate);

  it->push();
  for(var8 i = 0; i < moves.size(); i++) {
    it->setByte(moves.at(i)->moveID);
  }
  it->pop()->offsetBy(4);

  it->setWord(otID);

  it->setByte((exp & 0xFF0000) >> 16);
  it->setByte((exp & 0x00FF00) >> 8);
  it->setByte(exp & 0x0000FF);

  it->setWord(hpExp);
  it->setWord(atkExp);
  it->setWord(defExp);
  it->setWord(spdExp);
  it->setWord(spExp);

  var16 dvTmp = 0;
  dvTmp |= (dv[(var8)PokemonStats::Attack] << 12);
  dvTmp |= (dv[(var8)PokemonStats::Defense] << 8);
  dvTmp |= (dv[(var8)PokemonStats::Speed] << 4);
  dvTmp |= dv[(var8)PokemonStats::Special];
  it->setWord(dvTmp);

  it->push();
  for (var8 i = 0; i < moves.size(); i++) {
    var8 ppCombined = (moves.at(i)->ppUp << 6) | moves.at(i)->pp;
    it->setByte(ppCombined);
  }
  it->pop()->offsetBy(4);

  toolset->setStr(otNameOffset, 0xB, 10+1, otName);
  toolset->setStr(nicknameOffset, 0xB, 10+1, nickname);

  return it;
}

void PokemonBox::reset()
{
  species = 0;
  speciesChanged();

  hp = 0;
  hpChanged();

  level = 0;
  levelChanged();

  status = 0;
  statusChanged();

  type1 = 0;
  type1Changed();

  type2 = 0;
  type2Changed();

  catchRate = 0;
  catchRateChanged();

  otID = 0;
  otIDChanged();

  exp = 0;
  expChanged();

  hpExp = 0;
  hpExpChanged();

  atkExp = 0;
  atkExpChanged();

  defExp = 0;
  defExpChanged();

  spdExp = 0;
  spdExpChanged();

  spExp = 0;
  spExpChanged();

  otName = "";
  otNameChanged();

  nickname = "";
  nicknameChanged();

  dv[0] = 0;
  dv[1] = 0;
  dv[2] = 0;
  dv[3] = 0;
  dvChanged();

  clearMoves();

  type2Explicit = false;
  type2ExplicitChanged();
}

void PokemonBox::randomize(PlayerBasics* basics)
{
  reset();

  // Generate a random level 5 Pokemon from the pokedex
  // Bump it's level up to a random value
  // Give it a random name, otName, and otID
  // Then fix all of it's types, exp, stats, etc.. to be game accurate
  auto pkmn = PokemonBox::newPokemon(PokemonRandom::Random_Pokedex, basics);
  copyFrom(pkmn);
  delete pkmn;

  level = Random::rangeInclusive(1, pokemonLevelMax);
  levelChanged();

  atkExp = Random::rangeInclusive(0, 0xFFFF);
  atkExpChanged();

  defExp = Random::rangeInclusive(0, 0xFFFF);
  defExpChanged();

  spdExp = Random::rangeInclusive(0, 0xFFFF);
  spdExpChanged();

  spExp = Random::rangeInclusive(0, 0xFFFF);
  spExpChanged();

  hpExp = Random::rangeInclusive(0, 0xFFFF);
  hpExpChanged();

  // Delete it's moves and re-create 4 new non-glitch random moves
  randomizeMoves();

  // Make the pokemon a non-trade pokemon
  changeTrade(true, basics);

  // 50/50 chance of not having a nickname
  // If true, removes nickname
  // If false, assigns random nickname
  bool noNick = Random::flipCoin();
  changeName(noNick);

  // This is where we make the Pokemon completely game accurate
  update(true, true, true, true);

  // Heal the Pokemon
  heal();

  // This is where we give the Pokemon whacky types for fun
  // We have to do this after all the code above otherwise it'll be re-corrected
  // to be game accurate
  auto type1 = TypesDB::store.at(Random::rangeExclusive(0, TypesDB::store.size()));
  TypeDBEntry* type2 = nullptr;

  // 25% chance of type 2
  bool hasType2 = Random::chanceSuccess(25);
  if(hasType2)
    type2 = TypesDB::store.at(Random::rangeExclusive(0, TypesDB::store.size()));

  if(type1->ind == type2->ind)
    type2 = nullptr;

  this->type1 = type1->ind;
  type1Changed();

  if(type2 != nullptr)
    this->type2 = type2->ind;
  else
    this->type2 = 0xFF;
  type2Changed();
}

void PokemonBox::clearMoves()
{
  for(auto move : moves)
    delete move;

  moves.clear();

  movesChanged();
}

// Is this a valid Pokemon? (Is it even in the Pokedex?)
// If not returns false, otherwise returns Pokemon Record
PokemonDBEntry* PokemonBox::isValid()
{
  // Get Pokemon Record
  // The Pokemon Array is organized by species ID with 1 top entry missing
  // thus offset by 1 accordingly
  auto record = PokemonDB::ind.value(QString::number(species), nullptr);

  // Check it's a valid Pokemon (not glitch)
  if(record == nullptr || record->glitch || !(record->pokedex))
    return nullptr;

  return record;
}

bool PokemonBox::isValidBool()
{
  return isValid() != nullptr;
}

unsigned int PokemonBox::levelToExp(int level)
{
  auto record = isValid();
  double exp = 0;

  if(level < 0)
    level = this->level;

  // Proceed only if it's valid
  if(record == nullptr)
    return exp;

  // Obtain it's growth rate and calculate accordingly it's exp for the given level
  var8 gr = *record->growthRate;

  // Growth Rate 0: Medium Fast
  if(gr == 0)
    exp = qPow(level, 3);

  // Growth Rate 3: Medium Slow
  else if(gr == 3)
    exp = (1.2 * qPow(level, 3)) - (15 * qPow(level, 2)) + (100*level) - 140;

  // Growth Rate 4: Fast
  else if(gr == 4)
    exp = (4 * qPow(level, 3)) / 5;

  // Growth Rate 5: Slow
  else if(gr == 5)
    exp = (5 * qPow(level, 3)) / 4;

  // Return EXP
  return qFloor(exp);
}

var32 PokemonBox::expLevelRangeStart()
{
  if(isValid() == nullptr)
    return this->exp;

  return levelToExp((level < 100) ? level : 100);
}

var32 PokemonBox::expLevelRangeEnd()
{
  if(isValid() == nullptr)
    return exp;

  return levelToExp((level < 100) ? level + 1 : 100) - 1;
}

float PokemonBox::expLevelRangePercent()
{
  if(this->isValid() == nullptr)
    return 0;

  if(level >= 100)
    return 1;

  var32 curExp = exp - expLevelRangeStart();
  var32 expEnd = expLevelRangeEnd() - expLevelRangeStart();

  // Return percentage
  return curExp / expEnd;
}

void PokemonBox::resetExp()
{
  if(isValid() == nullptr)
    return;

  exp = expLevelRangeStart();
  expChanged();
}

int PokemonBox::hpDV()
{
  var8 hpDv = 0;

  if((dv[(var8)PokemonStats::Attack] % 2) != 0)
    hpDv |= 8;

  if((dv[(var8)PokemonStats::Defense] % 2) != 0)
    hpDv |= 4;

  if((dv[(var8)PokemonStats::Speed] % 2) != 0)
    hpDv |= 2;

  if((dv[(var8)PokemonStats::Special] % 2) != 0)
    hpDv |= 1;

  return hpDv;
}

int PokemonBox::hpStat()
{
  auto record = isValid();

  // Proceed only if it's valid
  if(record == nullptr || !(record->baseHp))
    return 1;

  return qFloor((((*record->baseHp + hpDV())*2+qFloor(qFloor(qSqrt(hpExp))/4))*level)/100) + level + 10;
}

int PokemonBox::nonHpStat(PokemonStats::PokemonStats_ stat)
{
  auto record = isValid();

  // Proceed only if it's valid
  if(record == nullptr)
    return 0;

  var8 baseStat = 0;
  var8 dvLocal = 0;
  var8 evLocal = 0;

  if(stat == PokemonStats::Attack) {
    baseStat = *record->baseAttack;
    dvLocal = dv[(var8)PokemonStats::Attack];
    evLocal = atkExp;
  }
  else if(stat == PokemonStats::Defense) {
    baseStat = *record->baseDefense;
    dvLocal = dv[(var8)PokemonStats::Defense];
    evLocal = defExp;
  }
  else if(stat == PokemonStats::Speed) {
    baseStat = *record->baseSpeed;
    dvLocal = dv[(var8)PokemonStats::Speed];
    evLocal = spdExp;
  }
  else if(stat == PokemonStats::Special) {
    baseStat = *record->baseSpecial;
    dvLocal = dv[(var8)PokemonStats::Speed];
    evLocal = spdExp;
  }

  return qFloor((((baseStat+dvLocal)*2+qFloor(qFloor(qSqrt(evLocal))/4))*level)/100) + 5;
}

void PokemonBox::update(bool resetHp,
                        bool resetExp,
                        bool resetType,
                        bool resetCatchRate)
{
  auto record = isValid();
  if(record == nullptr)
    return;

  if(resetHp) {
    hp = hpStat();
    hpChanged();
  }

  if(resetType && record->toType1) {
    type1 = (*record).toType1->ind;
    type1Changed();
  }

  if(resetType && record->toType2) {
    type2 = (*record).toType2->ind;
    type2Changed();
  }

  if(resetType && type1 == type2) {
    type2 = 0xFF;
    type2Changed();
  }

  if(resetCatchRate && record->catchRate) {
    catchRate = *record->catchRate;
    catchRateChanged();
  }

  if(resetExp) {
    exp = levelToExp();
    expChanged();
  }
}

bool PokemonBox::isHealed()
{
  return isMaxHp() && !isAfflicted() && isMaxPP();
}

bool PokemonBox::isAfflicted()
{
  return status > 0;
}

bool PokemonBox::isMaxHp()
{
  if(!isValid())
    return false;

  return hp == hpStat();
}

void PokemonBox::heal()
{
  hp = hpStat();
  hpChanged();

  status = 0;
  statusChanged();

  for(auto move : moves)
    move->restorePP();
}

bool PokemonBox::hasNickname()
{
  return isValid()->name == nickname;
}

bool PokemonBox::hasTradeStatus(PlayerBasics* basics)
{
  return basics->playerName == otName && basics->playerID == otID;
}

void PokemonBox::changeName(bool removeNickname)
{
  if(!removeNickname)
    nickname = NamesPokemonDB::randomName();
  else if(removeNickname)
    nickname = toData()->name;

  nicknameChanged();
}

void PokemonBox::changeOtData(bool removeOtData, PlayerBasics* basics)
{
  if(!removeOtData) {
    otName = NamesDB::randomName();
    otID = Random::rangeInclusive(0x0000, 0xFFFF);
  }
  else if(removeOtData && basics != nullptr) {
    otName = basics->playerName;
    otID = basics->playerID;
  }

  otNameChanged();
  otIDChanged();
}

void PokemonBox::changeTrade(bool removeTradeStatus, PlayerBasics* basics)
{
  changeName(removeTradeStatus);
  changeOtData(removeTradeStatus, basics);
}

bool PokemonBox::hasEvolution()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  if(record->evolution.size() == 0)
    return false;

  return true;
}

bool PokemonBox::hasDeEvolution()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  if(record->toDeEvolution == nullptr)
    return false;

  return true;
}

void PokemonBox::evolve()
{
  auto record = isValid();

  if(!hasEvolution())
    return;

  // Does it have a nickname before evolution
  bool nickStatus = hasNickname();

  // For Eevee evolutions, randomly pick one
  if(record->evolution.size() > 1) {
    var8 ind = Random::rangeExclusive(0, record->evolution.size());
    species = record->evolution.at(ind)->toEvolution->ind;
  }
  else
    species = record->evolution.at(0)->toEvolution->ind;

  speciesChanged();

  // Update name if no nickname
  if(!nickStatus)
    changeName(true);

  // Update all stats, make everything else game accurate
  update(true, true, true, true);
}

void PokemonBox::deEvolve()
{
  auto record = isValid();

  if(!hasDeEvolution())
    return;

  // Does it have a nickname before de-evolution
  bool nickStatus = hasNickname();

  species = record->toDeEvolution->ind;
  speciesChanged();

  // Update name if no nickname
  if(!nickStatus)
    changeName(true);

  // Update all stats, make everything game accurate
  update(true, true, true, true);

  // As for moves, given this is made-up territory, I'm going with evolution
  // rules and saying the Pokemon can keep the evolved moves because it's the
  // same Pokemon that's reverted to a younger self and has the same memory.
}

bool PokemonBox::isMaxLevel()
{
  return level >= 100;
}

bool PokemonBox::isMaxPP()
{
  bool ret = true;

  for(auto move : moves)
    if(!move->isMaxPP())
      ret = false;

  return ret;
}

bool PokemonBox::isMaxPpUps()
{
  bool ret = true;

  for(auto move : moves)
    if(!move->isMaxPpUps())
      ret = false;

  return ret;
}

bool PokemonBox::isMaxEVs()
{
  return atkExp == 0xFFFF &&
      defExp == 0xFFFF &&
      spdExp == 0xFFFF &&
      spExp == 0xFFFF &&
      hpExp == 0xFFFF;
}

bool PokemonBox::isMinEvs()
{
  return atkExp == 0 ||
      defExp == 0 ||
      spdExp == 0 ||
      spExp == 0 ||
      hpExp == 0;
}

bool PokemonBox::isMaxDVs()
{
  bool ret = true;

  for(var8 i = 0; i < 4; i++)
    if(dv[i] < 15) ret = false;

  return ret;
}

void PokemonBox::maxLevel()
{
  level = 100;
  levelChanged();

  update(true, true);
}

void PokemonBox::maxPpUps()
{
  for(auto move : moves)
    move->maxPpUp();
}

void PokemonBox::maxDVs()
{
  for(var8 i = 0; i < 4; i++)
    dv[i] = 15;

  dvChanged();
}

void PokemonBox::reRollDVs()
{
  for(var8 i = 0; i < 4; i++)
    dv[i] = Random::rangeInclusive(0, 15);

  dvChanged();
}

void PokemonBox::maxEVs()
{
  hpExp = 0xFFFF;
  hpExpChanged();

  atkExp = 0xFFFF;
  atkExpChanged();

  defExp = 0xFFFF;
  defExpChanged();

  spdExp = 0xFFFF;
  spdExpChanged();

  spExp = 0xFFFF;
  spExpChanged();

  update(true);
}

void PokemonBox::resetEVs()
{
  hpExp = 0;
  hpExpChanged();

  atkExp = 0;
  atkExpChanged();

  defExp = 0;
  defExpChanged();

  spdExp = 0;
  spdExpChanged();

  spExp = 0;
  spExpChanged();

  update(true);
}

void PokemonBox::maxOut()
{
  maxLevel();
  maxPpUps();
  maxEVs();
  maxDVs();
  heal();

  update(true, true);
}

void PokemonBox::randomizeMoves()
{
  clearMoves();

  for(var8 i = 0; i < 4; i++) {
    moves.append(new PokemonMove);
    moves.at(i)->randomize();
  }

  movesChanged();
}

bool PokemonBox::isPokemonReset()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  bool movesReset = true;

  for(var8 i = 0; i < moves.size(); i++) {
    auto move = moves.at(i);

    if(move->toMove() == nullptr)
      movesReset = false;
    if(!movesReset)
      break;

    if(move->moveID != record->toInitial.at(i)->ind)
      movesReset = false;
    if(!move->isMaxPP())
      movesReset = false;
    if(!move->isMaxPpUps())
      movesReset = false;

    if(!movesReset)
      break;
  }

  return level == 5 && movesReset && isMinEvs() && isHealed();
}

void PokemonBox::resetPokemon()
{
  level = 5;
  levelChanged();

  auto record = isValid();
  if(record == nullptr)
    return;

  clearMoves();

  for(auto moveData : record->toInitial)
    moves.append(new PokemonMove(moveData->ind, *moveData->pp, 0));

  movesChanged();

  resetEVs();
  heal();
  update(true, true, true, true);
}

void PokemonBox::copyFrom(PokemonBox* pkmn)
{
  species = pkmn->species;
  speciesChanged();

  hp = pkmn->hp;
  hpChanged();

  level = pkmn->level;
  levelChanged();

  status = pkmn->status;
  statusChanged();

  type1 = pkmn->type1;
  type1Changed();

  type2 = pkmn->type2;
  type2Changed();

  catchRate = pkmn->catchRate;
  catchRateChanged();

  otID = pkmn->otID;
  otIDChanged();

  exp = pkmn->exp;
  expChanged();

  hpExp = pkmn->hpExp;
  hpExpChanged();

  atkExp = pkmn->atkExp;
  atkExpChanged();

  defExp = pkmn->defExp;
  defExpChanged();

  spdExp = pkmn->spExp;
  spdExpChanged();

  spExp = pkmn->spExp;
  spExpChanged();

  otName = pkmn->otName;
  otNameChanged();

  nickname = pkmn->nickname;
  nicknameChanged();

  dv[0] = pkmn->dv[0];
  dv[1] = pkmn->dv[1];
  dv[2] = pkmn->dv[2];
  dv[3] = pkmn->dv[3];
  dvChanged();

  clearMoves();

  for(auto move : pkmn->moves) {
    auto m = new PokemonMove;
    m->pp = move->pp;
    m->ppUp = move->ppUp;
    m->moveID = move->moveID;
    moves.append(m);
  }

  movesChanged();

  type2Explicit = false;
  type2ExplicitChanged();
}

PokemonDBEntry* PokemonBox::toData()
{
  return PokemonDB::ind.value(QString::number(species), nullptr);
}

int PokemonBox::movesCount()
{
  return moves.size();
}

int PokemonBox::movesMax()
{
  return maxMoves;
}

PokemonMove* PokemonBox::movesAt(int ind)
{
  return moves.at(ind);
}

void PokemonBox::movesSwap(int from, int to)
{
  auto eFrom = moves.at(from);
  auto eTo = moves.at(to);

  moves.replace(from, eTo);
  moves.replace(to, eFrom);

  movesChanged();
}

void PokemonBox::movesRemove(int ind)
{
  // There has to be 1 move
  if(moves.size() <= 1)
    return;

  delete moves.at(ind);
  moves.removeAt(ind);
  movesChanged();
}

void PokemonBox::movesNew()
{
  if(moves.size() >= maxMoves)
    return;

  moves.append(new PokemonMove);
  movesChanged();
}

int PokemonBox::dvCount()
{
  return maxDV;
}

int PokemonBox::dvAt(int ind)
{
  return dv[ind];
}

void PokemonBox::dvSet(int ind, int val)
{
  dv[ind] = val;
  dvChanged();
}

void PokemonBox::dvSwap(int from, int to)
{
  auto eFrom = dv[from];
  auto eTo = dv[to];

  dv[from] = eTo;
  dv[to] = eFrom;

  dvChanged();
}

var16 PokemonBox::atkStat()
{
  return nonHpStat(PokemonStats::Attack);
}

var16 PokemonBox::defStat()
{
  return nonHpStat(PokemonStats::Defense);
}

var16 PokemonBox::spdStat()
{
  return nonHpStat(PokemonStats::Speed);
}

var16 PokemonBox::spStat()
{
  return nonHpStat(PokemonStats::Special);
}
