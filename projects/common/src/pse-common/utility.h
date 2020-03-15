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
#ifndef UTILITY_H
#define UTILITY_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "./common_autoport.h"

class Random;

class COMMON_AUTOPORT Utility : public QObject
{
  Q_OBJECT
  Q_PROPERTY(Random* random READ random CONSTANT)

public:
  static Utility* inst();

  Random* random();

  Q_INVOKABLE const QString encodeBeforeUrl(const QString beforeStr) const;
  Q_INVOKABLE const QString decodeAfterUrl(QString beforeStr) const;

  // Generic utility for any of the databases to use
  static void engineProtectUtil(const QObject* const obj, const QQmlEngine* const engine);

public slots:
  void engineProtect(const QQmlEngine* const engine) const;
  void engineHook(QQmlContext* const context);

private slots:
  void engineRegister() const;

private:
  Utility();
};

#endif // UTILITY_H
