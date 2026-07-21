/*
  * Copyright 2020 Fairy Fox
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

/**
 * @brief Aggregate of the example/preset sources (player, rival, Pokemon).
 *
 * Groups @ref player (ExamplesPlayer), @ref rival (ExamplesRival), and
 * @ref pokemon (ExamplesPokemon) under one singleton, reached via `db.examples`.
 * These supply ready-made example values (e.g. a random example player) used by
 * the editors. See db.md.
 *
 * @see ExamplesPlayer, ExamplesRival, ExamplesPokemon.
 */
class Examples : public QObject
{
  Q_OBJECT
  Q_PROPERTY(ExamplesPlayer*  player  READ player  CONSTANT) ///< Example player source.
  Q_PROPERTY(ExamplesRival*   rival   READ rival   CONSTANT) ///< Example rival source.
  Q_PROPERTY(ExamplesPokemon* pokemon READ pokemon CONSTANT) ///< Example Pokemon source.

public:
  static Examples* inst(); ///< The process-wide Examples singleton.

  ExamplesPlayer*  player()  const; ///< The example-player source (backs @c player).
  ExamplesRival*   rival()   const; ///< The example-rival source (backs @c rival).
  ExamplesPokemon* pokemon() const; ///< The example-Pokemon source (backs @c pokemon).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  Examples(); ///< Private -- use inst().
};
