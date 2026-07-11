/*
  * Copyright 2026 Twilight
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

/**
 * @file fontkeyboard.cpp
 * @brief Implementation of FontKeyboard -- the tile->key map. See fontkeyboard.h
 *        for the doctrine behind the table below.
 */

#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>
#include "./fontkeyboard.h"

namespace {

// The 36 assignable keys, in the order they appear on the deck: the number row,
// then QWERTY, then the home row, then the bottom row. EVERY page table below is
// written in exactly this order, so a page reads like the keyboard looks.
const char* const kKeys[FontKeyboard::keyTotal] = {
  "1","2","3","4","5","6","7","8","9","0",
  "Q","W","E","R","T","Y","U","I","O","P",
  "A","S","D","F","G","H","J","K","L",
  "Z","X","C","V","B","N","M",
};

// ---------------------------------------------------------------------------
// THE MAP. Font codes (FontDBEntry::ind); 0 = an empty key.
//
// Read each block as the deck: row 1 is the number row, row 2 is QWERTYUIOP,
// row 3 is ASDFGHJKL, row 4 is ZXCVBNM.
//
// Every tile 1-255 appears exactly once across these 8 pages + kSpaceInd. Do not
// "tidy" a tile from one page to another without re-running tst_font_keyboard --
// it checks the invariant, and a duplicate or a hole would be a silent bug.
//
// NOTE the tables are named, not numbered, and are wired to pages by MODIFIER MASK
// (kPages, below): a page's index IS its modifier bits (shift=1, ctrl=2, alt=4), so
// Alt is page 4 -- not page 4-in-reading-order. Mixing those two up silently swaps
// two whole pages, which is exactly what tst_font_keyboard caught the first time.
// The human reading order lives in kPageOrder.
// ---------------------------------------------------------------------------

// (no modifier) -- LETTERS. Pure identity: the alphabet is where the alphabet is,
// and the digits are on the digits. Nothing to learn.
const int kPageLetters[FontKeyboard::keyTotal] = {
  247, 248, 249, 250, 251, 252, 253, 254, 255, 246,   // 1 2 3 4 5 6 7 8 9 0
  144, 150, 132, 145, 147, 152, 148, 136, 142, 143,   // Q W E R T Y U I O P
  128, 146, 131, 133, 134, 135, 137, 138, 139,        // A S D F G H J K L
  153, 151, 130, 149, 129, 141, 140,                  // Z X C V B N M
};

// Shift -- LOWERCASE + the Normal symbols. Letters are identity again
// (Shift+A really is "a"). The number row keeps the real keyboard's shifted
// symbols wherever the game HAS that glyph: Shift+1 = !, Shift+8 = x (multiply,
// i.e. "*"), Shift+9/0 = ( ). Pages 0+1 together are the entire in-game-legal
// character set -- everything you can type on the real Game Boy's name screen.
const int kPageLower[FontKeyboard::keyTotal] = {
  231, 230, 232, 244, 227, 239, 245, 241, 154, 155,   // ! ? . , - male female x ( )
  176, 182, 164, 177, 179, 184, 180, 168, 174, 175,   // q w e r t y u i o p
  160, 178, 163, 165, 166, 167, 169, 170, 171,        // a s d f g h j k l
  185, 183, 162, 181, 161, 173, 172,                  // z x c v b n m
};

// Ctrl -- SYMBOLS. The 13 bold letters sit on their own letters, so
// Ctrl+B is bold B -- the same thing it means in every other program on earth.
// The rest of the row is punctuation, with the leftover Normal chars (: ; [ ] /
// Pk Mn) folded in so the whole Normal category is done by the end of this page.
const int kPageSymbols[FontKeyboard::keyTotal] = {
  156, 157, 158, 159, 243,   0,   0,   0,   0,   0,   // : ; [ ] /
  114, 115, 100, 112, 113, 116, 117, 104, 240, 225,   // " " boldE ' ' mdot ... boldI $ Pk
   96, 106,  99, 101, 102, 103, 224, 186, 107,        // boldA boldS boldD boldF boldG boldH ' e-acute boldL
  109, 242,  98, 105,  97, 226, 108,                  // :(narrow) .(alt) boldC boldV boldB Mn boldM
};

// Alt -- CODES. Contractions sit on their own letter ('s on S, 't on
// T...). The variables pair up: Player on P with Rival ("Opponent") right beside
// it on O, and the two usage codes on Y ("Your usage") / U ("User"). The kana --
// the rarest things here -- take the number row.
const int kPageCodes[FontKeyboard::keyTotal] = {
  118, 110, 111, 119, 120, 233, 234, 235,   0,   0,   // a i u e o + small a u e
    0,   0,  84, 228, 190,  89,  90,   0,  83,  82,   // Poke 'r 't targ user rival player
    0, 189, 187,  86,  94,  93,  92,  74, 188,        // 's 'd ...... ROCKET TRAINER TM PkMn 'l
    0,   0,  91, 191,   0,   0, 229,                  // PC 'v 'm
};

// Shift+Ctrl -- TILES I. The nine Pictures anyone actually uses get the
// best keys on the page: the six box-frame glyphs are literally DRAWN as a box on
// the keys (Q=TL W=top E=TR / A=left / Z=BL X=BR) and the three cursor arrows take
// the right home keys (J K L). Raw tiles 01-1B fill what's left, in reading order.
const int kPageTiles1[FontKeyboard::keyTotal] = {
    1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   // tile01..tile0A
  121, 122, 123,  11,  12,  13,  14,  15,  16,  17,   // box-TL box-horz box-TR, tile0B..tile11
  124,  18,  19,  20,  21,  22, 237, 238, 236,        // box-vert, tile12..tile16, arrows
  125, 126,  23,  24,  25,  26,  27,                  // box-BL box-BR, tile17..tile1B
};

// Shift+Alt -- TILES II. tile1C..tile3F, straight reading order.
const int kPageTiles2[FontKeyboard::keyTotal] = {
   28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
   38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
   48,  49,  50,  51,  52,  53,  54,  55,  56,
   57,  58,  59,  60,  61,  62,  63,
};

// Ctrl+Alt -- TILES III. tile40..tile48 + tile4D, then tileC0..tileD9.
const int kPageTiles3[FontKeyboard::keyTotal] = {
   64,  65,  66,  67,  68,  69,  70,  71,  72,  77,   // tile40..tile48, tile4D
  192, 193, 194, 195, 196, 197, 198, 199, 200, 201,   // tileC0..tileC9
  202, 203, 204, 205, 206, 207, 208, 209, 210,        // tileCA..tileD2
  211, 212, 213, 214, 215, 216, 217,                  // tileD3..tileD9
};

// Shift+Ctrl+Alt -- CONTROLS. The codes that GLITCH a name live behind
// the most deliberate key combination on the deck -- you cannot hit <end> by
// accident. Mnemonic letters throughout (E=end, L=line, N=next, P=page, X=dex...).
// The last six raw tiles ride the number row.
const int kPageControls[FontKeyboard::keyTotal] = {
  218, 219, 220, 221, 222, 223,   0,   0,   0,   0,   // tileDA..tileDF
    0,   0,  80,  88,   0,   0,   0,   0,   0,  73,   // end prompt ... page
   81,   0,  87,   0,   0,   0,   0,   0,  79,        // para done line
    0,  95,  85,  75,  76,  78,   0,                  // dex cont cont_ autocont next
};

// A page's INDEX IS ITS MODIFIER MASK (shift = 1, ctrl = 2, alt = 4). That's the
// whole trick: pageFor() is just the mask, no lookup, and the deck can never show a
// page that disagrees with the keys you're holding.
const int* const kPages[FontKeyboard::pageTotal] = {
  kPageLetters,   // 0 = ---              (nothing held)
  kPageLower,     // 1 = Shift
  kPageSymbols,   // 2 = Ctrl
  kPageTiles1,    // 3 = Shift+Ctrl
  kPageCodes,     // 4 = Alt
  kPageTiles2,    // 5 = Shift+Alt
  kPageTiles3,    // 6 = Ctrl+Alt
  kPageControls,  // 7 = Shift+Ctrl+Alt
};

// The 255th tile. Space is the only tile NOT on an alphanumeric key -- it rides
// the deck's real spacebar, which is where every human already expects it.
constexpr int kSpaceInd = 127;

// Indexed by mask, like kPages.
const char* const kPageNames[FontKeyboard::pageTotal] = {
  "Letters", "Lowercase", "Symbols", "Tiles I",
  "Codes", "Tiles II", "Tiles III", "Controls",
};

const char* const kPageBadges[FontKeyboard::pageTotal] = {
  "", "Shift", "Ctrl", "Shift+Ctrl",
  "Alt", "Shift+Alt", "Ctrl+Alt", "Shift+Ctrl+Alt",
};

// The order a HUMAN should meet the pages -- by category (Normal, Single-Char,
// Multi-Char, Variable, Picture, Control), cheapest chord first. This is NOT the mask
// order: Alt (4) is a one-key chord and belongs before Shift+Ctrl (3), which is two.
// The page strip renders in this order; everything else indexes by mask.
const int kPageOrder[FontKeyboard::pageTotal] = { 0, 1, 2, 4, 3, 5, 6, 7 };

/// One colour per tile, so the same precedence the old picker used (and therefore
/// the same colours users already learned): normal, then control, then picture,
/// then single, then variable, then multi.
FontKeyboard::Category categoryOf(const FontDBEntry* f)
{
  if(f == nullptr)          return FontKeyboard::CatNone;
  if(f->getNormal())        return FontKeyboard::CatNormal;
  if(f->getControl())       return FontKeyboard::CatControl;
  if(f->getPicture())       return FontKeyboard::CatPicture;
  if(f->getSingleChar())    return FontKeyboard::CatSingle;
  if(f->getVariable())      return FontKeyboard::CatVariable;
  if(f->getMultiChar())     return FontKeyboard::CatMulti;

  return FontKeyboard::CatNone;
}

/// How the cap should draw this tile. A control code has no glyph at all, and a
/// multi-char/variable is not one tile, so neither can come off the tile sheet.
FontKeyboard::Render renderOf(const FontDBEntry* f)
{
  if(f == nullptr)                        return FontKeyboard::RenderEmpty;
  if(f->getControl())                     return FontKeyboard::RenderLabel;
  if(f->getVariable() || f->getMultiChar()) return FontKeyboard::RenderPreview;

  return FontKeyboard::RenderTile;
}

}  // namespace

FontKeyboard::FontKeyboard(QObject* parent)
  : QObject(parent)
{}

int FontKeyboard::getPage() const
{
  return page;
}

void FontKeyboard::setPage(int val)
{
  if(val < 0 || val >= pageTotal || val == page)
    return;

  page = val;
  emit pageChanged();
}

QStringList FontKeyboard::getPageNames() const
{
  QStringList ret;
  ret.reserve(pageTotal);

  for(int i = 0; i < pageTotal; i++)
    ret.append(QString(kPageNames[i]));

  return ret;
}

int FontKeyboard::getPageCount() const
{
  return pageTotal;
}

QVariantList FontKeyboard::getPageOrder() const
{
  QVariantList ret;
  ret.reserve(pageTotal);

  for(int i = 0; i < pageTotal; i++)
    ret.append(kPageOrder[i]);

  return ret;
}

QStringList FontKeyboard::keyLabels()
{
  QStringList ret;
  ret.reserve(keyTotal);

  for(int i = 0; i < keyTotal; i++)
    ret.append(QString(kKeys[i]));

  return ret;
}

int FontKeyboard::pageFor(bool shift, bool ctrl, bool alt)
{
  return (shift ? 1 : 0) | (ctrl ? 2 : 0) | (alt ? 4 : 0);
}

QString FontKeyboard::pageBadge(int page)
{
  if(page < 0 || page >= pageTotal)
    return QString();

  return QString(kPageBadges[page]);
}

int FontKeyboard::indFor(int page, const QString& key)
{
  if(page < 0 || page >= pageTotal)
    return 0;

  const QString wanted = key.toUpper();

  for(int i = 0; i < keyTotal; i++) {
    if(wanted != QString(kKeys[i]))
      continue;

    return kPages[page][i];
  }

  return 0;
}

QVariantMap FontKeyboard::keyData(int page, const QString& key) const
{
  return dataForInd(indFor(page, key), key.toUpper());
}

QVariantMap FontKeyboard::spaceData() const
{
  return dataForInd(kSpaceInd, QStringLiteral("Space"));
}

QVariantMap FontKeyboard::dataForInd(int ind, const QString& key) const
{
  QVariantMap ret;

  ret["key"] = key;
  ret["ind"] = ind;

  // An unmapped key is a normal, expected thing (34 of the 288 slots are empty) --
  // it must come back as a well-formed "empty" map, never a null the QML has to
  // guard against.
  const FontDBEntry* f = (ind > 0)
      ? FontsDB::inst()->getStoreByVal(ind)
      : nullptr;

  if(f == nullptr) {
    ret["empty"] = true;
    ret["code"] = QString();
    ret["title"] = QString();
    ret["tip"] = QString();
    ret["category"] = static_cast<int>(CatNone);
    ret["render"] = static_cast<int>(RenderEmpty);
    return ret;
  }

  const QString alias = f->getAlias();

  ret["empty"] = false;
  ret["code"] = f->getName();
  ret["title"] = (alias != "") ? alias : f->getName();
  ret["tip"] = f->getTip();
  ret["category"] = static_cast<int>(categoryOf(f));
  ret["render"] = static_cast<int>(renderOf(f));

  return ret;
}

QString FontKeyboard::chopLastToken(const QString& str)
{
  if(str.isEmpty())
    return str;

  // A trailing "<...>" is ONE tile, not five characters. Backspace deletes the
  // whole token or it corrupts the string into something that no longer round-trips
  // through the codec.
  if(str.endsWith('>')) {
    const int open = str.lastIndexOf('<');
    if(open >= 0)
      return str.left(open);
  }

  return str.left(str.size() - 1);
}
