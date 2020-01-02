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
#ifndef SAVEFILETOOLSET_H
#define SAVEFILETOOLSET_H

#include "../../common/types.h"
#include <QVector>
#include <QString>

class SaveFile;

// Tools to operate on raw save file data
class SaveFileToolset
{
public:
  SaveFileToolset(SaveFile* newSaveFile);

  /*
   * bcd2number -> takes an array with a BCD and returns the corresponding number.
   * input: array
   * output: number
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
  */
  var32 bcd2int(QVector<var8> val);

  /*
   * number2bcd -> takes a number and returns the corresponding BCD in an array
   * input: 16 bit positive number, array size (How many bytes)
   * output: array
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
  */
  QVector<var8> int2bcd(var32 number, var8 size = 2);

  // Copies a range of bytes to a buffer of size and returns them
  QVector<var8> getRange(var16 from, var16 size, bool reverse = false);

  // Copies data to the save data at a particular place going no further
  // than the maximum size desired to be copied and/or max array size
  //
  // from = index to start copying at inclusive
  // size = maximum length to copy
  // data = array of data to copy into, will stop at size or data length
  // reverse = reverse copies the data into specified location
  void copyRange(var16 addr, var16 size, QVector<var8> data, bool reverse = false);

  // Gets a string from the sav file, converted from in-game font encoding
  // to UTF-8 for easy reading and manipulation
  QString getStr(var16 addr, var16 size, var8 maxLen);

  // Sets a string to the sav file, converted from UTF-8 front encoding to
  // in-game font encoding
  void setStr(var16 addr, var16 size, var8 maxLen, QString str);

  // Gets a value in hex from the save file
  QString getHex(var16 addr, var16 size, bool reverse = false);

  // Saves a hex value to bytes in the save file
  void setHex(var16 addr, var16 size, QString hex, bool reverse = false);

  // Get a raw BCD value as a number
  // Casino coins are 16-bit BCD meaning the maximum is 9,999
  // Player money is 24-bit BCD meaning the maximum is 999,999
  var32 getBCD(var16 addr, var8 size);

  // Set a raw BCD val from a given number
  // Casino coins are 16-bit BCD meaning the maximum is 9,999
  // Player money is 24-bit BCD meaning the maximum is 999,999
  void setBCD(var16 addr, var8 size, var32 val);

  // Get a bit from a value
  // Here for conv. but it's actually simpler just to test it yourself given
  // this is so complicated and expensive for bit testing
  // This method can only bit test a bit up to 4 bytes
  bool getBit(var16 addr, var8 size, var8 bit, bool reverse = false);

  // Set a bit into a value
  // Here for conv. but it's actually simpler just to set it yourself given
  // this is so complicated and expensive for bit setting
  // This method can only bit set a bit up to 4 bytes
  void setBit(var16 addr, var8 size, var8 bit, bool value, bool reverse = false);

  // Get and set 16-bit values
  // 0x12,0x34 <=> 0x1234
  // If you want the reverse then reverse it
  var16 getWord(var16 addr, bool reverse = false);
  void setWord(var16 addr, var16 val, bool reverse = false);

  // Simply gets or sets a byte
  var8 getByte(var16 addr);
  void setByte(var16 addr, var8 val);

  // Gets a single checksum from a given range
  // This properly calculates the checksum that Gen 1 games expect
  var8 getChecksum(var16 addr, var16 size);

  // Recalculates the checksum for all the boxes
  // This only needs to be called when the boxes checksums are used
  // The game doesn't always use the box checksums or calculate them
  // Use recalcChecksums below to have them calculated as approrpiately
  void recalcBoxesChecksums();

  // Recalculates all checksums in all banks following the very strict rule
  // of only calculating what's needed and when. If the game will never
  // calculate the box checksums then neither will we. There are cases when it
  // won't. This also calculates bank 1 thus covering all checksums in the file.
  void recalcChecksums(bool force = false);

protected:
  SaveFile* saveFile;
};

#endif // SAVEFILETOOLSET_H
