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

#include "creditdbentry.h"
#include "../creditsdb.h"
#include "../db.h"

CreditDBEntry::CreditDBEntry() {
  engineRegister();
}

CreditDBEntry::CreditDBEntry(QJsonValue& data)
{
  engineRegister();

  name = data["name"].toString("");
  url = data["url"].toString("");
  note = data["note"].toString("");
  license = data["license"].toString("");
  mandated = data["mandated"].toString("");
}

CreditDBEntry::CreditDBEntry(QString section)
{
  engineRegister();

  this->section = section;
}

QString CreditDBEntry::getMandated() const
{
  return mandated;
}

void CreditDBEntry::engineProtect(const QQmlEngine* const engine) const
{
  DB::engineProtectUtil(this, engine);
}

QString CreditDBEntry::getLicense() const
{
  return license;
}

QString CreditDBEntry::getNote() const
{
  return note;
}

QString CreditDBEntry::getUrl() const
{
  return url;
}

QString CreditDBEntry::getName() const
{
  return name;
}

QString CreditDBEntry::getSection() const
{
  return section;
}

void CreditDBEntry::process(QJsonObject& data)
{
  CreditsDB::inst()->store.append(new CreditDBEntry("Project Leaders"));

  for(QJsonValue entry : data["Project Leaders"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Data Sources"));

  for(QJsonValue entry : data["Data Sources"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Framework"));

  for(QJsonValue entry : data["Framework"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Tools Used"));

  for(QJsonValue entry : data["Tools Used"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Services Used"));

  for(QJsonValue entry : data["Services Used"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Icons"));

  for(QJsonValue entry : data["Icons"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));

  CreditsDB::inst()->store.append(new CreditDBEntry("Wallpapers"));

  for(QJsonValue entry : data["Wallpapers"].toArray())
    CreditsDB::inst()->store.append(new CreditDBEntry(entry));
}

void CreditDBEntry::engineRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<CreditDBEntry>("PSE.DB.CreditDBEntry", 1, 0, "CreditDBEntry", "Can't instantiate in QML");
  registered = true;
}
