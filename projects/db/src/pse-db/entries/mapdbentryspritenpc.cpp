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
#include "mapdbentryspritenpc.h"

MapDBEntrySpriteNPC::MapDBEntrySpriteNPC(const QJsonValue& data,
                                         MapDBEntry* const parent) :
  MapDBEntrySprite(data, parent)
{
  qmlRegister();
}

void MapDBEntrySpriteNPC::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntrySpriteNPC>(
        "PSE.DB.MapDBEntrySpriteNPC", 1, 0, "MapDBEntrySpriteNPC", "Can't instantiate in QML");
  once = true;
}

MapDBEntrySpriteNPC::SpriteType MapDBEntrySpriteNPC::type() const
{
  return SpriteType::NPC;
}
