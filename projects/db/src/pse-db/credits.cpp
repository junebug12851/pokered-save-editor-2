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

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValueRef>

#include "./credits.h"
#include "./gamedata.h"

CreditDBEntry::CreditDBEntry() {}

CreditDBEntry::CreditDBEntry(QJsonValue& data)
{
  name = data["name"].toString("");
  url = data["url"].toString("");
  note = data["note"].toString("");
  license = data["license"].toString("");
  mandated = data["mandated"].toString("");
}

CreditDBEntry::CreditDBEntry(QString section)
{
  this->section = section;
}

void CreditDBEntry::process(QJsonObject& data)
{
  CreditsDB::store.append(new CreditDBEntry("Project Leaders"));

  for(QJsonValue entry : data["Project Leaders"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Data Sources"));

  for(QJsonValue entry : data["Data Sources"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Framework"));

  for(QJsonValue entry : data["Framework"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Tools Used"));

  for(QJsonValue entry : data["Tools Used"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Services Used"));

  for(QJsonValue entry : data["Services Used"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Icons"));

  for(QJsonValue entry : data["Icons"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));

  CreditsDB::store.append(new CreditDBEntry("Wallpapers"));

  for(QJsonValue entry : data["Wallpapers"].toArray())
    CreditsDB::store.append(new CreditDBEntry(entry));
}

void CreditsDB::load()
{
  // Grab Event Pokemon Data
  auto jsonData = GameData::json("credits");
  auto obj = jsonData->object();

  // Create a entry
  CreditDBEntry::process(obj);

  delete jsonData;
}

QVector<CreditDBEntry*> CreditsDB::store = QVector<CreditDBEntry*>();
