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
#include "../../../db/starterPokemon.h"
#include "../../../db/types.h"

#include <QtMath>
#include <QRandomGenerator>

QRandomGenerator* rnd = QRandomGenerator::global();

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

MoveEntry* PokemonMove::toMove()
{
  return Moves::ind->value(QString::number(moveID), nullptr);
}

void PokemonMove::randomize()
{
  var8 moveListSize = Moves::moves->size();

  for(var8 i = 0; i < 4; i++) {
    MoveEntry* moveData;

    do
      moveData = Moves::ind->value(
            QString::number(rnd->bounded(0, moveListSize)));
    while(moveData != nullptr && moveData->glitch != true);

    moveID = moveData->ind;

    if(moveData->pp)
      pp = *moveData->pp;

    ppUp = rnd->bounded(1, 3+1);
  }
}

void PokemonMove::maxPpUp()
{
  ppUp = 3;
}

bool PokemonMove::isMaxPP()
{
  if(toMove() == nullptr || !toMove()->pp)
    return false;

  return pp >= *toMove()->pp;
}

bool PokemonMove::isMaxPpUps()
{
  return ppUp >= 3;
}

PokemonBox::PokemonBox(SaveFile* saveFile,
                       var16 startOffset,
                       var16 nicknameStartOffset,
                       var16 otNameStartOffset,
                       var8 index,
                       var8 recordSize)
{
  moves = new QVector<PokemonMove*>();

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
  delete moves;
}

PokemonBox* PokemonBox::newPokemon(PokemonRandom list, SaveFile* saveFile)
{
  PokemonEntry* pkmnData;

  if(list == PokemonRandom::Random_All) {
    auto listSize = Pokemon::pokemon->size();
    var8 ind = rnd->bounded(0, listSize);
    pkmnData = Pokemon::pokemon->at(ind);
  }
  else if(list == PokemonRandom::Random_Pokedex) {
    var8 dex = rnd->bounded(1, 151+1);
    pkmnData = Pokemon::ind->value("dex" + QString::number(dex));
  }
  else if(list == PokemonRandom::Random_Starters)
    pkmnData = StarterPokemon::randomAnyStarter();
  else
    pkmnData = StarterPokemon::random3Starter();

  return newPokemon(pkmnData, saveFile);
}

PokemonBox* PokemonBox::newPokemon(PokemonEntry* pkmnData, SaveFile* saveFile)
{
  auto pkmn = new PokemonBox();

  pkmn->species = pkmnData->ind;
  pkmn->level = 5;
  pkmn->reRollDVs();
  pkmn->nickname = pkmnData->name;

  if(saveFile != nullptr)
    pkmn->changeTrade(true, saveFile);
  else
    pkmn->changeTrade();

  pkmn->resetPokemon();
  pkmn->update(true, true);

  return pkmn;
}

SaveFileIterator* PokemonBox::load(SaveFile* saveFile,
                      var16 startOffset,
                      var16 nicknameStartOffset,
                      var16 otNameStartOffset,
                      var8 index,
                      var8 recordSize)
{
  if(saveFile == nullptr) {
    reset();
    return nullptr;
  }

  // Calculate record offset
  var16 offset = (recordSize * index) + startOffset;

  auto toolset = saveFile->toolset;
  auto it = saveFile->iterator()->offsetTo(offset);

  species = it->getByte();
  hp = it->getWord();
  level = it->getByte();

  status = it->getByte();

  type1 = it->getByte();
  type2 = it->getByte();

  // Don't duplicate type 1 to type 2, fill type 2 only if it's different
  // Also mark if it was explicitly marked no in-game
  if (type2 == 0xFF) {
    type2Explicit = true;
  } else if (type1 == type2) {
    type2 = 0xFF;
  }

  catchRate = it->getByte();

  // Save offset to restore later
  it->push();

  // Temporarily save moves for later
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

  // Exp is 3 bytes so it's a bit tricky
  auto expRaw = it->getRange(3);
  exp = expRaw[0];
  exp <<= 8;
  exp |= expRaw[1];
  exp <<= 8;
  exp |= expRaw[2];

  hpExp = it->getWord();
  atkExp = it->getWord();
  defExp = it->getWord();
  spdExp = it->getWord();
  spExp = it->getWord();

  var16 dvTotal = it->getWord();
  dv[(var8)PokemonStats::Attack] = (dvTotal & 0xF000) >> 12;
  dv[(var8)PokemonStats::Defense] = (dvTotal & 0x0F00) >> 8;
  dv[(var8)PokemonStats::Speed] = (dvTotal & 0x00F0) >> 4;
  dv[(var8)PokemonStats::Special] = dvTotal & 0x000F;

  it->push();

  // Next gather PP
  QVector<var8> ppList;
  for (var8 i = 0; i < 4; i++) {
    var8 ppListEntry = it->getByte();
    if(ppListEntry == 0)
      break;

    ppList.append(ppListEntry);
  }

  // Combine together in moves
  moves->clear();
  for (var8 i = 0; i < moveIDList.size(); i++) {
    var8 moveID = moveIDList.at(i);
    var8 pp = ppList[i];
    moves->append(new PokemonMove(
                    moveID,
                    pp & 0b00111111,
                    (pp & 0b11000000) >> 6
                    ));
  }

  // Restore back to before PP and move past PP
  it->pop()->offsetBy(0x4);

  // Now we must gather the OT names and Pokemon names whihc were poorly
  // implemented in sometimes arbitrary spots outside of the data sructure
  var16 otNameOffset = (index * 0xB) + otNameStartOffset;
  otName = toolset->getStr(otNameOffset, 0xB, 7+1);

  var16 nicknameOffset = (index * 0xB) + nicknameStartOffset;
  nickname = toolset->getStr(nicknameOffset, 0xB, 10);

  // Save the iterator to be picked up by sub-class if present
  return this->it = it;
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
  if(speciesStartOffset < 0) {
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
  for(var8 i = 0; i < moves->size(); i++) {
    it->setByte(moves->at(i)->moveID);
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
  for (var8 i = 0; i < moves->size(); i++) {
    var8 ppCombined = (moves->at(i)->ppUp << 6) | moves->at(i)->pp;
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
  hp = 0;
  level = 0;
  status = 0;
  type1 = 0;
  type2 = 0;
  catchRate = 0;
  otID = 0;
  exp = 0;
  hpExp = 0;
  atkExp = 0;
  defExp = 0;
  spdExp = 0;
  spExp = 0;
  otName = "";
  nickname = "";

  dv[0] = 0;
  dv[1] = 0;
  dv[2] = 0;
  dv[3] = 0;

  clearMoves();

  type2Explicit = false;
  it = nullptr;
}

void PokemonBox::randomize()
{
  // Generate a random level 5 Pokemon from the pokedex
  // Bump it's level up to a random value
  // Give it a random name, otName, and otID
  // Then fix all of it's types, exp, stats, etc.. to be game accurate
  auto pkmn = PokemonBox::newPokemon(PokemonRandom::Random_Pokedex);
  pkmn->level = rnd->bounded(1, 100+1);
  changeTrade();
  update(true, true);

  // Delete it's moves and re-create 4 new non-glitch random moves
  randomizeMoves();
}

void PokemonBox::clearMoves()
{
  for(auto move : *moves)
    delete move;

  moves->clear();
}

// Is this a valid Pokemon? (Is it even in the Pokedex?)
// If not returns false, otherwise returns Pokemon Record
PokemonEntry* PokemonBox::isValid()
{
  // Get Pokemon Record
  // The Pokemon Array is organized by species ID with 1 top entry missing
  // thus offset by 1 accordingly
  auto record = Pokemon::ind->value(QString::number(species), nullptr);

  // Check it's a valid Pokemon (not glitch)
  if(record == nullptr || record->glitch || !(record->pokedex))
    return nullptr;

  return record;
}

var32 PokemonBox::levelToExp(svar8 level)
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
}

var8 PokemonBox::hpDV()
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

var16 PokemonBox::hpStat()
{
  auto record = isValid();

  // Proceed only if it's valid
  if(record == nullptr || !(record->baseHp))
    return 1;

  return qFloor((((*record->baseHp + hpDV())*2+qFloor(qFloor(qSqrt(hpExp))/4))*level)/100) + level + 10;
}

var16 PokemonBox::nonHpStat(PokemonStats stat)
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

void PokemonBox::update(bool resetHp, bool resetExp)
{
  auto record = isValid();
  if(record == nullptr)
    return;

  if(resetHp)
    hp = hpStat();

  if(record->toType1)
    type1 = (*record).toType1->ind;

  if(record->toType2)
    type2 = (*record).toType2->ind;

  if(type1 == type2)
    type2 = 0xFF;

  if(record->catchRate)
    catchRate = *record->catchRate;

  if(resetExp)
    exp = levelToExp();
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
  status = 0;

  for(auto move : *moves) {
    auto moveData = move->toMove();

    if(moveData->pp)
      move->pp = (*moveData->pp);
  }
}

bool PokemonBox::hasNickname()
{
  return isValid()->name == nickname;
}

bool PokemonBox::hasTradeStatus(SaveFile* saveFile)
{
  auto pl = saveFile->dataExpanded->player->basics;
  return pl->playerName == otName && pl->playerID == otID;
}

void PokemonBox::changeName(bool removeNickname)
{
  if(!removeNickname)
    nickname = Names::randomName();
  else if(removeNickname)
    nickname = toData()->name;
}

void PokemonBox::changeOtData(bool removeOtData, SaveFile* saveFile)
{
  auto rnd = QRandomGenerator::global();

  if(!removeOtData) {
    otName = Names::randomName();
    otID = rnd->bounded(0x0000, 0xFFFF);
  }
  else if(removeOtData && saveFile != nullptr) {
    otName = saveFile->dataExpanded->player->basics->playerName;
    otID = saveFile->dataExpanded->player->basics->playerID;
  }
}

void PokemonBox::changeTrade(bool removeTradeStatus, SaveFile* saveFile)
{
  changeName(removeTradeStatus);
  changeOtData(removeTradeStatus, saveFile);
}

bool PokemonBox::hasEvolution()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  if(record->evolution->size() == 0)
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
  auto rnd = QRandomGenerator::global();

  if(!hasEvolution())
    return;

  // Does it have a nickname before evolution
  bool nickStatus = hasNickname();

  // For Eevee evolutions, randomly pick one
  if(record->evolution->size() > 1) {
    var8 ind = rnd->bounded(0, record->evolution->size());
    species = record->evolution->at(ind)->toEvolution->ind;
  }
  else
    species = record->evolution->at(0)->toEvolution->ind;

  // Update name if no nickname
  if(!nickStatus)
    changeName(true);

  // Update all stats
  update(true, true);
}

void PokemonBox::deEvolve()
{
  auto record = isValid();

  if(!hasDeEvolution())
    return;

  // Does it have a nickname before de-evolution
  bool nickStatus = hasNickname();

  species = record->toDeEvolution->ind;

  // Update name if no nickname
  if(!nickStatus)
    changeName(true);

  // Update all stats
  update(true, true);

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

  for(auto move : *moves)
    if(!move->isMaxPP()) ret = false;

  return ret;
}

bool PokemonBox::isMaxPpUps()
{
  bool ret = true;

  for(auto move : *moves)
    if(!move->isMaxPpUps()) ret = false;

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
  update(true, true);
}

void PokemonBox::maxPpUps()
{
  for(auto move : *moves)
    move->maxPpUp();
}

void PokemonBox::maxDVs()
{
  for(var8 i = 0; i < 4; i++)
    dv[i] = 15;
}

void PokemonBox::reRollDVs()
{
  auto rnd = QRandomGenerator::global();

  for(var8 i = 0; i < 4; i++)
    dv[i] = rnd->bounded(0, 15+1);
}

void PokemonBox::maxEVs()
{
  hpExp = 0xFFFF;
  atkExp = 0xFFFF;
  defExp = 0xFFFF;
  spdExp = 0xFFFF;
  spExp = 0xFFFF;
  update(true);
}

void PokemonBox::resetEVs()
{
  hpExp = 0;
  atkExp = 0;
  defExp = 0;
  spdExp = 0;
  spExp = 0;

  update(true);
}

void PokemonBox::maxOut()
{
  maxLevel();
  maxPpUps();
  maxEVs();
  maxDVs();
  heal();

  update(true);
}

void PokemonBox::randomizeMoves()
{
 for(auto move : *moves)
    delete move;

  moves->clear();

  for(var8 i = 0; i < 4; i++) {
    moves->append(new PokemonMove);
    moves->at(i)->randomize();
  }
}

bool PokemonBox::isPokemonReset()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  bool movesReset = true;

  for(var8 i = 0; i < moves->size(); i++) {
    auto move = moves->at(i);

    if(move->toMove() == nullptr)
      movesReset = false;
    if(!movesReset)
      break;

    if(move->moveID != record->toInitial->at(i)->ind)
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

  auto record = isValid();
  if(record == nullptr)
    return;

  clearMoves();

  for(auto moveData : *record->toInitial)
    moves->append(new PokemonMove(moveData->ind, *moveData->pp, 0));

  resetEVs();
  heal();
  update(true, true);
}

PokemonEntry* PokemonBox::toData()
{
  return Pokemon::ind->value(QString::number(species), nullptr);
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

// Not to be used
void PokemonBox::load(SaveFile* saveFile) {Q_UNUSED(saveFile)}
void PokemonBox::save(SaveFile* saveFile) {Q_UNUSED(saveFile)}
