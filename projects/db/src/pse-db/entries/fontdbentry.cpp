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
#include "fontdbentry.h"

FontDBEntry::FontDBEntry() {
  qmlRegister();
}

bool FontDBEntry::getNormal() const
{
  return normal;
}

void FontDBEntry::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

bool FontDBEntry::getSingleChar() const
{
  return singleChar;
}

bool FontDBEntry::getVariable() const
{
  return variable;
}

bool FontDBEntry::getMultiChar() const
{
  return multiChar;
}

bool FontDBEntry::getControl() const
{
  return control;
}

const QString FontDBEntry::getTip() const
{
  return tip;
}

const QString FontDBEntry::getAlias() const
{
  return alias;
}

int FontDBEntry::getLength() const
{
  return length;
}

bool FontDBEntry::getPicture() const
{
  return picture;
}

bool FontDBEntry::getShorthand() const
{
  return shorthand;
}

const QString FontDBEntry::getName() const
{
  return name;
}

int FontDBEntry::getInd() const
{
  return ind;
}
FontDBEntry::FontDBEntry(const QJsonValue& data) {
  qmlRegister();

  // Set simple properties
  name = data["name"].toString();
  ind = data["ind"].toDouble();
  length = data["length"].toDouble();

  // Set simple optional properties
  if(data["shorthand"].isBool())
    shorthand = data["shorthand"].toBool();

  if(data["picture"].isBool())
    picture = data["picture"].toBool();

  alias = data["alias"].toString("");
  tip = data["tip"].toString("");

  if(data["control"].isBool())
    control = data["control"].toBool();

  if(data["multiChar"].isBool())
    multiChar = data["multiChar"].toBool();

  if(data["variable"].isBool())
    variable = data["variable"].toBool();

  if(data["singleChar"].isBool())
    singleChar = data["singleChar"].toBool();

  if(data["normal"].isBool())
    normal = data["normal"].toBool();
}

void FontDBEntry::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<FontDBEntry>(
        "PSE.DB.FontDBEntry", 1, 0, "FontDBEntry", "Can't instantiate in QML");
  once = true;
}
