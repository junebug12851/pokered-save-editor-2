/*
  * Copyright 2020 Twilight
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
#pragma once
#include <QObject>
#include <QString>
#include <QColor>

class SaveFile;

/**
 * @brief App-wide UI settings: layout metrics, the colour palette, and font colours.
 *
 * The single source of truth the QML reads for theming and layout -- header
 * sizing, the Material-style colour palette (text/primary/divider/accent), the
 * per-font-category colours used by the keyboard, and the name-preview tileset
 * choice. Exposed to QML as `brg.settings`. setColorScheme() recolours the palette
 * at runtime; it holds the SaveFile so a few settings can react to data changes.
 *
 * @see Bridge.
 */
class Settings : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int headerHeight MEMBER headerHeight NOTIFY headerHeightChanged)             ///< Header bar height.
  Q_PROPERTY(int headerShadowHeight MEMBER headerShadowHeight NOTIFY headerShadowHeightChanged) ///< Header drop-shadow height.

  Q_PROPERTY(bool infoBtnPressed MEMBER infoBtnPressed NOTIFY infoBtnPressedChanged)      ///< Global tooltip (info button) toggle.
  Q_PROPERTY(QString previewTileset MEMBER previewTileset NOTIFY previewTilesetChanged)   ///< Tileset used for name previews.
  Q_PROPERTY(bool previewOutdoor MEMBER previewOutdoor NOTIFY previewOutdoorChanged)      ///< Outdoor vs indoor preview.

  Q_PROPERTY(QColor textColorLight MEMBER textColorLight NOTIFY textColorLightChanged)    ///< Light text colour.
  Q_PROPERTY(QColor textColorMid MEMBER textColorMid NOTIFY textColorMidChanged)          ///< Mid text colour.
  Q_PROPERTY(QColor textColorDark MEMBER textColorDark NOTIFY textColorDarkChanged)       ///< Dark text colour.
  Q_PROPERTY(QColor primaryColor MEMBER primaryColor NOTIFY primaryColorChanged)          ///< Primary accent colour.
  Q_PROPERTY(QColor primaryColorLight MEMBER primaryColorLight NOTIFY primaryColorLightChanged) ///< Lighter primary.
  Q_PROPERTY(QColor primaryColorDark MEMBER primaryColorDark NOTIFY primaryColorDarkChanged)    ///< Darker primary.
  Q_PROPERTY(QColor dividerColor MEMBER dividerColor NOTIFY dividerColorChanged)           ///< Divider/line colour.
  Q_PROPERTY(QColor accentColor MEMBER accentColor NOTIFY accentColorChanged)              ///< Secondary accent colour.
  Q_PROPERTY(int previewTilesetIndex READ getPreviewTilesetIndex NOTIFY previewTilesetChanged STORED false) ///< Index of the preview tileset.

  Q_PROPERTY(QColor fontColorNormal MEMBER fontColorNormal NOTIFY fontColorNormalChanged)   ///< Keyboard colour: normal glyphs.
  Q_PROPERTY(QColor fontColorControl MEMBER fontColorControl NOTIFY fontColorControlChanged) ///< Keyboard colour: control glyphs.
  Q_PROPERTY(QColor fontColorPicture MEMBER fontColorPicture NOTIFY fontColorPictureChanged) ///< Keyboard colour: picture glyphs.
  Q_PROPERTY(QColor fontColorSingle MEMBER fontColorSingle NOTIFY fontColorSingleChanged)   ///< Keyboard colour: single-char glyphs.
  Q_PROPERTY(QColor fontColorMulti MEMBER fontColorMulti NOTIFY fontColorMultiChanged)      ///< Keyboard colour: multi-char glyphs.
  Q_PROPERTY(QColor fontColorVar MEMBER fontColorVar NOTIFY fontColorVarChanged)            ///< Keyboard colour: variable glyphs.

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
  Settings(SaveFile* file); ///< @param file the live save (a few settings react to it).

  Q_INVOKABLE void setColorScheme(QColor primary, QColor secondary); ///< Recolour the palette at runtime.

  // Header and Footer height
  int headerHeight = 80;        ///< @see headerHeight property.
  int headerShadowHeight = 20;  ///< @see headerShadowHeight property.

  // Global Tooltips
  bool infoBtnPressed = false;  ///< @see infoBtnPressed property.

  // Tileset and related engine for naming previews
  QString previewTileset = "Overworld"; ///< @see previewTileset property.
  bool previewOutdoor = true;           ///< @see previewOutdoor property.
  int getPreviewTilesetIndex();         ///< Index of @ref previewTileset (backs the property).

  // Color Palette
  QColor textColorLight = QColor("#efefef"); //#fafafa
  QColor textColorMid = QColor("#757575");
  QColor textColorDark = QColor("#212121");

  QColor primaryColor = QColor("#d81b60");
  QColor primaryColorLight = QColor("#ff5c8d");
  QColor primaryColorDark = QColor("#a00037");

  QColor dividerColor = QColor("#BDBDBD");
  QColor accentColor = QColor("#607D8B");

  QColor fontColorNormal = QColor("#616161"); // Grey, Shade 700
  QColor fontColorControl = QColor("#7B1FA2"); // Purple, Shade 700
  QColor fontColorPicture = QColor("#303F9F"); // Blue, Shade 700
  QColor fontColorSingle = QColor("#9E9D24"); // Lime, Shade 800
  QColor fontColorMulti = QColor("#FF6F00"); // Amber, Shade 900
  QColor fontColorVar = QColor("#388E3C"); // Green, Shade 700

protected slots:
  void dataChanged(); ///< React to the save's data changing.

protected:
  SaveFile* file; ///< The live save (held for data-reactive settings).
};
