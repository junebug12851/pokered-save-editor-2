/*
  * Copyright 2019 June Hanabi
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
#ifndef TRADE_H
#define TRADE_H

#include "../../common/types.h"
#include <QString>

struct PokemonEntry;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// All in-game trades including the unused one
// All this was extracted from in-game data, not fan sites

struct TradeEntry {
  TradeEntry();

  QString give;
  QString get;
  var8 textId;
  QString nickname;
  bool unused;

  PokemonEntry* toGive;
  PokemonEntry* toGet;
};

class Trades
{
public:
  static void load();
  static void deepLink();

  static QVector<TradeEntry*>* trades;
};

#endif // TRADE_H
