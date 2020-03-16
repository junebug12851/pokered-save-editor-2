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
#include "hiddencoinsdb.h"

HiddenCoinsDB* HiddenCoinsDB::inst()
{
  static HiddenCoinsDB* _inst = new HiddenCoinsDB;
  return _inst;
}

void HiddenCoinsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<HiddenCoinsDB>(
        "PSE.DB.HiddenCoinsDB", 1, 0, "HiddenCoinsDB", "Can't instantiate in QML");
  once = true;
}

HiddenCoinsDB::HiddenCoinsDB()
  : AbstractHiddenItemDB("hiddenCoins")
{
  qmlRegister();
}
