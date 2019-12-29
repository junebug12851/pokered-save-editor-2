/*Copyright 2019 June Hanabi

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include <QJSValue>
#include <QHash>

class GameData : public QObject
{
  Q_OBJECT

public:
  explicit GameData(QObject *parent = nullptr);
  Q_INVOKABLE static QString json(QString filename);

  // Registers this as a singleton to QML
  static QObject* GameData_Provider(QQmlEngine *engine, QJSEngine *scriptEngine);

  // Stores cached file contents so that it doesn't have to re-read them
  // from disk on every access
  // Obviosuly very large so we use the heap
  static QHash<QString, QString>* cache;
};

#endif // GAMEDATA_H