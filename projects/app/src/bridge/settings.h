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
#include <QString>
#include <QColor>

class SaveFile;

class Settings : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int headerHeight MEMBER headerHeight NOTIFY headerHeightChanged)
  Q_PROPERTY(int headerShadowHeight MEMBER headerShadowHeight NOTIFY headerShadowHeightChanged)

  Q_PROPERTY(bool infoBtnPressed MEMBER infoBtnPressed NOTIFY infoBtnPressedChanged)
  Q_PROPERTY(QString previewTileset MEMBER previewTileset NOTIFY previewTilesetChanged)
  Q_PROPERTY(bool previewOutdoor MEMBER previewOutdoor NOTIFY previewOutdoorChanged)

  Q_PROPERTY(QColor textColorLight MEMBER textColorLight NOTIFY textColorLightChanged)
  Q_PROPERTY(QColor textColorMid MEMBER textColorMid NOTIFY textColorMidChanged)
  Q_PROPERTY(QColor textColorDark MEMBER textColorDark NOTIFY textColorDarkChanged)
  Q_PROPERTY(QColor primaryColor MEMBER primaryColor NOTIFY primaryColorChanged)
  Q_PROPERTY(QColor primaryColorLight MEMBER primaryColorLight NOTIFY primaryColorLightChanged)
  Q_PROPERTY(QColor primaryColorDark MEMBER primaryColorDark NOTIFY primaryColorDarkChanged)
  Q_PROPERTY(QColor dividerColor MEMBER dividerColor NOTIFY dividerColorChanged)
  Q_PROPERTY(QColor accentColor MEMBER accentColor NOTIFY accentColorChanged)
  Q_PROPERTY(int previewTilesetIndex READ getPreviewTilesetIndex NOTIFY previewTilesetChanged STORED false)

  Q_PROPERTY(QColor fontColorNormal MEMBER fontColorNormal NOTIFY fontColorNormalChanged)
  Q_PROPERTY(QColor fontColorControl MEMBER fontColorControl NOTIFY fontColorControlChanged)
  Q_PROPERTY(QColor fontColorPicture MEMBER fontColorPicture NOTIFY fontColorPictureChanged)
  Q_PROPERTY(QColor fontColorSingle MEMBER fontColorSingle NOTIFY fontColorSingleChanged)
  Q_PROPERTY(QColor fontColorMulti MEMBER fontColorMulti NOTIFY fontColorMultiChanged)
  Q_PROPERTY(QColor fontColorVar MEMBER fontColorVar NOTIFY fontColorVarChanged)

signals:
  void headerShadowHeightChanged();
  void headerHeightChanged();

  void infoBtnPressedChanged();
  void previewTilesetChanged();
  void previewOutdoorChanged();

  void textColorLightChanged();
  void textColorMidChanged();
  void textColorDarkChanged();
  void primaryColorChanged();
  void primaryColorLightChanged();
  void primaryColorDarkChanged();
  void dividerColorChanged();
  void accentColorChanged();

  void fontColorNormalChanged();
  void fontColorControlChanged();
  void fontColorPictureChanged();
  void fontColorSingleChanged();
  void fontColorMultiChanged();
  void fontColorVarChanged();

public:
  Settings(SaveFile* file);

  Q_INVOKABLE void setColorScheme(QColor primary, QColor secondary);

  // Header and Footer height
  int headerHeight = 80;
  int headerShadowHeight = 20;

  // Global Tooltips
  bool infoBtnPressed = false;

  // Tileset and related engine for naming previews
  QString previewTileset = "Overworld";
  bool previewOutdoor = true;
  int getPreviewTilesetIndex();

  // Color Palette
  QColor textColorLight = QColor("#efefef"); //#fafafa
  QColor textColorMid = QColor("#757575");
  QColor textColorDark = QColor("#212121");

  QColor primaryColor = QColor("#d81b60");
  QColor primaryColorLight = QColor("#ff5c8d");
  QColor primaryColorDark = QColor("#a00037");

  QColor dividerColor = QColor("#BDBDBD");
  QColor accentColor = QColor("#607D8B");

  QColor fontColorNormal = QColor("#388E3C"); // Green, Shade 700
  QColor fontColorControl = QColor("#7B1FA2"); // Purple, Shade 700
  QColor fontColorPicture = QColor("#303F9F"); // Blue, Shade 700
  QColor fontColorSingle = QColor("#9E9D24"); // Lime, Shade 800
  QColor fontColorMulti = QColor("#FF6F00"); // Amber, Shade 900
  QColor fontColorVar = QColor("#616161"); // Grey, Shade 700

protected slots:
  void dataChanged();

protected:
  SaveFile* file;
};

#endif // SETTINGS_H
