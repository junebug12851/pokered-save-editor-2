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
#ifndef EVENTPOKEMON_H
#define EVENTPOKEMON_H

// With amazing help of Quicktype!!!
// https://app.quicktype.io

#include <QObject>
#include <QVector>

#include "./db_autoport.h"

// These are Pokemon you get by going to or participating in real-life events
// that were held around the world

struct EventPokemonDBEntry;
class QQmlEngine;

class DB_AUTOPORT EventPokemonDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  // Get Instance
  static EventPokemonDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<EventPokemonDBEntry*> getStore() const;
  int getStoreSize() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const EventPokemonDBEntry* getStoreAt(const int ind) const;

public slots:
  // QML accessible methods
  void load();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  // Singleton Constructor
  EventPokemonDB();

  // Store
  QVector<EventPokemonDBEntry*> store;
};

#endif // EVENTPOKEMON_H
