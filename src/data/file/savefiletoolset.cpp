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

var32 SaveFileToolset::bcd2int(QVector<var8> bcd)
{
  var32 n = 0;
  var32 m = 1;
  for (var8 i = 0; i < bcd.size(); i += 1) {
    n += (bcd[bcd.size() - 1 - i] & 0x0F) * m;
    n += ((bcd[bcd.size() - 1 - i] >> 4) & 0x0F) * m * 10;
    m *= 100;
  }
  return n;
}

QVector<var8> SaveFileToolset::int2bcd(var32 number, var8 size)
{
  var32 s = size; // Rough conversion from it's Javascript counterpart
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

void SaveFileToolset::copyRange(var16 addr, var16 size, QVector<var8> newData, bool reverse)
{
  var8* data = saveFile->data;

  if(!reverse)
    for (var16 i = addr, j = 0;
         j < newData.length() && i < (addr + size); i++, j++)
      data[i] = newData[j];
  else
    for (var16 i = addr + size - 1, j = 0;
         j < newData.length() && i >= addr; i--, j++)
      data[i] = newData[j];
}

QString SaveFileToolset::getStr(var16 addr, var16 size, var8 maxLen)
{
  return Font::convertFromCode(getRange(addr, size), maxLen);
}

void SaveFileToolset::setStr(var16 addr, var16 size, var8 maxLen, QString str)
{
  auto strCode = Font::convertToCode(str, maxLen);
  copyRange(addr, size, strCode);
}

QString SaveFileToolset::getHex(var16 addr, var16 size, bool reverse)
{
  QVector<var8> rawHex = getRange(addr, size, reverse);

  QString hexStr = "";
  for (var16 i = 0; i < rawHex.length(); i++)
    hexStr += QString::number(rawHex[i], 16);

  hexStr = hexStr.toUpper();

  // Fixes the issue where a 16-bit hex value of 1 should be 0x0001 not 0x1
  // or a 16-bit val of 0xCAD should be 0x0CAD
  // Properly sizes the end result
  var8 endLen = size * 2; // A byte is represented by 2 hex characters
  if(hexStr.length() < endLen)
    hexStr = hexStr.rightJustified(endLen, '0');

  return hexStr;
}

void SaveFileToolset::setHex(var16 addr, var16 size, QString hex, bool reverse)
{
  // Convert to number
  var16 hexValue = hex.toInt(nullptr, 16);
  var16 hexValueOrig = hexValue;
  QVector<var8> hexArr;

  // Break apart number into seperate bytes and store them in an
  // array. This also places it big-endian style which is how the
  // save data is structured

  if (hexValue == 0)
    hexArr.append(0);
  else
    while (hexValue > 0) {
      hexArr.append(hexValue & 0xFF);
      hexValue >>= 8;
    }

  // Account for bug where 16-bit value under 0xFF still needs to take up
  // 16-bits
  if (hexValueOrig <= 0xFF && size == 2)
    hexArr.append(0x00);

  // Copy to save data
  copyRange(addr, size, hexArr, !reverse);
}

var32 SaveFileToolset::getBCD(var16 addr, var8 size)
{
  return bcd2int(getRange(addr, size));
}

void SaveFileToolset::setBCD(var16 addr, var8 size, var32 val)
{
  copyRange(addr, size, int2bcd(val, size));
}

bool SaveFileToolset::getBit(var16 addr, var8 size, var8 bit, bool reverse)
{
  // Get bytes first
  QVector<var8> value = getRange(addr, size, reverse);

  // Merge together into a single data type
  // This method cannot test larger than this which is 32-bits or 4-bytes
  var32 res = value.at(0);

  if (size > 0)
    for (var8 i = 1; i < size; i++) {
      res <<= 8;
      res |= value[i];
    }

  // Do the test
  return (res & (1 << bit)) != 0;
}

void SaveFileToolset::setBit(var16 addr, var8 size, var8 bit, bool value, bool reverse)
{
  // Get bytes first
  QVector<var8> val = getRange(addr, size, reverse);

  // Merge together into a single data type
  // This method cannot test larger than this which is 32-bits or 4-bytes
  var32 res = val.at(0);

  if (size > 0)
    for (var8 i = 1; i < size; i++) {
      res <<= 8;
      res |= val[i];
    }

  // Set
  if(value)
    res |= (1 << bit);
  else
    res &= ~(1 << bit);

  // Convert to hex and do a hex insert
  QString hex = QString::number(res, 16);
  setHex(addr, size, hex, reverse);
}

var16 SaveFileToolset::getWord(var16 addr, bool reverse)
{
  auto value = getRange(addr, 2, reverse);

  var16 res = value[0];
  res <<= 8;
  res |= value[1];

  return res;
}

void SaveFileToolset::setWord(var16 addr, var16 val, bool reverse)
{
  var8 byteL = val & 0x00FF;
  var8 byteH = (val & 0xFF00) >> 8;

  QVector<var8> tmp;
  tmp.append(byteH);
  tmp.append(byteL);

  copyRange(addr, 2, tmp, reverse);
}
