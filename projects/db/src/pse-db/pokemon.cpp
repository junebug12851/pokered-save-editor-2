/*
  * Copyright 2019 Twilight
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
 * @file pokemon.cpp
 * @brief Implementation of PokemonDB and the PokemonDBEntry family (species,
 *        moves, evolutions, and the heavy deep-link). See pokemon.h.
 */

#include <QJsonArray>
#include <QJsonObject>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./pokemon.h"
#include "./util/gamedata.h"
#include "./itemsdb.h"
#include "./entries/itemdbentry.h"
#include "./moves.h"
#include "./types.h"

// ── PokemonDBEntryEvolution ──────────────────────────────────────────────────

PokemonDBEntryEvolution::PokemonDBEntryEvolution() {}
PokemonDBEntryEvolution::PokemonDBEntryEvolution(QJsonValue& data,
                                                  PokemonDBEntry* parent)
  : parent(parent)
{
  toName = data["toName"].toString();
  if (data["level"].isDouble()) level = static_cast<var8>(data["level"].toDouble());
  if (data["trade"].isBool())   trade = data["trade"].toBool();
  if (data["item"].isString())  item  = data["item"].toString();
}

void PokemonDBEntryEvolution::deepLink(PokemonDBEntry* deEvolution)
{
  toEvolution = PokemonDB::inst()->getIndAt(toName);
  if (!item.isEmpty())
    toItem = ItemsDB::inst()->getIndAt(item);

#ifdef QT_DEBUG
  if (!toEvolution) qCritical() << "Evolution:" << toName << "could not be deep linked.";
  if (!deEvolution) qCritical() << "Evolution:" << toName << "null deEvolution provided.";
  if (!item.isEmpty() && !toItem) qCritical() << "Evolution item:" << item << "could not be deep linked.";
#endif

  if (toEvolution) {
    toEvolution->toDeEvolution = deEvolution;
    toDeEvolution = deEvolution;
  }
  if (toItem)
    toItem->toEvolvePokemon.append(this);
}

// ── PokemonDBEntryMove ───────────────────────────────────────────────────────

PokemonDBEntryMove::PokemonDBEntryMove() {}
PokemonDBEntryMove::PokemonDBEntryMove(QJsonValue& data, PokemonDBEntry* parent)
  : parent(parent)
{
  level = static_cast<var8>(data["level"].toDouble());
  move  = data["move"].toString();
}

void PokemonDBEntryMove::deepLink()
{
  toMove = MovesDB::inst()->getIndAt(move);
#ifdef QT_DEBUG
  if (!toMove) qCritical() << "Pokemon move:" << move << "could not be deep linked.";
#endif
  if (toMove)
    toMove->toPokemonLearned.append(this);
}

// ── PokemonDBEntry ───────────────────────────────────────────────────────────

PokemonDBEntry::PokemonDBEntry() {}
PokemonDBEntry::PokemonDBEntry(QJsonValue& data)
{
  name     = data["name"].toString();
  ind      = static_cast<var8>(data["ind"].toDouble());
  readable = data["readable"].toString();

  if (data["pokedex"].isDouble())    pokedex    = static_cast<var8>(data["pokedex"].toDouble());
  if (data["growthRate"].isDouble()) growthRate = static_cast<var8>(data["growthRate"].toDouble());
  if (data["baseHp"].isDouble())     baseHp     = static_cast<var8>(data["baseHp"].toDouble());
  if (data["baseAttack"].isDouble()) baseAttack = static_cast<var8>(data["baseAttack"].toDouble());
  if (data["baseDefense"].isDouble()) baseDefense = static_cast<var8>(data["baseDefense"].toDouble());
  if (data["baseSpeed"].isDouble())  baseSpeed  = static_cast<var8>(data["baseSpeed"].toDouble());
  if (data["baseSpecial"].isDouble()) baseSpecial = static_cast<var8>(data["baseSpecial"].toDouble());
  if (data["baseExpYield"].isDouble()) baseExpYield = static_cast<var8>(data["baseExpYield"].toDouble());
  if (data["catchRate"].isDouble())  catchRate  = static_cast<var8>(data["catchRate"].toDouble());
  if (data["type1"].isString())      type1      = data["type1"].toString();
  if (data["type2"].isString())      type2      = data["type2"].toString();
  if (data["glitch"].isBool())       glitch     = data["glitch"].toBool();

  if (data["moves"].isArray())
    for (QJsonValue e : data["moves"].toArray())
      moves.append(new PokemonDBEntryMove(e, this));

  if (data["initial"].isArray())
    for (QJsonValue e : data["initial"].toArray())
      initial.append(e.toString());

  if (data["tmHm"].isArray())
    for (QJsonValue e : data["tmHm"].toArray())
      tmHm.append(static_cast<var8>(e.toDouble()));

  if (data["evolution"].isArray()) {
    for (QJsonValue e : data["evolution"].toArray())
      evolution.append(new PokemonDBEntryEvolution(e, this));
  } else if (data["evolution"].isObject()) {
    auto tmp = data["evolution"];
    evolution.append(new PokemonDBEntryEvolution(tmp, this));
  }
}

void PokemonDBEntry::deepLink()
{
  if (!type1.isEmpty()) {
    toType1 = TypesDB::inst()->getIndAt(type1);
    if (toType1) toType1->toPokemon.append(this);
#ifdef QT_DEBUG
    if (!toType1) qCritical() << "Pokemon type1:" << type1 << "could not be deep linked.";
#endif
  }
  if (!type2.isEmpty()) {
    toType2 = TypesDB::inst()->getIndAt(type2);
    if (toType2) toType2->toPokemon.append(this);
#ifdef QT_DEBUG
    if (!toType2) qCritical() << "Pokemon type2:" << type2 << "could not be deep linked.";
#endif
  }

  for (auto* evolEntry : evolution)
    evolEntry->deepLink(this);

  for (auto* pokeMoveEntry : moves)
    pokeMoveEntry->deepLink();

  for (const auto& initMove : initial) {
    auto* link = MovesDB::inst()->getIndAt(initMove);
    toInitial.append(link);
#ifdef QT_DEBUG
    if (!link) qCritical() << "Pokemon initial move:" << initMove << "could not be deep linked.";
#endif
    if (link) link->toPokemonInitial.append(this);
  }

  for (auto tmHmMove : tmHm) {
    auto* moveLink = MovesDB::inst()->getIndAt("tm" + QString::number(tmHmMove));
    auto* itemLink = ItemsDB::inst()->getIndAt("tm" + QString::number(tmHmMove));
    toTmHmMove.append(moveLink);
    toTmHmItem.append(itemLink);
#ifdef QT_DEBUG
    if (!moveLink) qCritical() << "Pokemon TM/HM move:" << tmHmMove << "could not be deep linked.";
    if (!itemLink) qCritical() << "Pokemon TM/HM item:" << tmHmMove << "could not be deep linked.";
#endif
    if (moveLink) moveLink->toPokemonTmHm.append(this);
    if (itemLink) itemLink->toTeachPokemon.append(this);
  }
}

// ── PokemonDB ────────────────────────────────────────────────────────────────

PokemonDB* PokemonDB::inst()
{
  static PokemonDB* _inst = new PokemonDB;
  return _inst;
}

const QVector<PokemonDBEntry*> PokemonDB::getStore() const { return store; }
const QHash<QString, PokemonDBEntry*> PokemonDB::getInd() const { return ind; }
int PokemonDB::getStoreSize() const { return store.size(); }

PokemonDBEntry* PokemonDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

PokemonDBEntry* PokemonDB::getIndAt(const QString& key) const
{
  return ind.value(key, nullptr);
}

void PokemonDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("pokemon");
  for (QJsonValue entry : jsonData.array())
    store.append(new PokemonDBEntry(entry));
  once = true;
}

void PokemonDB::index()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store) {
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
    ind.insert(entry->readable, entry);
    if (entry->pokedex)
      ind.insert("dex" + QString::number(*entry->pokedex), entry);
  }
  once = true;
}

void PokemonDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    entry->deepLink();
  once = true;
}

void PokemonDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void PokemonDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<PokemonDB>("PSE.DB.PokemonDB", 1, 0, "PokemonDB", "Can't instantiate in QML");
  once = true;
}

PokemonDB::PokemonDB()
{
  qmlRegister();
}
