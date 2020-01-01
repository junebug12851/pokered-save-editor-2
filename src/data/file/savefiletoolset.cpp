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
#include "savefiletoolset.h"
#include "savefile.h"
#include "../db/fonts.h"

SaveFileToolset::SaveFileToolset(SaveFile* newSaveFile)
{
  saveFile = newSaveFile;
}

var16 SaveFileToolset::bcd2int(QVector<var8> bcd)
{
  var16 n = 0;
  var16 m = 1;
  for (var8 i = 0; i < bcd.size(); i += 1) {
    n += (bcd[bcd.size() - 1 - i] & 0x0F) * m;
    n += ((bcd[bcd.size() - 1 - i] >> 4) & 0x0F) * m * 10;
    m *= 100;
  }
  return n;
}

QVector<var8> SaveFileToolset::number2bcd(var16 number, var8 size)
{
  var16 s = size; // Rough conversion from it's Javascript counterpart
  QVector<var8> bcd = QVector<var8>(s);
  bcd.fill(0);
  while (number != 0 && s != 0) {
    s -= 1;
    bcd[s] = (number % 10);
    number = (number / 10) | 0;
    bcd[s] += (number % 10) << 4;
    number = (number / 10) | 0;
  }
  return bcd;
}

QVector<var8> SaveFileToolset::getRange(var16 from, var16 size, bool reverse)
{
  QVector<var8> ret;
  var8* data = saveFile->data;

  if(!reverse)
    for(var16 i = from; i < from + size; i++)
      ret.append(data[i]);
  else
    for(var16 i = from + size - 1; i >= from; i--)
      ret.append(data[i]);

  return ret;
}

QString SaveFileToolset::getStr(var16 addr, var16 size, var8 maxLen)
{
  return Font::convertFromCode(getRange(addr, size), maxLen);
}
