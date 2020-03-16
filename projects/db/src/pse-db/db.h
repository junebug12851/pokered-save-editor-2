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
#ifndef DB_H
#define DB_H

#include <QObject>
#include <QQmlContext>
#include "./db_autoport.h"

// An instance of DB must be retrieved at the start of the program or none of
// this will work. No class or method in this module should even be accessed
// without first accessing a DB instance

// First access to the DB sets up the module, creates the DB and all instances,
// and loads, indexes, and deep links all data. It essentially constructs the
// entire module.

class QQmlEngine;
class GameData;
class CreditsDB;
class EventPokemonDB;
class EventsDB;
class Examples;
class Names;
class FlyDB;
class FontsDB;

// Provides a common interface for the databases to use and a common interface
// to the databases.
class DB_AUTOPORT DB : public QObject
{
  Q_OBJECT

  Q_PROPERTY(GameData* json READ json CONSTANT)
  Q_PROPERTY(CreditsDB* credits READ credits CONSTANT)
  Q_PROPERTY(EventPokemonDB* eventPokemon READ eventPokemon CONSTANT)
  Q_PROPERTY(EventsDB* events READ events CONSTANT)
  Q_PROPERTY(Examples* examples READ examples CONSTANT)
  Q_PROPERTY(Names* names READ names CONSTANT)
  Q_PROPERTY(FlyDB* fly READ fly CONSTANT)
  Q_PROPERTY(FontsDB* fonts READ fonts CONSTANT)

public:
  static DB* inst();

  // While they can be accessed directly, this allows QML to access them easier
  // and without polluting the global namespace
  GameData* json() const;
  CreditsDB* credits() const;
  EventPokemonDB* eventPokemon() const;
  EventsDB* events() const;
  Examples* examples() const;
  Names* names() const;
  FlyDB* fly() const;
  FontsDB* fonts() const;

public slots:
  // It's very important to protect the engine from QML, in some cases QML may
  // think it has rights to delete the data which can be catastrophic.
  void qmlProtect(const QQmlEngine* const engine) const;

  // Hooks into a QML Context
  void qmlHook(QQmlContext* const context) const;

private slots:
  // Init the DLL resources, very important before any DB loading happens
  void initRes() const;

  // Register this to QML
  void qmlRegister() const;

  // Load, Index, and Deep Link all the DB
  void loadAll() const;
  void indexAll() const;
  void deepLinkAll() const;

private:
  DB();
};

#endif // DB_H
