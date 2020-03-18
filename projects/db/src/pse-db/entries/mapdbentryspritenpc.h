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
#ifndef MAPDBENTRYSPRITENPC_H
#define MAPDBENTRYSPRITENPC_H

#include "./mapdbentrysprite.h"
#include "../db_autoport.h"

class MapsDB;

// A regular NPC that says a few lines and may have a script that's run
struct DB_AUTOPORT MapDBEntrySpriteNPC : public MapDBEntrySprite
{
  Q_OBJECT

public:
  virtual SpriteType type() const;

protected:
  MapDBEntrySpriteNPC(const QJsonValue& data, MapDBEntry* const parent);
  virtual void qmlRegister() const;

  friend class MapDBEntry;
};

#endif // MAPDBENTRYSPRITENPC_H
