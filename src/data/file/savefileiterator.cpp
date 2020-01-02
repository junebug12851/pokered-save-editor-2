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
#include "savefileiterator.h"
#include "savefile.h"
#include "savefiletoolset.h"

SaveFileIterator::SaveFileIterator(SaveFile* saveFile)
{
  this->saveFile = saveFile;
}

SaveFileIterator* SaveFileIterator::offsetTo(var16 val)
{
  offset = val;
  return this;
}

SaveFileIterator* SaveFileIterator::offsetBy(var16 val)
{
  offset += val;
  return this;
}

SaveFileIterator* SaveFileIterator::skipPadding(var16 val)
{
  return offsetBy(val);
}

SaveFileIterator* SaveFileIterator::inc()
{
  offset++;
  return this;
}

SaveFileIterator* SaveFileIterator::dec()
{
  offset--;
  return this;
}

SaveFile* SaveFileIterator::file()
{
  return saveFile;
}

SaveFileToolset* SaveFileIterator::toolset()
{
  return saveFile->toolset;
}

SaveFileIterator* SaveFileIterator::push()
{
  state->append(offset);
  return this;
}

SaveFileIterator* SaveFileIterator::pop()
{
  // Default to Offset 0x0000
  var16 val = 0x0000;

  // If not empty then pop off the address and use that
  if(!state->isEmpty())
    val = state->takeFirst();

  offset = val;
  return this;
}

QVector<var8> SaveFileIterator::getRange(var16 size, var16 padding, bool reverse)
{
  auto ret = toolset()->getRange(offset, size, reverse);
  offsetBy(size + padding);
  return ret;
}

void SaveFileIterator::copyRange(var16 size, QVector<var8> data, var16 padding, bool reverse)
{
  toolset()->copyRange(offset, size, data, reverse);
  offsetBy(size + padding);
}

QString SaveFileIterator::getStr(var16 size, var8 maxLen, var16 padding)
{
  auto ret = toolset()->getStr(offset, size, maxLen);
  offsetBy(size + padding);
  return ret;
}

void SaveFileIterator::setStr(var16 size, var8 maxLen, QString str, var16 padding)
{
  toolset()->setStr(offset, size, maxLen, str);
  offsetBy(size + padding);
}

QString SaveFileIterator::getHex(var16 size, var16 padding, bool reverse)
{
  auto ret = toolset()->getHex(offset, size, reverse);
  offsetBy(size + padding);
  return ret;
}

void SaveFileIterator::setHex(var16 size, QString hex, var16 padding, bool reverse)
{
  toolset()->setHex(offset, size, hex, reverse);
  offsetBy(size + padding);
}

var32 SaveFileIterator::getBCD(var8 size, var16 padding)
{
  auto ret = toolset()->getBCD(offset, size);
  offsetBy(size + padding);
  return ret;
}

void SaveFileIterator::setBCD(var8 size, var32 val, var16 padding)
{
  toolset()->setBCD(offset, size, val);
  offsetBy(size + padding);
}

bool SaveFileIterator::getBit(var8 size, var8 bit, bool reverse)
{
  return toolset()->
      getBit(offset, size, bit, reverse);
}

void SaveFileIterator::setBit(var8 size, var8 bit, bool value, bool reverse)
{
  return toolset()->
      setBit(offset, size, bit, value, reverse);
}

var16 SaveFileIterator::getWord(var16 padding, bool reverse)
{
  auto ret = toolset()->getWord(offset, reverse);
  offsetBy(2 + padding);
  return ret;
}

void SaveFileIterator::setWord(var16 val, var16 padding, bool reverse)
{
  toolset()->setWord(offset, val, reverse);
  offsetBy(2 + padding);
}

var8 SaveFileIterator::getByte(var16 padding)
{
  auto ret = toolset()->getByte(offset);
  offsetBy(1 + padding);
  return ret;
}

void SaveFileIterator::setByte(var8 val, var16 padding)
{
  toolset()->setByte(offset, val);
  offsetBy(1 + padding);
}
