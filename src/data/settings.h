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

class Settings : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int headerHeight MEMBER headerHeight NOTIFY headerHeightChanged)
  Q_PROPERTY(int headerShadowHeight MEMBER headerShadowHeight NOTIFY headerShadowHeightChanged)
  Q_PROPERTY(bool infoBtnPressed MEMBER infoBtnPressed NOTIFY infoBtnPressedChanged)
  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
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

signals:
  void headerShadowHeightChanged();
  void headerHeightChanged();
  void infoBtnPressedChanged();
  void titleChanged();
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

public:
  Q_INVOKABLE void setColorScheme(QColor primary, QColor secondary);

  // Header and Footer height
  int headerHeight = 80;
  int headerShadowHeight = 20;

  // Global Tooltips
  bool infoBtnPressed = false;

  // Page Title
  QString title = "Title";

  // Tileset and related engine for naming previews
  QString previewTileset = "";
  bool previewOutdoor = false;

  // Color Palette
  QColor textColorLight = QColor("#fafafa");
  QColor textColorMid = QColor("#757575");
  QColor textColorDark = QColor("#212121");

  QColor primaryColor = QColor("#d81b60");
  QColor primaryColorLight = QColor("#ff5c8d");
  QColor primaryColorDark = QColor("#a00037");

  QColor dividerColor = QColor("#BDBDBD");
  QColor accentColor = QColor("#607D8B");
};

#endif // SETTINGS_H
