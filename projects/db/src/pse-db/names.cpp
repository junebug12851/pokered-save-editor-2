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

#include <QQmlEngine>
#include <pse-common/utility.h>
#include "names.h"

#include "./entries/namesplayer.h"
#include "./entries/namespokemon.h"

Names* Names::inst()
{
  static Names* _inst = new Names;
  return _inst;
}

const NamesPlayer* Names::player() const
{
  return NamesPlayer::inst();
}

const NamesPokemon* Names::pokemon() const
{
  return NamesPokemon::inst();
}

void Names::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  NamesPlayer::inst()->qmlProtect(engine);
  NamesPokemon::inst()->qmlProtect(engine);
}

void Names::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<Names>("PSE.DB.Names", 1, 0, "Names", "Can't instantiate in QML");
  once = true;
}

Names::Names()
{
  qmlRegister();

  NamesPlayer::inst();
  NamesPokemon::inst();
}
