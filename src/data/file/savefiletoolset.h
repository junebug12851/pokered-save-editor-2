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
  var16 bcd2int(QVector<var8> val);

  /*
   * number2bcd -> takes a number and returns the corresponding BCD in an array
   * input: 16 bit positive number, array size (How many bytes)
   * output: array
   * credit: joaomaia @ https://gist.github.com/joaomaia/3892692
  */
  QVector<var8> number2bcd(var16 number, var8 size = 2);

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

protected:
  SaveFile* saveFile;
};

#endif // SAVEFILETOOLSET_H
