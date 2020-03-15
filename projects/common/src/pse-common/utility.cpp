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

#include <QByteArray>
#include <QStringList>
#include <QQmlContext>
#include "./utility.h"
#include "./random.h"

// Thanks eyllanesc
// https://stackoverflow.com/questions/45772951/converting-qstring-to-ascii-value-vice-versa-in-qt
Utility* Utility::inst()
{
  static Utility* _inst = new Utility;
  return _inst;
}

Random* Utility::random()
{
  return Random::inst();
}

const QString Utility::encodeBeforeUrl(const QString beforeStr) const
{
  QStringList numberString;
  for(const auto character: beforeStr){
      numberString << QString::number(character.unicode(), 16);
  }

  return numberString.join(" ");
}

// Thanks eyllanesc
// https://stackoverflow.com/questions/45772951/converting-qstring-to-ascii-value-vice-versa-in-qt
const QString Utility::decodeAfterUrl(QString beforeStr) const
{
  return QByteArray::fromHex(beforeStr.remove(" ").toLocal8Bit());
}

void Utility::engineProtectUtil(const QObject* const obj, const QQmlEngine* const engine)
{
  // For some reason this demands it not be const
  engine->setObjectOwnership(const_cast<QObject*>(obj), QQmlEngine::CppOwnership);
}

void Utility::engineProtect(const QQmlEngine* const engine) const
{
  engineProtectUtil(this, engine);
  Random::inst()->engineProtect(engine);
}

void Utility::engineHook(QQmlContext* const context)
{
  context->setContextProperty("pseCommon", this);
}

void Utility::engineRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<Utility>("PSE.Common.Utility", 1, 0, "Utility", "Can't instantiate in QML");
  registered = true;
}

Utility::Utility()
{
  engineRegister();
  Random::inst();
}
