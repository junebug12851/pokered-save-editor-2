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

#include <QImage>
#include <QtMath>
#include <QPainter>

#include "./fontpreviewprovider.h"
#include "./tilesetengine.h"
#include "../data/db/fonts.h"

#include "../data/file/expanded/savefileexpanded.h"
#include "../data/file/expanded/player/player.h"
#include "../data/file/expanded/player/playerbasics.h"
#include "../data/file/expanded/rival.h"

FontPreviewInstance::FontPreviewInstance(
    QStringList idParts,
    SaveFileExpanded* expanded,
    QSize* size,
    const QSize& requestedSize)

  : size(size),
    requestedSize(requestedSize),
    expanded(expanded)
{
  // Initial setup and unfolding of idParts
  setup(idParts);

  // Error check maxInputStrLen and fix if needed
  errorCheckSetMax();

  // Now retrieve the tiles from the tileset engine
  getTiles();

  // Trim user supplied string to max
  enforceMaxSize();

  // Now retrieve tile codes for the whole thing and the lines they need to be
  // on
  getResultingText();

  // Figure out dimensions
  getImageWidth();
  getImageHeight();

  // Create base image and copy over to boxImg and fgImage
  getBaseImg();
  boxImg = baseImg;
  fgImg = baseImg;
  resultingImg = QPixmap::fromImage(baseImg);

  // If using box, then draw it
  if(box)
    drawBox();

  // Draw the Foreground now
  drawFg();

  // Finish the image by merging fg and box if applicable then tinting the
  // image
  finishImg();

  // Post process the image, this sort of finalizes the provider request by
  // setting requested size and scaling to requested size which could be the
  // providers requested size or my requested size.
  postProcess();
}

void FontPreviewInstance::setup(QStringList idParts)
{
  // Breakdown idParts into variables
  tileset = idParts.at(IdPartTileset);
  type = idParts.at(IdPartType);
  frame = idParts.at(IdPartFrame).toInt();

  toWidth = idParts.at(IdPartWidth).toInt();
  toHeight = idParts.at(IdPartHeight).toInt();

  box = idParts.at(IdPartType) == "box";
  lines2 = idParts.at(IdPartType) == "2-lines";
  maxInputStrLen = idParts.at(IdPartMax).toInt();
  bgColor = QColor(idParts.at(IdPartBGColor));

  useFg = idParts.at(IdPartFGColor) == "none";
  fgColor = (useFg)
      ? QColor(idParts.at(IdPartFGColor))
      : QColor();

  placeholder = idParts.at(IdPartPlaceHolder);
  getInputStr(); // Somewhat more complicated
}

void FontPreviewInstance::getInputStr()
{
  // Get str
  // If the player inserts slashes then we want the rest of it
  QString str = "";

  // Insert all the rest of the array and add slashes if not last entry
  for(int i = IdPartStr; i < idParts.size(); i++) {
    str += idParts[i];

    if(i < (idParts.size() - 1))
      str += "/";
  }

  inputStr = str;
}

void FontPreviewInstance::getTiles()
{
  // Pass though 3 of 4 arguments, we also make sure to tell to include font
  // tiles obviosuly, that's mandatory
  // <tileset>/<type>/<font>/<frame>
  tiles = TilesetEngine::buildTileset(
        tileset + "/" + type + "/font/" + frame);
}

void FontPreviewInstance::getResultingText()
{
  // Form the final text by inserting user string into placeholder string and
  // converting to code and back in a way that simulates how the game would
  // display it. Then split the string lines into seperate lines. Re-encode
  // those lines into tile numbers.
  QString resToConvert = placeholder.replace("%%", inputStr);
  auto res = FontsDB::expandStr(resToConvert, 255, rivalsName, playersName)
      .split("\n", QString::SkipEmptyParts);

  QVector<QVector<var8>> ret;
  for(auto entry : res) {
    QVector<var8> tmp = FontsDB::convertToCode(entry, 255, false);
    ret.append(tmp);
  }

  resultingText = ret;
}

void FontPreviewInstance::enforceMaxSize()
{
  // Converts to code, trimming after max chars, then converts back
  auto strCode = FontsDB::convertToCode(inputStr, maxInputStrLen, true);
  auto strBack = FontsDB::convertFromCode(strCode, 255);
  inputStr = strBack;
}

void FontPreviewInstance::errorCheckSetMax()
{
  if(maxInputStrLen <= 0 || maxInputStrLen > maxStrLenTiles)
      maxInputStrLen = maxStrLenTiles;
}

// Calculate longest line size to start with
// We have to use codes to figure that out, not string characters
void FontPreviewInstance::getImageWidth()
{
  imgWidth = 0;

  // Check to see if box is enabled, this simplifies things
  if(box)
    imgWidth = boxWidth;

  // Otherwise figure out longest line
  else {
    for(auto entry : resultingText) {
      if(entry.size() > imgWidth)
        imgWidth = entry.size() * tileSize;
    }
  }

  // Error check
  if(imgWidth > maxWidth)
    imgWidth = maxWidth;

  imgWidthTiles = imgWidth / tileSize;
}

void FontPreviewInstance::getImageHeight()
{
  // Large enough for 2 lines only if 2 lines are present and 2 lines are
  // allowed or if box is enabled, use that
  if(box)
    imgHeight = boxHeight;
  else {
    imgHeight = ((resultingText.size() > 1) && lines2)
        ? line2Height
        : line1Height;
  }

  imgHeightTiles = imgHeight / tileSize;
}

void FontPreviewInstance::getBaseImg()
{
  baseImg = QImage(imgWidth, imgHeight, QImage::Format_ARGB32);
  baseImg.fill((bgColor));
}

void FontPreviewInstance::drawBox()
{
  // Error check
  if(boxImg.width() < boxWidth ||
     boxImg.height() < boxHeight)
    return;

  QPainter p(&boxImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  var8 counter = 0;

  for(var8 y = 0; y < boxHeightTiles; y++) {
    for(var8 x = 0; x < boxWidthTiles; x++) {
      var8 boxTile = boxTiles[counter];
      p.drawPixmap(x * tileSize, y * tileSize, tiles.at(boxTile));
      counter++;
    }
  }
}

void FontPreviewInstance::drawFg()
{
  var8 startX = (box)
      ? 1 * tileSize
      : 0;

  var8 startY = (box)
      ? 1 * tileSize
      : 0;

  var8 yCounter = startY;
  var8 xCounter = startX;

  QPainter p(&fgImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  for(auto yLine : resultingText) {
    for(auto xTile : yLine) {

      // Don't draw in invalid areas
      if(((xCounter * tileSize) + tileSize) > fgImg.width())
        continue;
      if(((yCounter * tileSize) + tileSize) > fgImg.height())
        continue;

      p.drawPixmap(xCounter * tileSize, yCounter * tileSize, tiles.at(xTile));
      xCounter++;
    }

    xCounter = 0;
    yCounter++;
  }
}

void FontPreviewInstance::finishImg()
{
  QPainter p;

  QPixmap mask = resultingImg;

  p.begin(&resultingImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.drawImage(0, 0, boxImg);
  p.drawImage(0, 0, fgImg);
  p.end();

  if(useFg) {
    // Thanks Chris Kawa
    // https://forum.qt.io/topic/61684/tinting-a-qpixmap-using-qpainter-compositionmode_overlay/4

    p.begin(&mask);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(0, 0, resultingImg.width(), resultingImg.height(), fgColor);
    p.end();

    p.begin(&resultingImg);
    p.setCompositionMode(QPainter::CompositionMode_Overlay);
    p.drawPixmap(0, 0, mask);
    p.end();
  }
}

void FontPreviewInstance::postProcess()
{
  // Requested size
  QSize actualSize = QSize(toWidth, toHeight);

  // Set requested size if asked
  if(size != nullptr)
    *size = actualSize;

  // Scale to requested size
  // Either Qt Quick requested size or my requested size, either way the image
  // will be scaled to someones requested size
  resultingImg = resultingImg.scaled((requestedSize.width() > 0) ? requestedSize.width() : actualSize.width(),
                                     (requestedSize.height() > 0) ? requestedSize.height() : actualSize.height());
}

void FontPreviewInstance::getPlayersName()
{
  playersName = (expanded == nullptr || expanded->player->basics->playerName == "")
        ? "RED"
        : expanded->player->basics->playerName;
}

void FontPreviewInstance::getRivalsName()
{
  rivalsName = (expanded == nullptr || expanded->rival->name == "")
          ? "BLUE"
          : expanded->rival->name;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

FontPreviewProvider::FontPreviewProvider(SaveFileExpanded* expanded)
  : QQuickImageProvider(QQuickImageProvider::Pixmap),
    expanded(expanded)
{}

QPixmap FontPreviewProvider::requestPixmap(
    const QString& id, QSize* size, const QSize& requestedSize)
{
  // Pull from the cache if it exists in the cache & the players name and rivals
  // name match. If they don't the image needs to be regenerated.
  // It takes a seriously long time to generate the image so if it can be
  // pulled from the cache then go for it
  if(cache.contains(id)) {
    auto tmp = cache.object(id);
    if(tmp->playersName == expanded->player->basics->playerName &&
       tmp->rivalsName == expanded->rival->name)
      return cache.object(id)->resultingImg;
  }

  // Check to make sure it's a properly formed request
  auto idParts = id.split("/", QString::SplitBehavior::SkipEmptyParts);

  // Has to have all 10 parts unconditionally
  if(idParts.size() < FontPreviewInstance::IdPart_END)
    return getErrorImg(size, requestedSize);

  // With that out of the way, create an instance of the struct that will do
  // the work.
  auto inst = new FontPreviewInstance(idParts, expanded, size, requestedSize);

  // Cache it
  auto toCache = new FontPreviewCached();
  toCache->playersName = expanded->player->basics->playerName;
  toCache->rivalsName = expanded->rival->name;
  toCache->resultingImg = inst->resultingImg;
  cache.insert(id, toCache, 1);

  return inst->resultingImg;
}

QPixmap FontPreviewProvider::getErrorImg(QSize* size, const QSize& requestedSize)
{
  // Create an error "red" blank tile to indicate issue
  QSize actualSize = QSize(FontPreviewInstance::boxWidth,
                           FontPreviewInstance::boxHeight);

  if(size != nullptr)
    *size = actualSize;

  auto img = QImage(actualSize.width(),
                    actualSize.height(),
                    QImage::Format::Format_ARGB32);

  img.fill(QColor(255, 0, 0, 255)); // Fill with error "red" color

  auto ret = QPixmap::fromImage(img);

  ret = ret.scaled(
        (requestedSize.width() > 0) ? requestedSize.width() : actualSize.width(),
        (requestedSize.height() > 0) ? requestedSize.height() : actualSize.height());

  return ret;
}

// Allow 50 cached copies to exist
QCache<QString, FontPreviewCached> FontPreviewProvider::cache =
    QCache<QString, FontPreviewCached>(50);
