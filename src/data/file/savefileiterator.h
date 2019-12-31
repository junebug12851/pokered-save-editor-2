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
#ifndef SAVEFILEITERATOR_H
#define SAVEFILEITERATOR_H

#include "../../common/types.h"
#include <QVector>

// To prevent cross-include and thus errors
class SaveFile;

class SaveFileIterator
{
public:
  SaveFileIterator();

  // Slew of methods that wrap around SaveFile methods
  // SaveFileIterator* offsetTo(var16 val);

  // Current Offset in Save File
  // Can be freely changed directly
  var16 offset;

protected:
  // Allows saving places in the save file and goin back to them
  QVector<var16>* state;

  // The Save File
  SaveFile* saveFile;
};

#endif // SAVEFILEITERATOR_H
