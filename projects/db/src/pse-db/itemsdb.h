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
#ifndef ITEMS_H
#define ITEMS_H

#include <QObject>
#include <QVector>
#include <QHash>

#include "./db_autoport.h"

class ItemDBEntry;
class QQmlEngine;

class DB_AUTOPORT ItemsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  // Get Instance
  static ItemsDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<ItemDBEntry*> getStore() const;
  const QHash<QString, ItemDBEntry*> getInd() const;
  int getStoreSize() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const ItemDBEntry* getStoreAt(const int ind) const;
  Q_INVOKABLE const ItemDBEntry* getIndAt(const QString val) const;

public slots:
  void load();
  void index();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  ItemsDB();

  QVector<ItemDBEntry*> store;
  QHash<QString, ItemDBEntry*> ind;
};

#endif // ITEMS_H
