/*
  * Copyright 2020 Fairy Fox
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
  /// Has the user been told that letting the map's people walk is DESTRUCTIVE? Set only when they
  /// tick "don't show me this again" -- which starts UNTICKED, because a warning you have to opt
  /// back into is not a warning. @see MapSim
  Q_PROPERTY(bool mapSimWarned MEMBER mapSimWarned NOTIFY mapSimWarnedChanged)
  Q_PROPERTY(QString previewTileset MEMBER previewTileset NOTIFY previewTilesetChanged)   ///< Tileset used for name previews.

  // The tileset's ANIMATION setting -- three states, not two. See the member below.
  Q_PROPERTY(int previewTilesetType MEMBER previewTilesetType NOTIFY previewTilesetTypeChanged)          ///< 0 Indoor / 1 Cave / 2 Outdoor.
  Q_PROPERTY(QString previewTilesetTypeStr READ previewTilesetTypeStr NOTIFY previewTilesetTypeChanged STORED false)   ///< "indoor"/"cave"/"outdoor" -- what the image providers want.
  Q_PROPERTY(QString previewTilesetTypeName READ previewTilesetTypeName NOTIFY previewTilesetTypeChanged STORED false) ///< "Indoor"/"Cave"/"Outdoor" -- what the user reads.
  Q_PROPERTY(QString previewTilesetTypeDoes READ previewTilesetTypeDoes NOTIFY previewTilesetTypeChanged STORED false) ///< What it actually DOES, in words.

  Q_PROPERTY(QColor textColorLight MEMBER textColorLight NOTIFY textColorLightChanged)    ///< Light text colour.
  Q_PROPERTY(QColor textColorMid MEMBER textColorMid NOTIFY textColorMidChanged)          ///< Mid text colour.
  Q_PROPERTY(QColor textColorDark MEMBER textColorDark NOTIFY textColorDarkChanged)       ///< Dark text colour.
  Q_PROPERTY(QColor primaryColor MEMBER primaryColor NOTIFY primaryColorChanged)          ///< Primary accent colour.
  Q_PROPERTY(QColor primaryColorLight MEMBER primaryColorLight NOTIFY primaryColorLightChanged) ///< Lighter primary.
  Q_PROPERTY(QColor primaryColorDark MEMBER primaryColorDark NOTIFY primaryColorDarkChanged)    ///< Darker primary.
  Q_PROPERTY(QColor errorColor MEMBER errorColor NOTIFY errorColorChanged)                 ///< Error/invalid colour (red); theme-independent.
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
  void mapSimWarnedChanged();
  void previewTilesetChanged();
  void previewTilesetTypeChanged();

  void textColorLightChanged();
  void textColorMidChanged();
  void textColorDarkChanged();
  void primaryColorChanged();
  void primaryColorLightChanged();
  void primaryColorDarkChanged();
  void errorColorChanged();
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

  /// Step Indoor -> Cave -> Outdoor -> Indoor. What the tri-state button does.
  Q_INVOKABLE void cyclePreviewTilesetType();

  // Header and Footer height
  int headerHeight = 80;        ///< @see headerHeight property.
  int headerShadowHeight = 20;  ///< @see headerShadowHeight property.

  // Global Tooltips
  bool infoBtnPressed = false;
  bool mapSimWarned = false;   ///< @see mapSimWarned property.

  // Tileset and related engine for naming previews
  QString previewTileset = "Overworld"; ///< @see previewTileset property.
  int getPreviewTilesetIndex();         ///< Index of @ref previewTileset (backs the property).

  /**
   * @brief Which tiles animate: 0 Indoor, 1 Cave, 2 Outdoor.
   *
   * THREE states, not two. This mirrors the game's own byte (`hTileAnimations`, saved as
   * `sTileAnimations` -- what AreaTileset calls `type`), whose values are TILEANIM_NONE,
   * TILEANIM_WATER and TILEANIM_WATER_FLOWER. `tileset.json`'s Indoor/Cave/Outdoor is a
   * verified 1:1 rename of exactly that, so the friendly name and the cartridge agree.
   *
   * It used to be a **bool** (`previewOutdoor`), which collapsed Cave into Indoor and so
   * rendered every cave with *dead water* -- when the console animates it. Three states is
   * the fix, not a relabel. See notes/reference/tiles.md.
   *
   * Defaults to Outdoor because the default preview tileset is Overworld, which is Outdoor.
   */
  int previewTilesetType = 2;

  QString previewTilesetTypeStr() const;   ///< "indoor"/"cave"/"outdoor" (for the image provider ids).
  QString previewTilesetTypeName() const;  ///< "Indoor"/"Cave"/"Outdoor" (for the user).
  QString previewTilesetTypeDoes() const;  ///< What it does, in words (for the user).

  // Color Palette
  QColor textColorLight = QColor("#efefef"); //#fafafa
  QColor textColorMid = QColor("#757575");
  QColor textColorDark = QColor("#212121");

  QColor primaryColor = QColor("#d81b60");
  QColor primaryColorLight = QColor("#ff5c8d");
  QColor primaryColorDark = QColor("#a00037");

  // Error/invalid red. Deliberately a fixed, theme-independent red (NOT derived
  // from primaryColor -- that's pink). setColorScheme() leaves it untouched so a
  // recoloured palette never turns "error" into the accent. Screens should use
  // brg.settings.errorColor for plain red instead of a literal "red".
  QColor errorColor = QColor("red");

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
