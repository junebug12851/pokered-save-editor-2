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
#ifndef WORLDHIDDEN_H
#define WORLDHIDDEN_H

#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

// There's actually significantly more hidden item bits, 112 in total forming
// 14 bytes. But given they are all unused it makes no sense to load entire
// unused bytes
constexpr var8 hiddenItemCount = 54;
constexpr var8 hiddenCoinCount = 12;
constexpr var8 hiddenItemByteCount = 7; // 2 Bits of 56 unused
constexpr var8 hiddenCoinByteCount = 2; // 4 Bits of 16 unused

class SAVEFILE_AUTOPORT WorldHidden : public QObject
{
  Q_OBJECT

public:
  WorldHidden(SaveFile* saveFile = nullptr);
  virtual ~WorldHidden();

  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);

  Q_INVOKABLE int hItemsCount();
  Q_INVOKABLE bool hItemsAt(int ind);
  Q_INVOKABLE void hItemsSet(int ind, bool val);

  Q_INVOKABLE int hCoinsCount();
  Q_INVOKABLE bool hCoinsAt(int ind);
  Q_INVOKABLE void hCoinsSet(int ind, bool val);

signals:
  void hiddenItemsChanged();
  void hiddenCoinsChanged();

public slots:
  void reset();
  void randomize();

public:
  bool hiddenItems[hiddenItemCount];
  bool hiddenCoins[hiddenCoinCount];
};

#endif // WORLDHIDDEN_H
