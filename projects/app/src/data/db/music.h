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
#ifndef MUSIC_H
#define MUSIC_H

#include <QJsonValue>
#include <QVector>
#include <QString>
#include <QHash>

#include "../../common/types.h"

struct MapDBEntry;

struct MusicDBEntry {
  MusicDBEntry();
  MusicDBEntry(QJsonValue& data);

  QString name;
  var8 bank = 0;
  var8 id = 0;

  QVector<MapDBEntry*> toMaps;
};

class MusicDB
{
public:
  static void load();
  static void index();

  static QVector<MusicDBEntry*> store;
  static QHash<QString, MusicDBEntry*> ind;
};

#endif // MUSIC_H
