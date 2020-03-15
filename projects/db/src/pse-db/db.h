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
#include <QQmlEngine>
#include <QQmlContext>
#include "./db_autoport.h"

// An instance of DB must be retrieved at the start of the program or none of
// this may work.

class GameData;
class CreditsDB;

// Provides a common interface for the databases to use and a common interface
// to the databases.
class DB_AUTOPORT DB : public QObject
{
  Q_OBJECT

  Q_PROPERTY(GameData* json READ json CONSTANT)
  Q_PROPERTY(CreditsDB* credits READ credits CONSTANT)

public:
  static const DB* inst();

  // While they can be accessed directly, this allows QML to access them easier
  // and without polluting the global namespace
  GameData* json();
  CreditsDB* credits();

public slots:
  // It's very important to protect the engine from QML, in some cases QML may
  // think it has rights to delete the data which can be catastrophic.
  void engineProtect(const QQmlEngine* const engine) const;

  // Hooks into a QML Context
  void engineHook(QQmlContext* const context);

  // Generic utility for any of the databases to use
  static void engineProtectUtil(const QObject* const obj, const QQmlEngine* const engine);

private slots:
  // Init the DLL resources, very important before any DB loading happens
  void initRes() const;

  // Register this to QML
  void engineRegister() const;

  // Load, Index, and Deep Link all the DB
  void loadAll() const;
  void indexAll() const;
  void deepLinkAll() const;

private:
  DB();
};

#endif // DB_H
