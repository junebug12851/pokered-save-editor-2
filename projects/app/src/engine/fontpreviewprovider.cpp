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
#include <QBitmap>
#include <QDebug>

#include "./fontpreviewprovider.h"
#include "./tilesetengine.h"
#include <pse-db/fonts.h>
#include <pse-common/utility.h>

#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerbasics.h>
#include <pse-savefile/expanded/rival.h>

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

  // Get Names from save file if present
  getPlayersName();
  getRivalsName();

  // Now retrieve the tiles from the tileset engine
  getTiles();

  // Now retrieve tile codes for the whole thing and the lines they need to be
  // on
  getResultingText();

  // Figure out dimensions
  getImageWidth();
  getImageHeight();

  // Create base image and copy over to bgImg, boxImg, and fgImage
  // make bgImage the bgColor
  getBaseImg();
  bgImg = baseImg;
  bgImg.fill(bgColor);

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
  this->idParts = idParts;

  tileset = idParts.at(IdPartTileset);
  type = idParts.at(IdPartType);
  frame = idParts.at(IdPartFrame).toInt();

  toWidth = idParts.at(IdPartWidth).toInt();
  toHeight = idParts.at(IdPartHeight).toInt();

  box = idParts.at(IdPartBox) == "box";
  lines2 = idParts.at(IdPart2Line) == "2-lines";
  chopLen = idParts.at(IdPartChop).toInt();
  bgColor = QColor(idParts.at(IdPartBGColor));

  useFg = idParts.at(IdPartFGColor) != "none";
  fgColor = (useFg)
      ? QColor(idParts.at(IdPartFGColor))
      : QColor();

  placeholder = Utility::decodeAfterUrl(idParts.at(IdPartPlaceHolder));

  getInputStr(); // Somewhat more complicated
}

void FontPreviewInstance::getInputStr()
{
  // Get str
  // If the player inserts slashes then we want the rest of it
  inputStr = Utility::decodeAfterUrl(idParts.mid(IdPartStr).join("/"));
}

void FontPreviewInstance::getTiles()
{
  // Pass though 3 of 4 arguments, we also make sure to tell to include font
  // tiles obviosuly, that's mandatory
  // <tileset>/<type>/<font>/<frame>
  QString tileUrl = tileset + "/" + type + "/font/" + QString::number(frame);
  tiles = TilesetEngine::buildTileset(tileUrl);
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

void FontPreviewInstance::getImageWidth()
{
  if(!box && chopLen > 0)
    imgWidth = chopLen * tileSize;
  else
    imgWidth = drawWidth;

  if(imgWidth > drawWidth)
    imgWidth = drawWidth;

  imgWidthTiles = imgWidth / tileSize;
}

void FontPreviewInstance::getImageHeight()
{
  // Large enough for 2 lines only if 2 lines are present and 2 lines are
  // allowed or if box is enabled, use that
  if(box)
    imgHeight = drawHeightBox;
  else {
    imgHeight = ((resultingText.size() > 1) && lines2)
        ? drawHeightLines2
        : drawHeightLines1;
  }

  imgHeightTiles = imgHeight / tileSize;
}

void FontPreviewInstance::getBaseImg()
{
  baseImg = QImage(imgWidth, imgHeight, QImage::Format_ARGB32);
  baseImg.fill(QColor("transparent"));
}

void FontPreviewInstance::drawBox()
{
  QPainter p(&boxImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  int counter = 0;

  for(int y = 0; y < drawHeightBoxTiles; y++) {
    for(int x = 0; x < drawWidthTiles; x++) {
      int boxTile = boxTiles[counter];
      p.drawPixmap(x * tileSize, y * tileSize, tiles.at(boxTile));
      counter++;
    }
  }
}

void FontPreviewInstance::drawFg()
{
  int startX = (box)
      ? 1
      : 0;

  int startY = (box)
      ? 2
      : 0;

  int yCounter = startY;
  int xCounter = startX;

  QPainter p(&fgImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.setBrush(bgColor);
  p.setPen(QColor("transparent"));

  for(auto yLine : resultingText) {
    for(auto xTile : yLine) {

      if(box)
        p.drawRect(xCounter * tileSize, yCounter * tileSize, tileSize, tileSize);

      p.drawPixmap(xCounter * tileSize, yCounter * tileSize, tiles.at(xTile));
      xCounter++;
    }

    xCounter = startX;
    yCounter += 2; // Keep space between lines
  }
}

void FontPreviewInstance::finishImg()
{
  QPainter p;

  // Foreground is disabled because I could never get it to work correctly
  // The issue is I have 4-color images and it seems to want to colorize
  // non-B&W or B&W only but not both so I give up unless someone else can
  // help me.

  p.begin(&resultingImg);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.drawImage(0, 0, bgImg);
  p.drawImage(0, 0, boxImg);
  p.drawImage(0, 0, fgImg);
  p.end();
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

FontPreviewProvider::FontPreviewProvider(SaveFileExpanded* expanded)
  : QQuickImageProvider(QQuickImageProvider::Pixmap),
    expanded(expanded)
{}

QPixmap FontPreviewProvider::requestPixmap(
    const QString& id, QSize* size, const QSize& requestedSize)
{
  // Check to make sure it's a properly formed request
  auto idParts = id.split("/", QString::SplitBehavior::KeepEmptyParts);

  // Has to have all 10 parts unconditionally
  if(idParts.size() < FontPreviewInstance::IdPart_END)
    return getErrorImg(size, requestedSize);

  // With that out of the way, create an instance of the struct that will do
  // the work.
  auto inst = FontPreviewInstance(idParts, expanded, size, requestedSize);

  return inst.resultingImg;
}

QPixmap FontPreviewProvider::getErrorImg(QSize* size, const QSize& requestedSize)
{
  // Create an error "red" blank tile to indicate issue
  QSize actualSize = QSize(FontPreviewInstance::drawWidth,
                           FontPreviewInstance::drawHeightBox);

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
