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
#ifndef RANDOM_H
#define RANDOM_H

#include <QObject>
#include <QRandomGenerator>

#include "./common_autoport.h"

class QRandomGenerator;
class QQmlEngine;
class QQmlContext;

class COMMON_AUTOPORT Random : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool flipCoin READ flipCoin STORED false)
  Q_PROPERTY(bool flipCoinF READ flipCoinF STORED false)

public:
  static Random* inst();

  // Often the int versions can yield bad randomization, the float versions are
  // provided in case they yield better results.

  // [0.00,to)
  Q_INVOKABLE float range(const float end) const;

  // [from,to]
  Q_INVOKABLE int rangeInclusive(const int start, const int end) const;

  // [from,to)
  Q_INVOKABLE int rangeExclusive(const int start, const int end) const;

  // Chance of failure, 0 - 100 or 0.00 - 1.00
  Q_INVOKABLE bool chanceFailure(const int percent) const;
  Q_INVOKABLE bool chanceFailure(const float percent) const;

  // Chance of success, 0 - 100 or 0.00 - 1.00
  Q_INVOKABLE bool chanceSuccess(const int percent) const;
  Q_INVOKABLE bool chanceSuccess(const float percent) const;

  // Chance of success, 50%
  bool flipCoin() const;
  bool flipCoinF() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  Random();

  QRandomGenerator* rnd = QRandomGenerator::global();
};

#endif // RANDOM_H
