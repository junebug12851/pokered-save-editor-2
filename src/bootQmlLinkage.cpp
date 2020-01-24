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

template<class T>
void doRegister(QString name)
{
  auto lowerName = "app." + name.toLower();

  auto _name = name.toLocal8Bit().data();
  auto _lowerName = lowerName.toLocal8Bit().data();

  qmlRegisterUncreatableType<T>(
        _lowerName,
        1, 0,
        _name,
        "Can't instantiate in QML");
}

extern void bootQmlLinkage()
{
  doRegister<FileManagement>("FileManagement");
  doRegister<SaveFile>("SaveFile");
}
