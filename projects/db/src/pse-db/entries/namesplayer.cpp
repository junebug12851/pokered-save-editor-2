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

#include <QVector>
#include <QJsonArray>
#include <QtMath>
#include <QQmlEngine>

#include "./namesplayer.h"
#include "../util/gamedata.h"
#include <pse-common/random.h>
#include <pse-common/utility.h>

void NamesPlayer::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<NamesPlayer>("PSE.DB.NamesPlayer", 1, 0, "NamesPlayer", "Can't instantiate in QML");
  once = true;
}

NamesPlayer::NamesPlayer()
  : AbstractRandomString("names")
{
  qmlRegister();
}

NamesPlayer* NamesPlayer::inst()
{
  static NamesPlayer* _inst = new NamesPlayer;
  return _inst;
}
