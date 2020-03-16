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
#ifndef HIDDENCOINS_H
#define HIDDENCOINS_H

#include <QObject>
#include "./db_autoport.h"

class HiddenItemDBEntry;
class QQmlEngine;

class DB_AUTOPORT AbstractHiddenItemDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)

public:
  // Get Properties, includes QML array helpers
  const QVector<HiddenItemDBEntry*> getStore() const;
  int getStoreSize() const;

   Q_INVOKABLE const HiddenItemDBEntry* getStoreAt(const int ind) const;

public slots:
  void load();
  void deepLink();
  void qmlProtect(const QQmlEngine* const engine) const;

protected slots:
  virtual void qmlRegister() const = 0;

protected:
  AbstractHiddenItemDB(const QString loadFile);

  QVector<HiddenItemDBEntry*> store;
  const QString loadFile;
};

#endif // HIDDENCOINS_H
