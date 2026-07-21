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
#include <QVector>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>
#include <QSize>
#include <QStringList>
#include <QColor>
#include <QCache>

#include <pse-common/types.h>

class SaveFileExpanded;

/**
 * @brief The render pipeline for a single font-preview request.
 *
 * Built per request from the parsed id-string parts (see @ref IdPart* and
 * FontPreviewProvider's id format). It renders in-game text -- optionally inside a
 * dialogue box, on 1 or 2 lines, in chosen colours -- to a QPixmap, step by step
 * (getInputStr -> getTiles -> drawBox -> drawFg -> finishImg -> postProcess). The
 * `static constexpr` layout values capture the Game Boy box geometry.
 *
 * @see FontPreviewProvider (the QML image provider that drives this), FontsDB
 *      (the codec that turns text into tile codes).
 */
struct FontPreviewInstance
{
  /// @param idParts the slash-split id; @param expanded the live save (for names);
  /// @param size out-param size; @param requestedSize QML's requested size.
  FontPreviewInstance(QStringList idParts,
                      SaveFileExpanded* expanded,
                      QSize* size,
                      const QSize& requestedSize);

  /// Index of each field within the parsed id-string.
  enum : int {
    IdPartTileset = 0,
    IdPartType,
    IdPartFrame,
    IdPartWidth,
    IdPartHeight,
    IdPartBox,
    IdPart2Line,
    IdPartChop,
    IdPartBGColor,
    IdPartFGColor,
    IdPartPlaceHolder,
    IdPartStr,
    IdPart_END
  };

  // Tiles to draw a box with arrow
  // A box is 6 tiles tall and 20 tiles wide. It's contents are double spaced
  // so it can only hold 2 lines of text but gives it nice line-height and
  // it has no left margin but a right margin of 1 tile. This is because the
  // text sits right in the box but keeps it from overlapping the right bottom
  // arrow.
  static constexpr int boxTiles[] = {
    0x79,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7B,
    0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7C,
    0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7C,
    0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7C,
    0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0xEE,0x7C,
    0x7D,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7E
  }; ///< Tilemap for a 20x6 dialogue box with arrow (see note above).

  // Tile size
  static constexpr int tileSize = 8; ///< Pixels per tile edge.

  // Hard-coded in FontDB and in FrontPreviewProvider
  static constexpr int maxLines = 2; ///< Max text lines (must match FontsDB).

  // Spacing between mutliple lines
  static constexpr int lineHeightTiles = 1;
  static constexpr int lineHeight = lineHeightTiles * tileSize;

  // Width and Heights
  static constexpr int drawWidthTiles = 20;
  static constexpr int drawHeightLines1Tiles = 1;
  static constexpr int drawHeightLines2Tiles = 2 + lineHeightTiles;
  static constexpr int drawHeightBoxTiles = 6;

  static constexpr int drawWidth = drawWidthTiles * tileSize;
  static constexpr int drawHeightLines1 = drawHeightLines1Tiles * tileSize;
  static constexpr int drawHeightLines2 = drawHeightLines2Tiles * tileSize;
  static constexpr int drawHeightBox = drawHeightBoxTiles * tileSize;

  // Max str length
  static constexpr int maxStrLenTiles = drawWidthTiles * maxLines;

  void setup(QStringList idParts); ///< Parse the id parts into the fields below.

  void getInputStr();      ///< Resolve the final input string (placeholder + str + names).
  void getTiles();         ///< Render the per-tile pixmaps for the text.
  void getResultingText(); ///< Convert the string to tile-code rows via FontsDB.
  void getImageWidth();    ///< Compute output width.
  void getImageHeight();   ///< Compute output height.
  void getBaseImg();       ///< Allocate the base image.
  void drawBox();          ///< Draw the dialogue box background, if requested.
  void drawFg();           ///< Draw the foreground text.
  void finishImg();        ///< Composite into the final pixmap.
  void postProcess();      ///< Apply any post-processing.

  void getPlayersName(); ///< Read the player's name from the save (for substitution).
  void getRivalsName();  ///< Read the rival's name from the save (for substitution).

  // From ID String
  QString tileset;   ///< Tileset name (id part).
  QString type;      ///< Type, e.g. "outdoor" (id part).
  int frame = 0;     ///< Animation frame (id part).
  int toWidth = 0;   ///< Target width (id part).
  int toHeight = 0;  ///< Target height (id part).
  bool box = false;  ///< Render a dialogue box (id part).
  bool lines2 = false; ///< Render on two lines (id part).
  int chopLen = 0;   ///< Max line length before chopping (id part).
  QColor bgColor;    ///< Background colour (id part).
  QColor fgColor;    ///< Foreground/text colour (id part; "none" = don't paint).
  QString placeholder; ///< Template the str is inserted into (id part).
  QString inputStr;  ///< The raw font string to render (id part).

  // Other vars
  QStringList idParts;                  ///< The raw split id parts.
  QVector<QVector<var8>> resultingText; ///< Text as tile-code rows.
  QImage bgImg;        ///< Background layer.
  QImage baseImg;      ///< Base canvas.
  QImage boxImg;       ///< Box layer.
  QImage fgImg;        ///< Foreground layer.
  QPixmap resultingImg; ///< The finished pixmap.
  QVector<QPixmap> tiles; ///< Per-character tile pixmaps.
  QString playersName; ///< Resolved player name.
  QString rivalsName;  ///< Resolved rival name.
  int imgWidth = 0;       ///< Output width (px).
  int imgWidthTiles = 0;  ///< Output width (tiles).
  int imgHeight = 0;      ///< Output height (px).
  int imgHeightTiles = 0; ///< Output height (tiles).

  bool useFg = false; ///< Whether the foreground is painted.

  // Provider Vars
  QSize* size = nullptr;          ///< Out-param size handed back to QML.
  const QSize& requestedSize;     ///< Size QML requested.

  // Reference vars
  SaveFileExpanded* expanded = nullptr; ///< Live save (for name substitution).
};

/// A cached font-preview result, keyed alongside the names it depended on.
struct FontPreviewCached
{
  QString playersName;  ///< Player name at render time (cache validity).
  QString rivalsName;   ///< Rival name at render time (cache validity).
  QPixmap resultingImg; ///< The cached pixmap.
};

/**
 * @brief QML image provider that renders in-game text previews ("image://...").
 *
 * QML requests `image://<provider>/<id>` and this returns the rendered pixmap. The
 * id format (slash-separated) is documented in-code below. Backs the live name
 * previews in the editors/keyboard. Holds the live save for name substitution.
 *
 * @see FontPreviewInstance (the per-request renderer), Settings::previewTileset.
 */
class FontPreviewProvider : public QQuickImageProvider
{
public:
  FontPreviewProvider(SaveFileExpanded* expanded); ///< @param expanded the live save.

  // <tileset>/<type>/<frame>/<width>/<height>/<box>/<2-lines>/<max>/<bgColor>/<fgColor>/<placeholder>/<str>
  //  * <tileset> is the tileset, case-insensitive and spaces converted to
  //    underscores
  //  * <type> is the type, specifically "outdoor" or not is used here
  //  * <frame> can be any positive number, a full frame cycle completes in 8
  //    frames though so it's suggested to use multiple of 8 for smooth
  //    animation
  //  * <box> tells whether to render a textbox or not as a bg
  //    specifically "box" or not
  //  * <2-lines> tells whether to render on 2 lines or not
  //    specifically "2-lines" or not
  //  * <max> is maximum line length (Excluding placeholder), if not between
  //    1 and 21 then it will be set to 21
  //  * <bgColor> tells what background color it should be in any QColor format
  //  * <fgColor> tells what text color it should be in any QColor format
  //    "none" however is special and means don't paint
  //  * <str> is the font str to render
  //  * <placeholder> is text to put str into, use %% in placeholder to insert
  //    the str
  /// Render the preview for @p id (format documented above). @return the pixmap.
  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
  QPixmap getErrorImg(QSize* size, const QSize& requestedSize); ///< Fallback image when an id is invalid.

  SaveFileExpanded* expanded = nullptr; ///< Live save (for name substitution).
};
