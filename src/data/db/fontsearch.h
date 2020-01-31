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
#ifndef FONTSEARCH_H
#define FONTSEARCH_H

#include <QObject>
#include <QVector>
#include <QString>
class FontsDB;
class FontDBEntry;

class FontSearch : public QObject
{
  Q_OBJECT

public:
  FontSearch();

  Q_INVOKABLE FontSearch* startOver();
  Q_INVOKABLE FontSearch* andShorthand();
  Q_INVOKABLE FontSearch* notShorthand();
  Q_INVOKABLE FontSearch* andNormal();
  Q_INVOKABLE FontSearch* notNormal();
  Q_INVOKABLE FontSearch* andControl();
  Q_INVOKABLE FontSearch* notControl();
  Q_INVOKABLE FontSearch* andPicture();
  Q_INVOKABLE FontSearch* notPicture();
  Q_INVOKABLE FontSearch* andSingleChar();
  Q_INVOKABLE FontSearch* notSingleChar();
  Q_INVOKABLE FontSearch* andMultiChar();
  Q_INVOKABLE FontSearch* notMultiChar();
  Q_INVOKABLE FontSearch* andVariable();
  Q_INVOKABLE FontSearch* notVariable();
  Q_INVOKABLE FontSearch* andTilemap();
  Q_INVOKABLE FontSearch* notTilemap();

  // QML Interface
  Q_INVOKABLE int fontCount();
  Q_INVOKABLE QString fontStrAt(int ind);

  QVector<FontDBEntry*> results = QVector<FontDBEntry*>();
};

#endif // FONTSEARCH_H
