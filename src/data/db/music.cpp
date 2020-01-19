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

#include "./music.h"
#include "./gamedata.h"

MusicDBEntry::MusicDBEntry() {}
MusicDBEntry::MusicDBEntry(QJsonValue& data)
{
  // Set simple properties
  name = data["name"].toString();
  bank = data["bank"].toDouble();
  id = data["id"].toDouble();
}

void MusicDB::load()
{
  // Grab Music Data
  auto musicData = GameData::json("music");

  // Go through each music
  for(QJsonValue musicEntry : musicData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new MusicDBEntry(musicEntry);

    // Add to array
    store.append(entry);
  }

  delete musicData;
}

void MusicDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(entry->name, entry);
    ind.insert(
          QString::number(entry->bank)+"_"+QString::number(entry->id), entry);
  }
}

QVector<MusicDBEntry*> MusicDB::store = QVector<MusicDBEntry*>();
QHash<QString, MusicDBEntry*> MusicDB::ind = QHash<QString, MusicDBEntry*>();
