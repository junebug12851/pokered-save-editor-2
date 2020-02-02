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

#include <QByteArray>
#include <QStringList>
#include "./utility.h"

QString Utility::encodeBeforeUrl(QString beforeStr)
{
  QStringList numberString;
  for(const auto character: beforeStr){
      numberString << QString::number(character.unicode(), 16);
  }

  return numberString.join(" ");
}

QString Utility::decodeAfterUrl(QString beforeStr)
{
  return QByteArray::fromHex(beforeStr.remove(" ").toLocal8Bit());
}
