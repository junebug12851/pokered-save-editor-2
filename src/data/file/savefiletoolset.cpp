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
  return FontsDB::convertFromCode(getRange(addr, size), maxLen);
}

void SaveFileToolset::setStr(var16 addr, var16 size, var8 maxLen, QString str)
{
  auto strCode = FontsDB::convertToCode(str, maxLen);
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

var8 SaveFileToolset::getByte(var16 addr)
{
  return saveFile->data[addr];
}

void SaveFileToolset::setByte(var16 addr, var8 val)
{
  saveFile->data[addr] = val;
}

QVector<bool> SaveFileToolset::getBitField(var16 addr, var16 size)
{
  QVector<bool> ret;

  // Basically extract all the bits in a bitfield of a given size and dump them
  // in the vector
  for(var16 i = 0; i < size; i++) {
    ret.append(getBit(addr, 1, 0));
    ret.append(getBit(addr, 1, 1));
    ret.append(getBit(addr, 1, 2));
    ret.append(getBit(addr, 1, 3));
    ret.append(getBit(addr, 1, 4));
    ret.append(getBit(addr, 1, 5));
    ret.append(getBit(addr, 1, 6));
    ret.append(getBit(addr, 1, 7));

    addr++;
  }

  // Return that
  return ret;
}

void SaveFileToolset::setBitField(var16 addr, var16 size, QVector<bool> src)
{
  // The opposite
  // Dump all the bits one-by-one from the bit field into a given area
  // We perform extra checks to make sure we're not extending past the given
  // src array which is more complicated when working with a bit-field
  for(var16 i = 0; i < size * 8 && i < src.size(); i +=8 ) {
    if(src.size() < (i + 0))
      setBit(addr, 1, 0, src.at(i + 0));
    if(src.size() < (i + 1))
      setBit(addr, 1, 1, src.at(i + 1));
    if(src.size() < (i + 2))
      setBit(addr, 1, 2, src.at(i + 2));
    if(src.size() < (i + 3))
      setBit(addr, 1, 3, src.at(i + 3));
    if(src.size() < (i + 4))
      setBit(addr, 1, 4, src.at(i + 4));
    if(src.size() < (i + 5))
      setBit(addr, 1, 5, src.at(i + 5));
    if(src.size() < (i + 6))
      setBit(addr, 1, 6, src.at(i + 6));
    if(src.size() < (i + 7))
      setBit(addr, 1, 7, src.at(i + 7));

    addr++;
  }
}

var8 SaveFileToolset::getChecksum(var16 addr, var16 size)
{
  // Get the bytes to checksum
  auto toChecksum = getRange(addr, size);

  // This is the checksum to return, it's always 8-bit and uses a more efficient
  // algorithm which is to start at 255. The actual game uses a less efficient
  // version that starts at 0 and requires some more steps.
  var8 checksum = 0xFF;

  // Gen 1 games were super simple especially with the improved algorithm
  // Just subtract from 255 each byte in order, underflowing back around to 255
  // and onward til you're done and then you have the checksum gen 1 games will
  // accept
  for (var16 i = 0; i < toChecksum.length(); i++) {
    checksum -= toChecksum.at(i);
  }

  // Return that checksum
  return checksum;
}

void SaveFileToolset::recalcBoxesChecksums()
{
  // Bank 2 individual checksums for Boxes 1-6
  QVector<var8> bank2IndvChecksums;
  bank2IndvChecksums.append(getChecksum(0x4000, 0x462));
  bank2IndvChecksums.append(getChecksum(0x4462, 0x462));
  bank2IndvChecksums.append(getChecksum(0x48C4, 0x462));
  bank2IndvChecksums.append(getChecksum(0x4D26, 0x462));
  bank2IndvChecksums.append(getChecksum(0x5188, 0x462));
  bank2IndvChecksums.append(getChecksum(0x55EA, 0x462));

  // Apply the individual checksums for boxes 1-6 in bank 2
  copyRange(0x5A4D, 0x6, bank2IndvChecksums);

  // Apply the checksum that covers all of bank 2, excluding individual
  // checksums
  saveFile->data[0x5A4C] = getChecksum(0x4000, 0x1A4C);

  // Bank 3 Checksums for Boxes 7-12
  // Same as bank 2, apply values and such the same way
  // Bank 2 and 3 are 100% identical in every way just duplicates for more
  // storage
  QVector<var8> bank3IndvChecksums;
  bank2IndvChecksums.append(getChecksum(0x6000, 0x462));
  bank2IndvChecksums.append(getChecksum(0x6462, 0x462));
  bank2IndvChecksums.append(getChecksum(0x68C4, 0x462));
  bank2IndvChecksums.append(getChecksum(0x6D26, 0x462));
  bank2IndvChecksums.append(getChecksum(0x7188, 0x462));
  bank2IndvChecksums.append(getChecksum(0x75EA, 0x462));

  copyRange(0x7A4D, 0x6, bank3IndvChecksums);

  // Bank 3 Checksum
  saveFile->data[0x7A4C] = getChecksum(0x6000, 0x1A4C);
}

void SaveFileToolset::recalcChecksums(bool force)
{
  // Apply Bank 1 Checksum
  saveFile->data[0x3523] = getChecksum(0x2598, 0xF8B);

  // Has the player even switched boxes before?
  // If not, then the game hasn't even formatted them yet and there's no need
  // to calculate checksum as that breaks the critical rule of not changing
  // what doesn't need to be changed
  bool boxesFormatted = (saveFile->data[0x284C] & 0b10000000) > 0;

  // Recalculate box checksums if needed or forced to
  if (boxesFormatted || force)
    recalcBoxesChecksums();
}
