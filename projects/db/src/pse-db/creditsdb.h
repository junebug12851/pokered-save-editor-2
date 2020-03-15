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
#ifndef CREDITSDB_H
#define CREDITSDB_H

// With amazing help of Quicktype!!!
// https://app.quicktype.io

#include <QObject>
#include <QVector>

#include "./db_autoport.h"

class QQmlEngine;
struct CreditDBEntry;

// Singleton accessible, registered to, and protected from QML
class DB_AUTOPORT CreditsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  // Get Instance
  static CreditsDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<CreditDBEntry*> getStore() const;
  int getStoreSize() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const CreditDBEntry* getStoreAt(const int ind) const;

public slots:
  // QML accessible methods
  void load();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  // Singleton Constructor
  CreditsDB();

  // Store
  QVector<CreditDBEntry*> store;

  // Allow modifications from these classes
  friend struct CreditDBEntry;
};

#endif // CREDITSDB_H
