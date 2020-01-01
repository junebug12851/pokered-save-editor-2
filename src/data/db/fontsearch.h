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

#include <QVector>
#include <QString>
class Font;
class FontEntry;

class FontSearch
{
public:
  FontSearch();

  FontSearch* andShorthand();
  FontSearch* notShorthand();
  FontSearch* andNormal();
  FontSearch* notNormal();
  FontSearch* andControl();
  FontSearch* notControl();
  FontSearch* andPicture();
  FontSearch* notPicture();
  FontSearch* andSingleChar();
  FontSearch* notSingleChar();
  FontSearch* andMultiChar();
  FontSearch* notMultiChar();
  FontSearch* andVariable();
  FontSearch* notVariable();
  FontSearch* andTilemap();
  FontSearch* notTilemap();

  QVector<FontEntry*>* results = new QVector<FontEntry*>();
};

#endif // FONTSEARCH_H
