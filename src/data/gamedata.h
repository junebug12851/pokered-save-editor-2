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
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <QHash>
#include <QJsonDocument>

class GameData
{
public:
  // Retrieves JSON document from cache and if a cache miss then from disk
  // and perma caches it.
  // Give the name of the file in /assets/data without the .json file extension
  static QJsonDocument* json(QString filename);

private:
  // Stores cached file contents so that it doesn't have to re-read them
  // from disk on every access
  // The cache contents can and are modified by other code, this is just
  // to simplify things and cut down on memory usage
  // The last thing we want is copies of stuff everywhere in RAM
  static QHash<QString, QJsonDocument*>* cache;
};

#endif // GAMEDATA_H
