/*
  * Copyright 2020 Twilight
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
#pragma once
#include <QObject>

#include "./entries/examplesplayer.h"
#include "./entries/examplespokemon.h"
#include "./entries/examplesrival.h"

class QQmlEngine;

class Examples : public QObject
{
  Q_OBJECT
  Q_PROPERTY(ExamplesPlayer*  player  READ player  CONSTANT)
  Q_PROPERTY(ExamplesRival*   rival   READ rival   CONSTANT)
  Q_PROPERTY(ExamplesPokemon* pokemon READ pokemon CONSTANT)

public:
  static Examples* inst();

  ExamplesPlayer*  player()  const;
  ExamplesRival*   rival()   const;
  ExamplesPokemon* pokemon() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  Examples();
};
