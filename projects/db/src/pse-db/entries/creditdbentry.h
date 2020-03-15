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
#ifndef CREDITDBENTRY_H
#define CREDITDBENTRY_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QJsonValue>

#include "../db_autoport.h"

struct DB_AUTOPORT CreditDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString getSection READ getSection CONSTANT)
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(QString getUrl READ getUrl CONSTANT)
  Q_PROPERTY(QString getNote READ getNote CONSTANT)
  Q_PROPERTY(QString getLicense READ getLicense CONSTANT)
  Q_PROPERTY(QString getMandated READ getMandated CONSTANT)

public:
  QString getSection() const;
  QString getName() const;
  QString getUrl() const;
  QString getNote() const;
  QString getLicense() const;
  QString getMandated() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  CreditDBEntry();
  CreditDBEntry(QJsonValue& data);
  CreditDBEntry(QString section);

  static void process(QJsonObject& data);
  void qmlRegister() const;

  QString section = "";
  QString name = "";
  QString url = "";
  QString note = "";
  QString license = "";
  QString mandated = "";

  friend class CreditsDB;
};

#endif // CREDITDBENTRY_H
