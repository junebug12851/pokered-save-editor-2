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
#include "fonts.h"
#include <QVector>
#include <QJsonArray>
#include "./gamedata.h"

FontEntry::FontEntry()
{
  shorthand = false;
  picture = false;
  useTilemap = false;
  control = false;
  multiChar = false;
  variable = false;
  singleChar = false;
  normal = false;
}

void Font::load()
{
  // Grab Event Pokemon Data
  auto fontData = GameData::json("font");

  // Go through each event Pokemon
  for(QJsonValue fontEntry : fontData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new FontEntry();

    // Set simple properties
    entry->name = fontEntry["name"].toString();
    entry->ind = fontEntry["ind"].toDouble();
    entry->length = fontEntry["length"].toDouble();

    // Set simple optional properties
    if(fontEntry["shorthand"].isBool())
      entry->shorthand = fontEntry["shorthand"].toBool();

    if(fontEntry["picture"].isBool())
      entry->picture = fontEntry["picture"].toBool();

    if(fontEntry["useTilemap"].isBool())
      entry->useTilemap = fontEntry["useTilemap"].toBool();

    if(fontEntry["control"].isBool())
      entry->control = fontEntry["control"].toBool();

    if(fontEntry["multiChar"].isBool())
      entry->multiChar = fontEntry["multiChar"].toBool();

    if(fontEntry["variable"].isBool())
      entry->variable = fontEntry["variable"].toBool();

    if(fontEntry["singleChar"].isBool())
      entry->singleChar = fontEntry["singleChar"].toBool();

    if(fontEntry["normal"].isBool())
      entry->normal = fontEntry["normal"].toBool();

    // Add to array
    font->append(entry);
  }
}

void Font::index()
{
  for(auto entry : *font)
  {
    // Index name and index
    ind->insert(entry->name, entry);
    ind->insert(QString::number(entry->ind), entry);
  }
}

QVector<FontEntry*>* Font::font = new QVector<FontEntry*>();
auto Font::ind = new QHash<QString, FontEntry*>();
