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
#ifndef AREAPOKEMON_H
#define AREAPOKEMON_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"

class SaveFile;
class SaveFileIterator;
class MapDBEntry;

constexpr var8 wildMonsCount = 10;

class AreaPokemonWild : public QObject {

  Q_OBJECT

  Q_PROPERTY(int index MEMBER index NOTIFY indexChanged)
  Q_PROPERTY(int level MEMBER level NOTIFY levelChanged)

public:
  AreaPokemonWild(int index = 0, int level = 0);
  AreaPokemonWild(bool random);

  void load(SaveFileIterator* it);
  void save(SaveFileIterator* it);

  bool operator<(const AreaPokemonWild& a);
  bool operator>(const AreaPokemonWild& a);

signals:
  void indexChanged();
  void levelChanged();

public slots:
  // Generates a random Pokemon from any dex entry and level
  void randomize();
  void reset();
  void load(int index, int level);

public:
  // Pokemon index number and level
  int index;
  int level;
};

/**
     * Rate is how likely to encounter Pokemon
     * higher number = higher chance
     * A rate of 0 means no wild pokemon on map
     *
     * The Pokemon list is in order from most common to most rare
     * Pokemon 0: 19.9% chance
     * Pokemon 1: 19.9% chance
     * Pokemon 2: 15.2% chance
     * Pokemon 3: 9.8% chance
     * Pokemon 4: 9.8% chance
     * Pokemon 5: 9.8% chance
     * Pokemon 6: 5.1% chance
     * Pokemon 7: 5.1% chance
     * Pokemon 8: 4.3% chance
     * Pokemon 9: 1.2% chance
     */
class AreaPokemon : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int grassRate MEMBER grassRate NOTIFY grassRateChanged)
  Q_PROPERTY(int waterRate MEMBER waterRate NOTIFY waterRateChanged)
  Q_PROPERTY(bool pauseMons3Steps MEMBER pauseMons3Steps NOTIFY pauseMons3StepsChanged)

  // C++ Arrays can't be Q_PROPERTY and don't need a signal because they are
  // pre-created with only their contents changing and have no properties of
  // their own

public:
  AreaPokemon(SaveFile* saveFile = nullptr);
  virtual ~AreaPokemon();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int grassMonsCount();
  Q_INVOKABLE AreaPokemonWild* grassMonsAt(int ind);
  Q_INVOKABLE void grassMonsSwap(int from, int to);

  Q_INVOKABLE int waterMonsCount();
  Q_INVOKABLE AreaPokemonWild* waterMonsAt(int ind);
  Q_INVOKABLE void waterMonsSwap(int from, int to);

signals:
  void grassRateChanged();
  void waterRateChanged();
  void pauseMons3StepsChanged();
  void grassMonsChanged();
  void waterMonsChanged();

public slots:
  void reset();
  void randomize();
  void setTo(MapDBEntry* map);

public:
  // There are exactly 10 wild Pokemon in areas that have wild Pokemon
  // Create 10 entries each, no more or less
  int grassRate;
  AreaPokemonWild* grassMons[wildMonsCount];

  int waterRate;
  AreaPokemonWild* waterMons[wildMonsCount];

  bool pauseMons3Steps;
};

#endif // AREAPOKEMON_H
