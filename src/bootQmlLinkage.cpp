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

#include <QString>
#include <QQmlEngine>

#include "./data/file/filemanagement.h"
#include "./data/file/savefile.h"
#include "./data/file/expanded/daycare.h"
#include "./data/file/expanded/halloffame.h"
#include "./data/file/expanded/rival.h"
#include "./data/file/expanded/savefileexpanded.h"
#include "./data/file/expanded/storage.h"

constexpr const char* msg = "Can't instantiate in QML";

const char* dn(QString name)
{
  return ("app." + name.toLower()).toLocal8Bit().data();
}

extern void bootQmlLinkage()
{
  // Can't put this into a template because there would be no QML processing
  // for hints so I have to duplicate the class name 3 times on each line
  qmlRegisterUncreatableType<FileManagement>(dn("FileManagement"), 1, 0, "FileManagement", msg);
  qmlRegisterUncreatableType<SaveFile>(dn("SaveFile"), 1, 0, "SaveFile", msg);
  qmlRegisterUncreatableType<Daycare>(dn("Daycare"), 1, 0, "Daycare", msg);
  qmlRegisterUncreatableType<HallOfFame>(dn("HallOfFame"), 1, 0, "HallOfFame", msg);
  qmlRegisterUncreatableType<Rival>(dn("Rival"), 1, 0, "Rival", msg);
  qmlRegisterUncreatableType<SaveFileExpanded>(dn("SaveFileExpanded"), 1, 0, "SaveFileExpanded", msg);
  qmlRegisterUncreatableType<Storage>(dn("Storage"), 1, 0, "Storage", msg);
}
