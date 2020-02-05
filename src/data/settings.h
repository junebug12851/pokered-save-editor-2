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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

class Settings : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int headerHeight MEMBER headerHeight NOTIFY headerHeightChanged)
  Q_PROPERTY(bool infoBtnPressed MEMBER infoBtnPressed NOTIFY infoBtnPressedChanged)
  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
  Q_PROPERTY(QString previewTileset MEMBER previewTileset NOTIFY previewTilesetChanged)
  Q_PROPERTY(bool previewOutdoor MEMBER previewOutdoor NOTIFY previewOutdoorChanged)

signals:
  void headerHeightChanged();
  void infoBtnPressedChanged();
  void titleChanged();
  void previewTilesetChanged();
  void previewOutdoorChanged();

public:
  int headerHeight = 60;

  bool infoBtnPressed = false;
  QString title = "";

  // Tileset and related engine for naming previews
  QString previewTileset = "";
  bool previewOutdoor = false;
};

#endif // SETTINGS_H
