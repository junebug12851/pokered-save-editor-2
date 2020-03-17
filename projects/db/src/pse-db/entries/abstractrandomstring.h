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
#ifndef ABSTRACTEXAMPLE_H
#define ABSTRACTEXAMPLE_H

#include <QObject>
#include <QString>

#include "../db_autoport.h"

class QQmlEngine;

class DB_AUTOPORT AbstractRandomString : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize STORED false NOTIFY listChanged)
  Q_PROPERTY(QString randomExample READ randomExample STORED false)

signals:
  // Sort of an exception, the DB is almost entirely read-only but this is an
  // exception of some moving parts in the DB
  void listChanged();

public:
  const QVector<QString> getStore() const;
  int getStoreSize() const;
  Q_INVOKABLE const QString getStoreAt(const int ind) const;

  QString randomExample();

public slots:
  // QML accessible methods
  void load();
  void qmlProtect(const QQmlEngine* const engine) const;

protected slots:
  virtual void qmlRegister() const = 0;

protected:
  AbstractRandomString(const QString fileName);
  QVector<QString> store;
  const QString fileName;
};

#endif // ABSTRACTEXAMPLE_H
