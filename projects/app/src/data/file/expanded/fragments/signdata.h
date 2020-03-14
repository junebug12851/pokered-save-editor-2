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
#ifndef SIGNDATA_H
#define SIGNDATA_H

#include <QObject>
#include <QVector>
#include "../../../../common/types.h"
class SaveFile;
class MapDBEntrySign;
struct TmpSignPos;

class SignData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int x MEMBER x NOTIFY xChanged)
  Q_PROPERTY(int y MEMBER y NOTIFY yChanged)
  Q_PROPERTY(int txtId MEMBER txtId NOTIFY txtIdChanged)

public:
  SignData(SaveFile* saveFile = nullptr, var8 index = 0);
  virtual ~SignData();

  void load(SaveFile* saveFile = nullptr, var8 index = 0);
  void save(SaveFile* saveFile, var8 index);

signals:
  void xChanged();
  void yChanged();
  void txtIdChanged();

public slots:
  void reset();
  void randomize(QVector<TmpSignPos*>* tmpPos = nullptr);
  static QVector<SignData*> randomizeAll(QVector<MapDBEntrySign*> mapSigns);

  void setTo(MapDBEntrySign* signData);
  static QVector<SignData*> setToAll(QVector<MapDBEntrySign*> mapSigns);

public:
  int x;
  int y;
  int txtId;
};

#endif // SIGNDATA_H
