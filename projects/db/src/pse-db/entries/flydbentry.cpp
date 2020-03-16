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

#include <QtDebug>
#include <pse-common/utility.h>

#include "flydbentry.h"
#include "../maps.h"

FlyDBEntry::FlyDBEntry() {
  qmlRegister();
}

FlyDBEntry::FlyDBEntry(QJsonValue& data)
{
  qmlRegister();

  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
}

void FlyDBEntry::deepLink()
{
  toMap = MapsDB::ind.value(name, nullptr);

#ifdef QT_DEBUG
  if(toMap == nullptr)
    qCritical() << "Fly Destination: " << name << ", could not be deep linked." ;
#endif

  if(toMap != nullptr)
    toMap->toFlyDestination = this;
}

const MapDBEntry* FlyDBEntry::getToMap() const
{
  return toMap;
}

void FlyDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

int FlyDBEntry::getInd() const
{
    return ind;
}

const QString FlyDBEntry::getName() const
{
    return name;
}
