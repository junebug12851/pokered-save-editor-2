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

#include <QQmlEngine>
#include "./hiddenItemsdb.h"

HiddenItemsDB* HiddenItemsDB::inst()
{
  static HiddenItemsDB* _inst = new HiddenItemsDB;
  return _inst;
}

void HiddenItemsDB::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<HiddenItemsDB>(
        "PSE.DB.HiddenItemsDB", 1, 0, "HiddenItemsDB", "Can't instantiate in QML");
  once = true;
}

HiddenItemsDB::HiddenItemsDB()
  : AbstractHiddenItemDB("hiddenItems")
{
  qmlRegister();
}
