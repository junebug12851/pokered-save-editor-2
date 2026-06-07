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

#include "./db_autoport.h"
#include "./entries/namesplayer.h"
#include "./entries/namespokemon.h"

class QQmlEngine;

/**
 * @brief Small aggregate of the two random-name sources (player and Pokemon).
 *
 * Just groups @ref player (NamesPlayer) and @ref pokemon (NamesPokemon) under one
 * singleton so QML and the name randomizers reach them via `db.names`. Each source
 * supplies a random in-character name. See db.md.
 *
 * @see NamesPlayer, NamesPokemon.
 */
class DB_AUTOPORT Names : public QObject
{
  Q_OBJECT
  Q_PROPERTY(NamesPlayer*  player  READ player  CONSTANT) ///< Random player-name source.
  Q_PROPERTY(NamesPokemon* pokemon READ pokemon CONSTANT) ///< Random Pokemon-name source.

public:
  static Names* inst(); ///< The process-wide Names singleton.

  NamesPlayer*  player()  const; ///< The player-name source (backs @c player).
  NamesPokemon* pokemon() const; ///< The Pokemon-name source (backs @c pokemon).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  Names(); ///< Private -- use inst().
};
