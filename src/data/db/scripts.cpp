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

#include "./scripts.h"
#include "./gamedata.h"

void ScriptsDB::load()
{
  // Grab Event Pokemon Data
  auto scriptData = GameData::json("scripts");

  // Go through each event Pokemon
  for(QJsonValue scriptEntry : scriptData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new ScriptDBEntry();

    // Set simple properties
    entry->name = scriptEntry["name"].toString();
    entry->ind = scriptEntry["ind"].toDouble();
    entry->size = scriptEntry["size"].toDouble();

    if(scriptEntry["skip"].isDouble())
      entry->skip = scriptEntry["skip"].toDouble();

    // Add to array
    store.append(entry);
  }

  delete scriptData;
}

void ScriptsDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(QString::number(entry->ind), entry);
  }
}

QVector<ScriptDBEntry*> ScriptsDB::store = QVector<ScriptDBEntry*>();
QHash<QString, ScriptDBEntry*> ScriptsDB::ind = QHash<QString, ScriptDBEntry*>();
