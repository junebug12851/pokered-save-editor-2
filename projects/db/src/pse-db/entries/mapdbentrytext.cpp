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

/**
 * @file mapdbentrytext.cpp
 * @brief Implementation of MapDBEntryText -- one row of a map's text-pointer table.
 *        See mapdbentrytext.h.
 */

#include <QQmlEngine>
#include <pse-common/utility.h>
#include "mapdbentrytext.h"

MapDBEntryText::MapDBEntryText() {
  qmlRegister();
}

MapDBEntryText::MapDBEntryText(const QJsonValue& data, MapDBEntry* const parent) :
  parent(parent)
{
  qmlRegister();

  id = data["id"].toDouble();
  category = data["category"].toString("other");

  // "string": null in JSON == a text_asm script with no single literal.
  const QJsonValue str = data["string"];
  if(str.isString()) {
    text = str.toString();
    scripted = false;
  }
  else {
    text = "";
    scripted = true;
  }
}

int MapDBEntryText::getId() const
{
  return id;
}

const QString MapDBEntryText::getText() const
{
  return text;
}

bool MapDBEntryText::getScripted() const
{
  return scripted;
}

const QString MapDBEntryText::getCategory() const
{
  return category;
}

MapDBEntry* MapDBEntryText::getParent() const
{
  return parent;
}

void MapDBEntryText::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void MapDBEntryText::qmlRegister() const
{
  static bool once = false;
  if(once)
    return;

  qmlRegisterUncreatableType<MapDBEntryText>(
        "PSE.DB.MapDBEntryText", 1, 0, "MapDBEntryText", "Can't instantiate in QML");
  once = true;
}
