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
#ifndef FONTPREVIEWPROVIDER_H
#define FONTPREVIEWPROVIDER_H

#include <QVector>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>
#include <QSize>
#include <QStringList>
#include <QColor>
#include <QCache>

#include "../common/types.h"

class SaveFileExpanded;

struct FontPreviewInstance
{
  FontPreviewInstance(QStringList idParts,
                      SaveFileExpanded* expanded,
                      QSize* size,
                      const QSize& requestedSize);

  enum : var8 {
    IdPartTileset = 0,
    IdPartType,
    IdPartFrame,
    IdPartBox,
    IdPart2Line,
    IdPartMax,
    IdPartBGColor,
    IdPartFGColor,
    IdPartPlaceHolder,
    IdPartStr,
    IdPart_END
  };

  // Tiles to draw a box with arrow
  static constexpr var8 boxTiles[] = {
      0x79,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7B,
      0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7C,
      0x7C,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0xEE,0x7C,
      0x7D,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7A,0x7E
  };

  static constexpr var8 tileSize = 8;
  static constexpr var8 boxWidthTiles = 20;
  static constexpr var8 boxHeightTiles = 4;
  static constexpr var8 boxWidth = boxWidth * tileSize;
  static constexpr var8 boxHeight = boxHeight * tileSize;
  static constexpr var8 maxWidthTiles = 20;
  static constexpr var8 maxWidth = 20 * tileSize;
  static constexpr var8 maxBoxFGWidthTiles = 17;
  static constexpr var8 maxBoxFGHeightTiles = 2;
  static constexpr var8 maxBoxFGWidth = maxBoxFGWidthTiles * tileSize;
  static constexpr var8 maxBoxFGHeight = maxBoxFGHeightTiles * tileSize;
  static constexpr var8 maxStrLenTiles = maxWidthTiles * maxBoxFGHeightTiles;
  static constexpr var8 line1HeightTiles = 1;
  static constexpr var8 line2HeightTiles = 2;
  static constexpr var8 line1Height = line1HeightTiles * tileSize;
  static constexpr var8 line2Height = line2HeightTiles * tileSize;

  void setup(QStringList idParts);

  void getInputStr();
  void getTiles();
  void getResultingText();
  void enforceMaxSize();
  void errorCheckSetMax();
  void getImageWidth();
  void getImageHeight();
  void getBaseImg();
  void drawBox();
  void drawFg();
  void finishImg();

  void getPlayersName();
  void getRivalsName();

  // From ID String
  QString tileset;
  QString type;
  int frame = 0;
  bool box = false;
  bool lines2 = false;
  int maxInputStrLen = 0;
  QColor bgColor;
  QColor fgColor;
  QString placeholder;
  QString inputStr;

  // Other vars
  QStringList idParts;
  QVector<QVector<var8>> resultingText;
  QImage baseImg;
  QImage boxImg;
  QImage fgImg;
  QPixmap resultingImg;
  QVector<QPixmap> tiles;
  QString playersName;
  QString rivalsName;
  int imgWidth = 0;
  int imgWidthTiles = 0;
  int imgHeight = 0;
  int imgHeightTiles = 0;
  bool useFg = false;

  // Provider Vars
  QSize* size = nullptr;
  const QSize& requestedSize;

  // Reference vars
  SaveFileExpanded* expanded = nullptr;
};

struct FontPreviewCached
{
  QString playersName;
  QString rivalsName;
  QPixmap resultingImg;
};

class FontPreviewProvider : public QQuickImageProvider
{
public:
  FontPreviewProvider(SaveFileExpanded* expanded);

  // <tileset>/<type>/<frame>/<box>/<2-lines>/<max>/<bgColor>/<fgColor>/<placeholder>/<str>
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
  virtual QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;
  QPixmap getErrorImg(QSize* size, const QSize& requestedSize);

  static QCache<QString, FontPreviewCached> cache;
  SaveFileExpanded* expanded = nullptr;
};

#endif // FONTPREVIEWPROVIDER_H
