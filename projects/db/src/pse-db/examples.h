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
#ifndef EXAMPLES_H
#define EXAMPLES_H

#include <QObject>

class QQmlEngine;
class ExamplesPlayer;
class ExamplesRival;
class ExamplesPokemon;

class Examples : public QObject
{
  Q_OBJECT
  Q_PROPERTY(ExamplesPlayer* player READ player CONSTANT)
  Q_PROPERTY(ExamplesRival* rival READ rival CONSTANT)
  Q_PROPERTY(ExamplesPokemon* pokemon READ pokemon CONSTANT)

public:
  static Examples* inst();

  const ExamplesPlayer* player() const;
  const ExamplesRival* rival() const;
  const ExamplesPokemon* pokemon() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  Examples();
};

#endif // EXAMPLES_H
