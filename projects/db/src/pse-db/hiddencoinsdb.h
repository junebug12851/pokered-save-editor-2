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
#ifndef HIDDENCOINSDB_H
#define HIDDENCOINSDB_H

#include <QObject>
#include "./abstracthiddenitemdb.h"

class HiddenCoinsDB : public AbstractHiddenItemDB
{
  Q_OBJECT

public:
  static HiddenCoinsDB* inst();

protected slots:
  virtual void qmlRegister() const;

protected:
  HiddenCoinsDB();
};

#endif // HIDDENCOINSDB_H
