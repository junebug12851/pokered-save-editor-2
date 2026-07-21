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
 * @file pokemonbox.cpp
 * @brief Implementation of PokemonBox and PokemonMove -- the central Pokemon
 *        record: load/save, stat computation, validity, evolution, shininess,
 *        nature, and constrained randomization. See pokemonbox.h for the API.
 */
#include "pokemonbox.h"
#include "../../qmlownership.h"
#include "../savefileexpanded.h"
#include "../player/player.h"
#include "../player/playerbasics.h"
#include "../../savefile.h"
#include "../../savefiletoolset.h"
#include "../../savefileiterator.h"
#include <pse-db/pokemon.h>
#include <pse-db/moves.h>
#include <pse-db/names.h>
#include <pse-db/starterPokemon.h>
#include <pse-db/types.h>
#include <pse-common/random.h>
//#include "../../../../../ui/window/mainwindow.h"

#include <QtMath>
#include <QQmlEngine>
#include <QDebug>

PokemonMove::PokemonMove(PokemonBox* parentMon, var8 move, var8 pp, var8 ppUp)
{
  // Own this move slot in C++ so QML's GC can never free it. (Realises the old
  // @TODO; QQmlEngine::setObjectOwnership is static and needs no engine.)
  // parentMon is stored as a plain member, NOT a QObject parent, so this object
  // is parentless and would otherwise default to JavaScriptOwnership the moment
  // it's handed to QML (e.g. via movesAt()). See qmlownership.h / qt6-patterns.md.
  QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

  this->parentMon = parentMon;

  connect(this, &PokemonMove::moveIDChanged, this, &PokemonMove::ppCapChanged);
  connect(this, &PokemonMove::ppChanged, this, &PokemonMove::ppCapChanged);
  connect(this, &PokemonMove::ppUpChanged, this, &PokemonMove::ppCapChanged);
  connect(this, &PokemonMove::moveIDChanged, this, &PokemonMove::onMoveIdChanged);
  connect(this, &PokemonMove::ppUpChanged, this, &PokemonMove::onMoveIdChanged);

  this->moveID = move;
  this->pp = pp;
  this->ppUp = ppUp;

  if(move == 0) {
    randomize();
    // A freshly-created empty move slot starts with 0 PP-Ups. randomize() (above)
    // assigns a RANDOM 0-3 ppUp; this resets it so a brand-new move is clean. The
    // original code wrote `ppUp = 0`, which assigned the constructor PARAMETER
    // (shadowing the member) -- a no-op, so new moves silently kept a random ppUp.
    // Now writes the member. (clang-analyzer-deadcode.DeadStores; confirmed intended
    // with project leadership 2026-06-22.) Set directly (no ppUpChanged()) -- we're in the
    // ctor, matching the plain member writes above; nothing is connected yet.
    this->ppUp = 0;
  }
}

MoveDBEntry* PokemonMove::toMove()
{
  return MovesDB::inst()->getIndAt(QString::number(moveID));
}

void PokemonMove::randomize()
{
  var8 moveListSize = MovesDB::inst()->getStoreSize();

  for(var8 i = 0; i < 4; i++) {
    MoveDBEntry* moveData;

    do
      moveData = const_cast<MoveDBEntry*>(MovesDB::inst()->getIndAt(
            QString::number(Random::inst()->rangeExclusive(0, moveListSize))));
    while(moveData == nullptr || moveData->glitch == true);

    moveID = moveData->ind;
    moveIDChanged();

    ppUp = Random::inst()->rangeInclusive(0, 3);
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

void PokemonMove::raisePpUp()
{
  if(ppUp < 3)
    ppUp++;
  ppUpChanged();
}

void PokemonMove::lowerPpUp()
{
  if(ppUp > 0)
    ppUp--;
  ppUpChanged();
}

void PokemonMove::resetPpUp()
{
  ppUp = 0;
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

bool PokemonMove::isInvalid()
{
  return moveID == 0 || toMove() == nullptr || toMove()->glitch;
}

QString PokemonMove::moveType()
{
  if(moveID == 0)
    return "";
  else if(isInvalid() || toMove()->type == "")
    return "Glitch";
  else
    return toMove()->toType->readable;
}

void PokemonMove::onMoveIdChanged()
{
  if(!isInvalid() && pp > getMaxPP()) {
    pp = getMaxPP();
    ppChanged();
  }
}

QVector<int> PokemonMove::allValidMoves()
{
  QVector<int> ret;

  if(!parentMon->isValidBool())
    return ret;

  auto monData = parentMon->toData();

  for(auto el : monData->toInitial) {
    if(!ret.contains(el->ind))
      ret.append(el->ind);
  }

  for(auto el : monData->moves) {
    if(el->toMove != nullptr && !ret.contains(el->toMove->ind))
      ret.append(el->toMove->ind);
  }

  for(auto el : monData->toTmHmMove) {
    if(!ret.contains(el->ind))
      ret.append(el->ind);
  }

  return ret;
}

QVector<int> PokemonMove::validMovesLeft()
{
  QVector<int> ret = allValidMoves();

  if(!parentMon->isValidBool())
    return ret;

  for(int i = 0 ; i < 4; i++) {
    if(parentMon->moves[i]->moveID > 0)
      ret.removeAll(parentMon->moves[i]->moveID);
  }

  return ret;
}

bool PokemonMove::isDuplicateMove()
{
  int count = 0;

  for(int i = 0 ; i < 4; i++) {
    if(parentMon->moves[i]->moveID == this->moveID)
      count++;
  }

  return count > 1;
}

void PokemonMove::restorePP()
{
  var8 maxPP = getMaxPP();
  if(maxPP == 0)
    return;

  pp = maxPP;
  ppChanged();
}

void PokemonMove::changeMove(int move, int pp, int ppUp)
{
  this->moveID = move;
  moveIDChanged();

  this->pp = pp;
  ppChanged();

  this->ppUp = ppUp;
  ppUpChanged();
}

void PokemonMove::correctMove()
{
  // Skip if pokemon is a glitch mon
  if(!parentMon->isValidBool())
    return;

  // Count number of non-zero move rows
  int rowCount = 0;

  for(int i = 0; i < 4; i++) {
    if(parentMon->moves[i]->moveID > 0)
      rowCount++;
  }

  // Stop here if this move is zero and there are other moves which aren't
  if(rowCount >= 1 && moveID <= 0)
    return;

  auto validMoves = allValidMoves();

  if(!validMoves.contains(moveID) || isDuplicateMove()) {
    if(rowCount <= 1) {
      auto validMovesLeftList = validMovesLeft();
      moveID = validMovesLeftList.at(0);
      moveIDChanged();
      return;
    }

    moveID = 0;
    moveIDChanged();
  }
}

PokemonBox::PokemonBox(SaveFile* saveFile,
                       var16 startOffset,
                       var16 nicknameStartOffset,
                       var16 otNameStartOffset,
                       var8 index,
                       var8 recordSize)
{
  // Own this mon in C++ for its container's lifetime so QML's GC can never free
  // it. (Realises the old @TODO: it wanted MainWindow::engine, but
  // QQmlEngine::setObjectOwnership is static and needs no engine.) Every mon --
  // loaded from a save OR created fresh via this same default-arg ctor
  // (new PokemonBox() in newPokemon()) -- is now self-protecting from birth.
  // This closes the QML-GC use-after-free at the source instead of relying only
  // on qmlCppOwned() at each accessor, which protects a mon only once it's been
  // handed out and misses any other exposure path (symptom: open a stored or
  // freshly-created mon's editor, back out, re-open -> intermittent crash in
  // PokemonStorageModel::hasChecked()/data() reading a GC'd mon). The container
  // still owns lifetime and frees mons via deleteLater(), so CppOwnership here
  // introduces no leak/double-free. See qmlownership.h / qt6-patterns.md.
  QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

  for(int i = 0; i < 4; i++) {
    moves[i] = new PokemonMove(this);

    //connect(moves[i], &PokemonMove::moveIDChanged, this, &PokemonBox::movesChanged);
    connect(moves[i], &PokemonMove::ppCapChanged, this, &PokemonBox::movesChanged);
  }

  connect(this, &PokemonBox::speciesChanged, this, &PokemonBox::expRangeChanged);
  connect(this, &PokemonBox::levelChanged, this, &PokemonBox::expRangeChanged);

  connect(this, &PokemonBox::speciesChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::hpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::levelChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::hpExpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::atkExpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::defExpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::spdExpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::spExpChanged, this, &PokemonBox::statChanged);
  connect(this, &PokemonBox::dvChanged, this, &PokemonBox::statChanged);

  connect(this, &PokemonBox::statChanged, this, &PokemonBox::healedChanged);
  connect(this, &PokemonBox::movesChanged, this, &PokemonBox::healedChanged);

  connect(this, &PokemonBox::nicknameChanged, this, &PokemonBox::hasNicknameChanged);
  connect(this, &PokemonBox::speciesChanged, this, &PokemonBox::hasNicknameChanged);

  connect(this, &PokemonBox::hpExpChanged, this, &PokemonBox::evChanged);
  connect(this, &PokemonBox::atkExpChanged, this, &PokemonBox::evChanged);
  connect(this, &PokemonBox::defExpChanged, this, &PokemonBox::evChanged);
  connect(this, &PokemonBox::spdExpChanged, this, &PokemonBox::evChanged);
  connect(this, &PokemonBox::spExpChanged, this, &PokemonBox::evChanged);

  connect(this, &PokemonBox::healedChanged, this, &PokemonBox::pokemonResetChanged);

  load(saveFile,
       startOffset,
       nicknameStartOffset,
       otNameStartOffset,
       index,
       recordSize);
}

PokemonBox::~PokemonBox() {
  for(int i = 0; i < 4; i++) {
    moves[i]->deleteLater();
  }
}

PokemonBox* PokemonBox::newPokemon(PokemonRandom::PokemonRandom_ list, PlayerBasics* basics)
{
  PokemonDBEntry* pkmnData;

  if(list == PokemonRandom::Random_All) {
    auto listSize = PokemonDB::inst()->getStoreSize();
    var8 ind = Random::inst()->rangeExclusive(0, listSize);
    pkmnData = PokemonDB::inst()->getStoreAt(ind);
  }
  else if(list == PokemonRandom::Random_Pokedex) {
    // The "dexN" keys are 0-based (dex0 = Bulbasaur .. dex150 = Mew; there is no
    // dex151). rangeExclusive(0, 151) -> [0,150] covers all 151 species. Was
    // rangeExclusive(1, ...), which silently made Bulbasaur (dex0) unreachable.
    var8 dex = Random::inst()->rangeExclusive(0, pokemonDexCount);
    pkmnData = PokemonDB::inst()->getIndAt("dex" + QString::number(dex));
  }
  else if(list == PokemonRandom::Random_Starters)
    pkmnData = StarterPokemonDB::inst()->randomAnyStarter();
  else
    pkmnData = StarterPokemonDB::inst()->random3Starter();

  return newPokemon(pkmnData, basics);
}

PokemonBox* PokemonBox::newPokemon(PokemonDBEntry* pkmnData, PlayerBasics* basics)
{
  auto pkmn = new PokemonBox();

  pkmn->species = pkmnData->ind;
  pkmn->level = Random::inst()->rangeInclusive(5, 8);
  pkmn->reRollDVs();

  // Randomly give a nickanme or not
  bool noNick = Random::inst()->flipCoin();
  pkmn->changeName(noNick);

  // 10% chance of it being a traded pokemon
  bool isTrade = Random::inst()->chanceSuccess(10);

  if(basics != nullptr && !isTrade)
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

  // Normalise type 2 to the internal single-type sentinel (0xFF = "None") while
  // preserving on-disk fidelity (see the "single truth" note in save()):
  //   * a literal 0xFF (only from a hacked/glitch save -- the game never writes
  //     it) is kept as-is AND flagged type2Explicit so save() writes 0xFF back;
  //   * the game's real single-type form (type2 == type1, a duplicate) collapses
  //     to the 0xFF sentinel with type2Explicit left false, so save() writes the
  //     duplicate back. A genuine dual type is stored verbatim.
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
  for (var8 i = 0; i < moveIDList.size(); i++) {
    var8 moveID = moveIDList.at(i);
    var8 pp = ppList[i];
    moves[i]->moveID = moveID;
    moves[i]->moveIDChanged();

    moves[i]->pp = pp & 0b00111111;
    moves[i]->ppChanged();

    moves[i]->ppUp = (pp & 0b11000000) >> 6;
    moves[i]->ppUpChanged();
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

  // Type 2 storage -- the resolved "single truth" (grounded in the pokered
  // disassembly; see notes/reference/gen1-knowledge.md "Single-type storage"):
  //   * the game stores a SINGLE type as a duplicate of type 1 (base_stats/*.asm,
  //     e.g. Charmander `db FIRE, FIRE`), and 0xFF is not a valid type at all
  //     (type_constants.asm runs $00..$1A). So the CANONICAL written form of a
  //     single type is duplicate-of-type1 -- that is what the editor writes.
  //   * Byte fidelity still rules for LOADED saves: a mon read with a literal
  //     0xFF (only ever from a hacked/glitch save) is written back as 0xFF
  //     unchanged (type2Explicit); a mon read as a duplicate is written back as
  //     the duplicate. A type byte changes only when explicitly edited.
  // Internally a single type is held as type2 == 0xFF ("None"); type2Explicit
  // marks that the 0xFF was literal-on-disk (preserve it) vs. editor-implied
  // (write the duplicate). Any editor (re)generation clears type2Explicit, so a
  // freshly generated/corrected single type takes the canonical duplicate form.
  if (type2Explicit) {
    it->setByte(type2);           // preserve a literally-loaded 0xFF (byte fidelity)
  } else if (type2 == 0xFF) {
    it->setByte(type1);           // canonical single type -> duplicate-of-type1
  } else {
    it->setByte(type2);           // genuine dual type
  }

  it->setByte(catchRate);

  it->push();
  for(var8 i = 0; i < 4; i++) {
    it->setByte(moves[i]->moveID);
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
  for (var8 i = 0; i < 4; i++) {
    var8 ppCombined = (moves[i]->ppUp << 6) | moves[i]->pp;
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
  pkmn->deleteLater();

  level = Random::inst()->rangeInclusive(5, pokemonLevelMax);
  levelChanged();

  atkExp = Random::inst()->rangeInclusive(0, 0xFFFF);
  atkExpChanged();

  defExp = Random::inst()->rangeInclusive(0, 0xFFFF);
  defExpChanged();

  spdExp = Random::inst()->rangeInclusive(0, 0xFFFF);
  spdExpChanged();

  spExp = Random::inst()->rangeInclusive(0, 0xFFFF);
  spExpChanged();

  hpExp = Random::inst()->rangeInclusive(0, 0xFFFF);
  hpExpChanged();

  // Delete it's moves and re-create 4 new non-glitch random moves
  randomizeMoves();

  // Make the pokemon a non-trade pokemon
  changeTrade(true, basics);

  // 50/50 chance of not having a nickname
  // If true, removes nickname
  // If false, assigns random nickname
  bool noNick = Random::inst()->flipCoin();
  changeName(noNick);

  // This is where we make the Pokemon completely game accurate
  update(true, true, true, true);

  // Heal the Pokemon
  heal();

  // This is where we give the Pokemon whacky types for fun
  // We have to do this after all the code above otherwise it'll be re-corrected
  // to be game accurate
  auto type1 = TypesDB::inst()->getStoreAt(Random::inst()->rangeExclusive(0, TypesDB::inst()->getStoreSize()));
  TypeDBEntry* type2 = nullptr;

  // 25% chance of type 2
  bool hasType2 = Random::inst()->chanceSuccess(25);
  if(hasType2) {
    type2 = TypesDB::inst()->getStoreAt(Random::inst()->rangeExclusive(0, TypesDB::inst()->getStoreSize()));

    if(type1->ind == type2->ind)
      type2 = nullptr;
  }

  this->type1 = type1->ind;
  type1Changed();

  if(type2 != nullptr)
    this->type2 = type2->ind;
  else
    this->type2 = 0xFF;
  type2Changed();

  // Editor-generated typing is canonical (not preserved load bytes): a single
  // type must serialise as duplicate-of-type1, the game's own form (see the note
  // in save() and notes/reference/gen1-knowledge.md). Clear the load-fidelity
  // flag so save() writes the duplicate, never a stray 0xFF. (randomize() also
  // reset()s at the top, but keep the invariant local + explicit here.)
  type2Explicit = false;
  type2ExplicitChanged();
}

void PokemonBox::clearMoves()
{
  for(int i = 0; i < 4; i++) {
    moves[i]->moveID = 0;
    moves[i]->moveIDChanged();

    moves[i]->pp = 0;
    moves[i]->ppChanged();

    moves[i]->ppUp = 0;
    moves[i]->ppUpChanged();
  }

  movesChanged();
}

// Is this a valid Pokemon? (Is it even in the Pokedex?)
// If not returns false, otherwise returns Pokemon Record
PokemonDBEntry* PokemonBox::isValid()
{
  // Get Pokemon Record
  // The Pokemon Array is organized by species ID with 1 top entry missing
  // thus offset by 1 accordingly
  auto record = PokemonDB::inst()->getIndAt(QString::number(species));

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

  // Return percentage. Both operands are var32, so the previous `curExp / expEnd`
  // was an INTEGER division truncated to 0 (or 1 at the very top of the level)
  // before being widened to the float return -- the fractional percent was always
  // lost. Divide in floating point, and guard a zero-width range (degenerate exp
  // data) against divide-by-zero. Found by clang-tidy (bugprone-integer-division +
  // clang-analyzer-core.DivideZero); see notes/reference/fix-patterns.md.
  if(expEnd == 0)
    return 0;
  return static_cast<float>(curExp) / static_cast<float>(expEnd);
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

  int baseStat = 0;
  int dvLocal = 0;
  int evLocal = 0;

  if(stat == PokemonStats::Attack) {
    baseStat = *record->baseAttack;
    dvLocal = dv[PokemonStats::Attack];
    evLocal = atkExp;
  }
  else if(stat == PokemonStats::Defense) {
    baseStat = *record->baseDefense;
    dvLocal = dv[PokemonStats::Defense];
    evLocal = defExp;
  }
  else if(stat == PokemonStats::Speed) {
    baseStat = *record->baseSpeed;
    dvLocal = dv[PokemonStats::Speed];
    evLocal = spdExp;
  }
  else if(stat == PokemonStats::Special) {
    baseStat = *record->baseSpecial;
    dvLocal = dv[PokemonStats::Special];
    evLocal = spExp;
  }

  return qFloor((((baseStat+dvLocal)*2+qFloor(qFloor(qSqrt(evLocal))/4))*level)/100) + 5;
}

void PokemonBox::update(bool resetHp,
                        bool resetExp,
                        bool resetType,
                        bool resetCatchRate,
                        bool correctMoves)
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

  // Only (re)derive type2 when explicitly asked. The previous code's bare `else`
  // ran on EVERY update() called with resetType=false and overwrote type2 with
  // type1 -- silently dropping a dual-type mon's second type (reachable via
  // maxLevel()/maxEVs()/resetEVs()/reRollEVs()/manualLevelChanged()). Now type2
  // is left untouched unless resetType is set.
  if(resetType) {
    if(record->toType2)
      type2 = (*record).toType2->ind;
    else if(record->toType1)   // guard toType1 (matches the resetType-&&-toType1
      type2 = (*record).toType1->ind;  // check above) -- avoid a null deref on a
                                       // record with neither type resolved.
                                       // Found by clang-analyzer-core.NullDereference.

    // A single type (no distinct second type) is stored internally as 0xFF.
    if(type1 == type2)
      type2 = 0xFF;

    type2Changed();

    // A DB-derived (re)typing is canonical, so drop any preserved load-fidelity
    // 0xFF: a single type now serialises as duplicate-of-type1 (the game's form;
    // see the note in save()). Stops a mon that was loaded with a literal 0xFF
    // from re-writing 0xFF after its species/typing was reset here.
    type2Explicit = false;
    type2ExplicitChanged();
  }

  if(resetCatchRate && record->catchRate) {
    catchRate = *record->catchRate;
    catchRateChanged();
  }

  if(resetExp) {
    exp = levelToExp();
    expChanged();
  }

  if(correctMoves) {
    this->correctMoves();
    cleanupMoves();
  }
}

// Reset the typing to the species' DB defaults. Mirrors update()'s resetType block
// but standalone, so QML can call it without the unreliable multi-bool update() call
// (see the header note). No-ops on a glitch species (no DB record / unlinked type).
void PokemonBox::correctTypes()
{
  auto record = isValid();
  if(record == nullptr || !record->toType1)
    return;

  type1 = record->toType1->ind;
  type1Changed();

  if(record->toType2)
    type2 = record->toType2->ind;
  else
    type2 = record->toType1->ind;

  // A single type (no distinct second type) is stored internally as 0xFF.
  if(type1 == type2)
    type2 = 0xFF;

  type2Changed();

  // This is an editor-driven correction, so the result is canonical, not loaded
  // bytes: clear the load-fidelity flag so a single type serialises as
  // duplicate-of-type1 (the game's form; see the note in save()) rather than a
  // stale 0xFF carried over from a hacked save that literally stored 0xFF.
  type2Explicit = false;
  type2ExplicitChanged();
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

  for(int i = 0; i < 4; i++)
    moves[i]->restorePP();
}

bool PokemonBox::hasNickname()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  return record->name != nickname;
}

bool PokemonBox::hasTradeStatus(PlayerBasics* basics)
{
  return basics->playerName != otName || basics->playerID != otID;
}

void PokemonBox::changeName(bool removeNickname)
{
  if(!removeNickname)
    nickname = Names::inst()->pokemon()->randomExample();
  else if(removeNickname)
    nickname = toData()->name;

  nicknameChanged();
}

void PokemonBox::changeOtData(bool removeOtData, PlayerBasics* basics)
{
  // Randomize OT (give it "traded" status): always a real change, always emit.
  if(!removeOtData) {
    otName = Names::inst()->player()->randomExample();
    otID = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
    otNameChanged();
    otIDChanged();
    return;
  }

  // Adopt the player's OT (remove "traded" status). Need the player's data.
  if(basics == nullptr)
    return;

  // Idempotent: only touch a field (and emit) if it actually differs. Keeps the
  // owned-mon OT sync from firing a storm of no-op change signals, and keeps us
  // from rewriting OT bytes that didn't need to change.
  if(otName != basics->playerName) {
    otName = basics->playerName;
    otNameChanged();
  }

  if(otID != basics->playerID) {
    otID = basics->playerID;
    otIDChanged();
  }
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
    var8 ind = Random::inst()->rangeExclusive(0, record->evolution.size());
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

  // Empty slots (moveID 0) hold no move and have no PP, so they must NOT count as
  // "not max PP" -- otherwise any mon with fewer than 4 moves can never read as
  // max-PP, and therefore never as isHealed() (a user-facing wrong result on the
  // heal indicator). Mirrors isMaxedOut()'s existing moveID>0 guard. (2026-06-08.)
  for(int i = 0; i < 4; i++)
    if(moves[i]->moveID > 0 && !moves[i]->isMaxPP())
      ret = false;

  return ret;
}

bool PokemonBox::isMaxPpUps()
{
  bool ret = true;

  // Same empty-slot guard as isMaxPP(): an empty slot has no PP-Ups to max.
  for(int i = 0; i < 4; i++)
    if(moves[i]->moveID > 0 && !moves[i]->isMaxPpUps())
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
  // "Minimum EVs" means ALL five stat-exp are zero (symmetric with isMaxEVs()'s
  // all-0xFFFF). Was `||` (true if ANY one was 0), which wrongly disabled the
  // "Reset EVs" UI action whenever a single stat-exp happened to be 0. (Fixed
  // 2026-06-08, project leadership-confirmed.)
  return atkExp == 0 &&
      defExp == 0 &&
      spdExp == 0 &&
      spExp == 0 &&
      hpExp == 0;
}

bool PokemonBox::isMaxDVs()
{
  bool ret = true;

  for(var8 i = 0; i < 4; i++)
    if(dv[i] < 15) ret = false;

  return ret;
}

bool PokemonBox::isMinDVs()
{
  bool ret = true;

  for(var8 i = 0; i < 4; i++)
    if(dv[i] > 0) ret = false;

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
  for(int i = 0; i < 4; i++)
    moves[i]->maxPpUp();
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
    dv[i] = Random::inst()->rangeInclusive(0, 15);

  dvChanged();
}

void PokemonBox::resetDVs()
{
  for(var8 i = 0; i < 4; i++)
    dv[i] = 0;

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

void PokemonBox::reRollEVs()
{
  hpExp = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
  hpExpChanged();

  atkExp = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
  atkExpChanged();

  defExp = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
  defExpChanged();

  spdExp = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
  spdExpChanged();

  spExp = Random::inst()->rangeInclusive(0x0000, 0xFFFF);
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
    moves[i]->randomize();
  }

  movesChanged();
}

bool PokemonBox::isPokemonReset()
{
  auto record = isValid();

  if(record == nullptr)
    return false;

  bool movesReset = true;

  // A reset mon (see resetPokemon()) carries exactly the species' initial moves,
  // each at base PP with 0 PP-Ups, and empty slots beyond them. The old loop
  // checked all four slots against toInitial.at(i) (out-of-range for species with
  // <4 initial moves -- saved only by the toMove()==null early-out, which also
  // forced "not reset"), and required isMaxPpUps() (3) when a reset mon actually
  // has 0 PP-Ups. Iterate only the real initial moves; require empty slots after.
  int initialCount = record->toInitial.size();
  if(initialCount > 4)
    initialCount = 4;

  for(int i = 0; i < 4; i++) {
    auto move = moves[i];

    // Slots past the species' initial-move list must be empty.
    if(i >= initialCount) {
      if(move->moveID != 0)
        movesReset = false;
      if(!movesReset)
        break;
      continue;
    }

    if(move->toMove() == nullptr)
      movesReset = false;
    if(!movesReset)
      break;

    if(move->moveID != record->toInitial.at(i)->ind)
      movesReset = false;
    if(move->ppUp != 0)       // resetPokemon leaves PP-Ups at 0
      movesReset = false;

    if(!movesReset)
      break;
  }

  // isHealed() (full HP + no status + max PP) is now correct for any move count
  // because isMaxPP() skips empty slots; PP/HP/status are covered there, so here
  // we only need the level, the initial-move match, and zeroed EVs.
  return level == 5 && movesReset && isMinEvs() && isHealed();
}

bool PokemonBox::isMaxedOut()
{
  if(level < 100)
    return false;

  for(int i = 0; i < 4; i++) {
    if(moves[i]->moveID > 0 && !moves[i]->isInvalid() && (!moves[i]->isMaxPP() || !moves[i]->isMaxPpUps()))
      return false;
  }

  if(atkExp < 0xFFFF || defExp < 0xFFFF || spdExp < 0xFFFF || spExp < 0xFFFF || hpExp < 0xFFFF)
    return false;

  for(int i = 0; i < 4; i++) {
    if(dv[i] < 15)
      return false;
  }

  // Stop here if pokemon is invalid and we got this far
  if(!isValidBool())
    return true;

  if(hp < hpStat())
    return false;

  if(exp < expLevelRangeEnd())
    return false;

  return true;
}

bool PokemonBox::isCorrected()
{
  auto record = isValid();
  if(record == nullptr)
    return true;

  if(hpStat() != hp)
    return false;

  if(record->toType1 != nullptr) {
    if(record->toType1->ind != type1)
      return false;
  }

  // A mon is genuinely dual-type only when the record's second type really
  // differs from its first. The DB inconsistently stores single-type mons with
  // toType2 either null OR a duplicate of toType1; load()/update() collapse a
  // single type to the internal 0xFF sentinel. Accept EITHER 0xFF or the
  // duplicate (type2 == type1) as "corrected" for a single-type species.
  //
  // RESOLVED "single truth" (2026-07-09, from the pokered disassembly -- see
  // notes/reference/gen1-knowledge.md "Single-type storage"): the game's single-
  // type form is duplicate-of-type1 and 0xFF is not a valid type, so BOTH the
  // 0xFF sentinel and the duplicate are legitimate representations of the same
  // single-type mon that serialise to the identical canonical bytes. Neither is
  // "wrong", so tolerating both here is the intended final behaviour, not a
  // temporary patch.
  bool dualType = record->toType2 != nullptr &&
                  record->toType1 != nullptr &&
                  record->toType2->ind != record->toType1->ind;

  if(dualType) {
    if(record->toType2->ind != type2)
      return false;
  }
  else if(type2 != 0xFF && type2 != type1)
    return false;

  if(record->catchRate) {
    if(*record->catchRate != catchRate)
      return false;
  }

  if(levelToExp() != exp)
    return false;

  return true;
}

int PokemonBox::dexNum()
{
  auto record = isValid();
  if(record == nullptr)
    return -1;

  return *record->pokedex;
}

QString PokemonBox::speciesName()
{
  auto record = isValid();
  if(record == nullptr)
    return "";

  if(record->readable == "")
    return record->name;
  else
    return record->readable;
}

bool PokemonBox::isShiny()
{
  bool atkChk = dv[PokemonStats::Attack] & 2;
  bool defChk = dv[PokemonStats::Defense] == 0b1010;
  bool spdChk = dv[PokemonStats::Speed] == 0b1010;
  bool spChk = dv[PokemonStats::Special] == 0b1010;

  return atkChk && defChk && spdChk && spChk;
}

int PokemonBox::getNature()
{
  return exp % 25;
}

void PokemonBox::setNature(int nature)
{
  // Get current value
  var8 cur = exp % 25;

  // Get Level Ranges
  // We want to keep Pokemon in same level range if possible
  var32 min = expLevelRangeStart();
  var32 max = expLevelRangeEnd();

  // Stop here if this is the nature
  if(cur == nature)
    return;

  // Get offset to apply
  var8 offset = qAbs(nature - cur);

  // Add or subtract
  if(cur > nature)
    exp -= offset;
  else
    exp += offset;

  // Notify of change
  expChanged();

  // Stop here if invalid Pokemon
  if(!isValidBool())
    return;

  // Otherwise lets ensure the Pokemon is within the correct level range
  // If it's fallen below or risen above the max, offset by 25 to bring back
  // within level
  if(exp <= min) {
    exp += 25;
    expChanged();
  }
  else if(exp >= max) {
    exp -= 25;
    expChanged();
  }
}

void PokemonBox::cleanupMoves()
{
  QVector<PokemonMove*> movesNew;

  // First gather actual moves
  for(int i = 0; i < 4; i++) {
    if(moves[i]->moveID <= 0)
      continue;

    auto newMoveEl = new PokemonMove(
          this,
          moves[i]->moveID,
          moves[i]->pp,
          moves[i]->ppUp
          );

    movesNew.append(newMoveEl);
  }

  // Then clear out moves
  for(int i = 0; i < 4; i++) {
    moves[i]->moveID = 0;
    moves[i]->pp = 0;
    moves[i]->ppUp = 0;
  }

  // Then re-insert moves
  for(int i = 0; i < movesNew.size(); i++) {
    moves[i]->moveID = movesNew.at(i)->moveID;
    moves[i]->pp = movesNew.at(i)->pp;
    moves[i]->ppUp = movesNew.at(i)->ppUp;
  }

  // Then aknowledge changes
  for(int i = 0; i < 4; i++) {
    moves[i]->moveIDChanged();
    moves[i]->ppChanged();
    moves[i]->ppUpChanged();
  }
}

void PokemonBox::correctMoves()
{
  for(int i = 0; i < 4; i++)
    moves[i]->correctMove();
}

void PokemonBox::rollShiny()
{
  dv[PokemonStats::Defense] = 0b1010;
  dvChanged();

  dv[PokemonStats::Speed] = 0b1010;
  dvChanged();

  dv[PokemonStats::Special] = 0b1010;
  dvChanged();

  dv[PokemonStats::Attack] = Random::inst()->rangeInclusive(0, 15);
  dv[PokemonStats::Attack] |= 2;
  dvChanged();
}

void PokemonBox::rollNonShiny()
{
  reRollDVs();

  dv[PokemonStats::Attack] &= ~2;
  dvChanged();
}

void PokemonBox::makeShiny()
{
  // Since shinies have such specific DV's, it's easier just to roll a shiny
  // and set it's attack dv to the same attack as before only or'd with 2
  var8 tmpAtkDV = dv[PokemonStats::Attack];
  rollShiny();

  dv[PokemonStats::Attack] = tmpAtkDV | 2;
  dvChanged();
}

void PokemonBox::unmakeShiny()
{
  // Just remove bit #1, the most minimum way of eliminating it as a shiny
  dv[PokemonStats::Attack] &= ~2;
  dvChanged();
}

bool PokemonBox::isBoxMon()
{
  return true;
}

void PokemonBox::changeMove(int ind, int moveID, int pp, int ppUp)
{
  moves[ind]->changeMove(moveID, pp, ppUp);
}

void PokemonBox::deleteMoveAt(int ind)
{
  if(ind < 0 || ind >= maxMoves)
    return;

  // Clear the slot, then compact (cleanupMoves slides the later moves up and
  // emits each slot's per-field signals so the UI refreshes).
  moves[ind]->moveID = 0;
  moves[ind]->pp = 0;
  moves[ind]->ppUp = 0;
  cleanupMoves();

  movesChanged();
}

void PokemonBox::clearMovesButFirst()
{
  // Compact first so the surviving move is genuinely the list's first slot, then
  // clear the rest.
  cleanupMoves();

  for(int i = 1; i < maxMoves; i++) {
    moves[i]->moveID = 0;
    moves[i]->moveIDChanged();

    moves[i]->pp = 0;
    moves[i]->ppChanged();

    moves[i]->ppUp = 0;
    moves[i]->ppUpChanged();
  }

  movesChanged();
}

void PokemonBox::correctMoveAt(int ind)
{
  if(ind < 0 || ind >= maxMoves)
    return;

  // correctMove() may clear an invalid/duplicate move (leaving a gap); compact so
  // the later moves slide up and there is no hole.
  moves[ind]->correctMove();
  cleanupMoves();

  movesChanged();
}

void PokemonBox::reorderMove(int from, int to)
{
  // The (id, pp, ppUp) triple that travels together when a move is reordered, so
  // a move keeps its current/max PP as it changes slots.
  struct MoveVals { int id; int pp; int ppUp; };

  // Collect the filled move slots (the compact prefix -- the first empty slot
  // ends the move list in game logic). Only filled moves reorder; empties stay
  // parked at the bottom.
  QVector<MoveVals> vec;
  for(int i = 0; i < maxMoves; i++) {
    if(moves[i]->moveID <= 0)
      break;
    vec.append({moves[i]->moveID, moves[i]->pp, moves[i]->ppUp});
  }

  if(from < 0 || from >= vec.size())
    return;

  MoveVals moved = vec.at(from);

  // Anchor = the first move at/after the drop slot that ISN'T the one being moved;
  // the move is re-inserted directly before it, or appended when there is none
  // (dropping past the last move). Mirrors the storage drag-reorder convention.
  int anchorIdx = -1;
  for(int i = qBound(0, to, vec.size()); i < vec.size(); i++) {
    if(i != from) {
      anchorIdx = i;
      break;
    }
  }
  int anchorShift = (anchorIdx > from) ? 1 : 0; // removing 'from' shifts the anchor left

  vec.removeAt(from);
  if(anchorIdx < 0)
    vec.append(moved);
  else
    vec.insert(anchorIdx - anchorShift, moved);

  // Write the reordered values back into the fixed slot objects (the slot QObjects
  // themselves stay put -- only their values move -- so QML's movesAt() pointers
  // remain valid). changeMove() emits the per-field signals each row binds to.
  for(int i = 0; i < maxMoves; i++) {
    if(i < vec.size())
      moves[i]->changeMove(vec.at(i).id, vec.at(i).pp, vec.at(i).ppUp);
    else
      moves[i]->changeMove(0, 0, 0);
  }

  movesChanged();
}

void PokemonBox::resetPokemon()
{
  level = 5;
  levelChanged();

  auto record = isValid();
  if(record == nullptr)
    return;

  clearMoves();

  for(int i = 0; i < 4 && i < record->toInitial.size(); i++) {
    auto moveData = record->toInitial.at(i);
    moves[i]->moveID = moveData->ind;
    moves[i]->moveIDChanged();

    moves[i]->pp = *moveData->pp;
    moves[i]->ppChanged();

    moves[i]->ppUp = 0;
    moves[i]->ppUpChanged();
  }

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

  spdExp = pkmn->spdExp;
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

  for(int i = 0; i < 4; i++) {
    moves[i]->moveID = pkmn->moves[i]->moveID;
    moves[i]->moveIDChanged();

    moves[i]->pp = pkmn->moves[i]->pp;
    moves[i]->ppChanged();

    moves[i]->ppUp = pkmn->moves[i]->ppUp;
    moves[i]->ppUpChanged();
  }

  movesChanged();

  type2Explicit = false;
  type2ExplicitChanged();
}

PokemonDBEntry* PokemonBox::toData()
{
  return PokemonDB::inst()->getIndAt(QString::number(species));
}

int PokemonBox::movesCount()
{
  int ret = 0;

  // Follows game logic
  // The first move with 0 ends move lookup
  for(int i = 0; i < 4; i++) {
    if(moves[i]->moveID <= 0)
      break;

    ret++;
  }

  return ret;
}

int PokemonBox::movesMax()
{
  return maxMoves;
}

PokemonMove* PokemonBox::movesAt(int ind)
{
  return qmlCppOwned(moves[ind]);
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

void PokemonBox::manualSpeciesChanged()
{
  update(true, true, true, true);
}

void PokemonBox::manualLevelChanged()
{
  update(true, true);
}

int PokemonBox::atkStat()
{
  return nonHpStat(PokemonStats::Attack);
}

int PokemonBox::defStat()
{
  return nonHpStat(PokemonStats::Defense);
}

int PokemonBox::spdStat()
{
  return nonHpStat(PokemonStats::Speed);
}

int PokemonBox::spStat()
{
  return nonHpStat(PokemonStats::Special);
}
