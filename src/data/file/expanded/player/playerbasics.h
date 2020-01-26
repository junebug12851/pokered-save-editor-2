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
#ifndef PLAYERBASICS_H
#define PLAYERBASICS_H

#include <QObject>
#include <QString>
#include "../../../../common/types.h"
class SaveFile;
struct PokemonDBEntry;

struct Badges : public QObject
{
  Q_OBJECT
  Q_ENUMS(Badges_)

public:
  enum Badges_ : var8 {
    Boulder = 0,
    Cascade,
    Thunder,
    Rainbow,
    Soul,
    Marsh,
    Volcano,
    Earth
  };
};

constexpr var8 maxBadges = 8;

class PlayerBasics : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString playerName MEMBER playerName NOTIFY playerNameChanged)
  Q_PROPERTY(int playerID MEMBER playerID NOTIFY playerIDChanged)
  Q_PROPERTY(unsigned int money MEMBER money NOTIFY moneyChanged)
  Q_PROPERTY(int coins MEMBER coins NOTIFY coinsChanged)
  Q_PROPERTY(int playerStarter MEMBER playerStarter NOTIFY playerStarterChanged)

public:
  PlayerBasics(SaveFile* saveFile = nullptr);
  virtual ~PlayerBasics();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void setBadges(SaveFile* saveFile, var16 offset);

  PokemonDBEntry* toStarter();

  Q_INVOKABLE int badgeCount();
  Q_INVOKABLE bool badgeAt(int ind);
  Q_INVOKABLE void badgeSet(int ind, bool val);

signals:
  void playerNameChanged();
  void playerIDChanged();
  void moneyChanged();
  void coinsChanged();
  void badgesChanged();
  void playerStarterChanged();

public slots:
  void reset();
  void randomize();

public:
  QString playerName;
  int playerID;
  unsigned int money;
  int coins;
  bool badges[maxBadges] = {
    false, // Boulder
    false, // Cascade
    false, // Thunder
    false, // Rainbow
    false, // Soul
    false, // Marsh
    false, // Volcano
    false // Earth
  };
  int playerStarter;
};

#endif // PLAYERBASICS_H
