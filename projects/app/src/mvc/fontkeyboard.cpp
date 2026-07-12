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

// The 47 assignable keys, in the order they appear on the deck: the number row, then
// QWERTY, then the home row, then the bottom row. EVERY page table below is written in
// exactly this order, so a page reads like the keyboard looks.
//
// The punctuation keys (` - = [ ] \ ; ' , . /) are in here because the tiles that
// BELONG on them should be on them: `.` types `.`, `,` types `,`, Shift+`/` types `?`,
// Shift+`;` types `:`. They were dead filler in the first cut, which meant `.` and `,`
// were exiled to a number row they have no business being on.
const char* const kKeys[FontKeyboard::keyTotal] = {
  "`","1","2","3","4","5","6","7","8","9","0","-","=",
  "Q","W","E","R","T","Y","U","I","O","P","[","]","\\",
  "A","S","D","F","G","H","J","K","L",";","'",
  "Z","X","C","V","B","N","M",",",".","/",
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

// Each block is written as the deck reads, one line per row:
//   row 1:  `  1 2 3 4 5 6 7 8 9 0  -  =
//   row 2:  Q W E R T Y U I O P  [  ]  '\'
//   row 3:  A S D F G H J K L  ;  '
//   row 4:  Z X C V B N M  ,  .  /

// (no modifier) -- LETTERS. The base layer of a real keyboard: press a key, get the
// LOWERCASE letter; press a digit, get the digit; press "." and get ".". Pure identity,
// nothing to learn.
//
// (This was uppercase-unshifted at first, on the reasoning that Gen 1 names are all
// caps. Twilight rejected it, and she's right: it's the one place the deck would have
// contradicted every keyboard the user has ever touched. Caps Lock is how you type a
// name in caps -- see effectivePage().)
const int kPageLetters[FontKeyboard::keyTotal] = {
    0, 247, 248, 249, 250, 251, 252, 253, 254, 255, 246, 227,   0, //   1..0  -
  176, 182, 164, 177, 179, 184, 180, 168, 174, 175, 158, 159,   0, // q..p  [  ]
  160, 178, 163, 165, 166, 167, 169, 170, 171, 157, 224,          // a..l  ;  '
  185, 183, 162, 181, 161, 173, 172, 244, 232, 243,               // z..m  ,  .  /
};

// Shift -- UPPERCASE + the Normal symbols, each on the key a real keyboard puts it on
// wherever the game HAS that glyph:
//   Shift+1 = !     Shift+4 = $     Shift+8 = x (the "*" key -- multiply)
//   Shift+9 = (     Shift+0 = )     Shift+; = :     Shift+' = "     Shift+/ = ?
// The glyphs the game simply doesn't have (@ # % ^ & _ + { } | < >) leave their keys
// empty -- except that the five spare number-row slots take the Normal characters with
// nowhere else to be: the gender signs and Pk/Mn.
//
// Pages 0+1 together are the entire in-game-legal character set: everything you can
// type on the real Game Boy's name screen.
const int kPageCaps[FontKeyboard::keyTotal] = {
    0, 231, 239, 245, 240, 225, 226,   0, 241, 154, 155,   0,   0, //  ! M F $ Pk Mn _ x ( )
  144, 150, 132, 145, 147, 152, 148, 136, 142, 143,   0,   0,   0, // Q..P
  128, 146, 131, 133, 134, 135, 137, 138, 139, 156, 114,          // A..L  :  "
  153, 151, 130, 149, 129, 141, 140,   0,   0, 230,               // Z..M  ?
};

// Ctrl -- SYMBOLS. The 13 bold letters sit on their own letters, so Ctrl+B is bold B --
// the same thing it means in every other program on earth. The rest is the punctuation
// that has a real partner one layer up: the closing double quote under the opening one
// on ', the narrow colon under ":" on ;, the ellipsis on "." (three dots on the dot
// key), and the middle dot on ",".
//
// The alternate period `<.>` sits on the spare "=" key, NOT on "/" where it was: a
// period that appears on the SLASH key -- while a perfectly good period sits on the
// period key one layer down -- is a false affordance. If a tile can go where a real
// keyboard would put it, it goes there; if it can't, it must not pretend.
const int kPageSymbols[FontKeyboard::keyTotal] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 116, //  middle-dot on "="
    0,   0, 100,   0,   0,   0,   0, 104,   0,   0, 112, 113,   0, // boldE boldI  '  '
   96, 106,  99, 101, 102, 103,   0, 186, 107, 109, 115,          // boldA..H  e-acute boldL  :(narrow)  "
    0,   0,  98, 105,  97,   0, 108, 242, 117,   0,               // boldC boldV boldB boldM  .(alt)  ...
};

// Alt -- CODES. Contractions sit on their own letter ('s on S, 't on T...). The
// variables pair up: Player on P with Rival ("Opponent") right beside it on O, and the
// two usage codes on Y ("Your usage") / U ("User"). The kana -- the rarest things here
// -- take the number row.
const int kPageCodes[FontKeyboard::keyTotal] = {
    0, 118, 110, 111, 119, 120, 233, 234, 235,   0,   0,   0,   0, // a i u e o + small a u e
    0,   0,  84, 228, 190,  89,  90,   0,  83,  82,   0,   0,   0, // Poke 'r 't targ user rival player
    0, 189, 187,  86,  94,  93,  92,  74, 188,   0,   0,          // 's 'd ...... ROCKET TRAINER TM PkMn 'l
    0,   0,  91, 191,   0,   0, 229,   0,   0,   0,               // PC 'v 'm
};

// Shift+Ctrl -- TILES I. The box-frame glyphs are laid out AS A BOX you can type:
//
//        Q  W  E          ╔  ═  ╗
//        A  S  D    -->   ║     ║      (S is the box's hollow middle, so it's empty)
//        Z  X  C          ╚  ═  ╝
//
// so a border is literally "Q W W W E" then "Z X X X C". Twilight's idea, and it's a
// better one than parking them in reading order: the keys ARE the picture.
//
// This is why the horizontal (122) and vertical (124) edges appear TWICE on this page --
// deliberately, and the only duplicated tiles anywhere in the map. You cannot draw a box
// with one vertical edge on one side. `tst_font_keyboard` pins exactly this: every tile
// reachable, and these two the only ones reachable from more than one key.
//
// The three cursor arrows take , . / -- they're arrow-ish keys, and it keeps the whole
// "things that point" family together.
// (tile14 sits on the "1" key by request -- it's the one people reach for -- so it and
// the tile that would have been there simply trade places.)
const int kPageTiles1[FontKeyboard::keyTotal] = {
    1,  20,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13, // tile14 on "1"
  121, 122, 123,  14,  15,  16,  17,  18,  19,   2,  21,  22,  23, // ╔ ═ ╗ + tiles
  124,   0, 124,  24,  25,  26,  27,  28,  29,  30,  31,          // ║ (hollow) ║ + tiles
  125, 122, 126,  32,  33,  34,  35, 237, 238, 236,               // ╚ ═ ╝ + tiles, arrows on , . /
};

// Shift+Alt -- TILES II. The rest of the real tileset: tile24..tile48 + tile4D.
const int kPageTiles2[FontKeyboard::keyTotal] = {
   36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
   49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
   62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,          // ...tile48
   77,   0,   0,   0,   0,   0,   0,   0,   0,   0,               // tile4D
};

// Ctrl+Alt -- TILES III. tileC0..tileDF -- the high half of the tilemap, which is
// USUALLY BLANK. These only hold anything in particular game states, which is exactly
// why they're all together on their own page behind their own chord, with a description
// that says so rather than leaving you wondering why the keys look empty.
const int kPageTiles3[FontKeyboard::keyTotal] = {
  192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
  205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
  218, 219, 220, 221, 222, 223,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

// Shift+Ctrl+Alt -- CONTROLS. The codes that GLITCH a name live behind the most
// deliberate key combination on the deck -- you cannot hit <end> by accident. Mnemonic
// letters throughout (E=end, L=line, N=next, P=page, X=dex...).
const int kPageControls[FontKeyboard::keyTotal] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,  80,  88,   0,   0,   0,   0,   0,  73,   0,   0,   0, // end prompt ... page
   81,   0,  87,   0,   0,   0,   0,   0,  79,   0,   0,          // para done line
    0,  95,  85,  75,  76,  78,   0,   0,   0,   0,               // dex cont cont_ autocont next
};

// A page's INDEX IS ITS MODIFIER MASK (shift = 1, ctrl = 2, alt = 4). That's the
// whole trick: pageFor() is just the mask, no lookup, and the deck can never show a
// page that disagrees with the keys you're holding.
const int* const kPages[FontKeyboard::pageTotal] = {
  kPageLetters,   // 0 = ---              (nothing held)
  kPageCaps,      // 1 = Shift
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
  "Letters", "Uppercase", "Symbols", "Tiles I",
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

// A quiet line under the deck saying what you're looking at. Indexed by mask.
const char* const kPageDescs[FontKeyboard::pageTotal] = {
  "The characters the game itself lets you type into a name.",
  "The same set, shifted -- capitals and the punctuation above the number row.",
  "Bold letters and the extra marks: accents, quotes, dots.",
  "Tiles from the loaded tileset. Q W E / A D / Z X C draws a box.",
  "One byte, many characters: names pulled from memory, and words the game spells out for you.",
  "The rest of the loaded tileset -- these change as you play.",
  "Usually blank: these only hold anything in certain game states.",
  "Text-engine codes. These do not print -- they END, WRAP or PAUSE the text, and in a name they glitch it.",
};

// What a REAL keyboard produces for this key, so a tile that lands on its natural key
// can drop the little legend in the corner (there is nothing to teach: the key IS the
// character). US layout, unshifted then shifted.
const char* const kNaturalPlain[] = {
  "`","1","2","3","4","5","6","7","8","9","0","-","=",
  "[","]","\\",";","'",",",".","/",
};
const char* const kNaturalShift[] = {
  "~","!","@","#","$","%","^","&","*","(",")","_","+",
  "{","}","|",":","\"","<",">","?",
};
constexpr int kNaturalCount = 21;

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

int FontKeyboard::effectivePage(bool shift, bool ctrl, bool alt, bool caps)
{
  // CAPS LOCK LOCKS THE SHIFT PAGE. It is a page selector like every other modifier
  // here -- not a per-key letter-case rule (2026-07-11, Twilight).
  //
  // The first cut gave caps its real-keyboard behaviour: letters only, leaving the
  // number row typing digits. That is what a physical keyboard does, but it produces a
  // layer that is NOT one of the 8 pages -- uppercase letters over an unshifted number
  // row -- so the page strip could not say where you were, and it read as a bug
  // ("why are there two different page 2s?"). A model the UI cannot display is a bad
  // model however correct it is. So caps now moves the WHOLE page, and every state the
  // deck can be in is exactly one page.
  //
  // Two rules keep it sane:
  //   * Shift INVERTS it (shift XOR caps) -- caps + Shift is the base page, just as
  //     caps + Shift is lowercase on a real keyboard.
  //   * Ctrl/Alt IGNORE it -- Ctrl+B is bold B with the caps light on or off. Without
  //     this, typing a name in caps and then reaching for Ctrl would land you on Tiles
  //     I instead of Symbols, which nobody would expect or want.
  //
  // The cost is that the punctuation row rides along with caps, so typing a digit
  // means tapping caps off. That is a rare need (digits aren't even in-game-legal in a
  // name), and the deck SHOWS the number row change, so nothing is hidden.
  if(ctrl || alt)
    return pageFor(shift, ctrl, alt);

  return pageFor(shift != caps, false, false);   // XOR
}

QVariantMap FontKeyboard::keyDataFor(const QString& key,
                                     bool shift, bool ctrl, bool alt, bool caps) const
{
  return keyData(effectivePage(shift, ctrl, alt, caps), key);
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

QString FontKeyboard::naturalChar(const QString& key, bool shift)
{
  if(key.size() == 1) {
    const QChar c = key.at(0);
    if(c.isLetter())
      return shift ? key.toUpper() : key.toLower();
  }

  for(int i = 0; i < kNaturalCount; i++) {
    if(key != QString(kNaturalPlain[i]))
      continue;

    return QString(shift ? kNaturalShift[i] : kNaturalPlain[i]);
  }

  return QString();
}

QString FontKeyboard::pageDescription(int page)
{
  if(page < 0 || page >= pageTotal)
    return QString();

  return QString(kPageDescs[page]);
}

QVariantMap FontKeyboard::keyData(int page, const QString& key) const
{
  return dataForInd(indFor(page, key), key.toUpper(), page);
}

QVariantMap FontKeyboard::spaceData() const
{
  return dataForInd(kSpaceInd, QStringLiteral("Space"), 0);
}

QVariantMap FontKeyboard::dataForInd(int ind, const QString& key, int page) const
{
  QVariantMap ret;

  ret["key"] = key;
  ret["ind"] = ind;

  // Does this key produce, on THIS page, exactly what the same key + the same modifiers
  // would produce on a real keyboard? (Shift+1 = "!", the A key = "a", "." = ".") If so
  // the corner legend is noise -- the key is telling you what it types by BEING that
  // key. Ctrl/Alt pages never match: a real keyboard produces no character for those, so
  // those keys always keep their legend.
  const bool chorded = (page & 2) || (page & 4);
  const QString natural = chorded
      ? QString()
      : naturalChar(key, (page & 1) != 0);

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
    ret["natural"] = false;
    return ret;
  }

  const QString alias = f->getAlias();

  ret["empty"] = false;
  ret["code"] = f->getName();
  ret["title"] = (alias != "") ? alias : f->getName();
  ret["tip"] = f->getTip();
  ret["category"] = static_cast<int>(categoryOf(f));
  ret["render"] = static_cast<int>(renderOf(f));
  // Two ways a corner legend earns its silence:
  //
  //  1. `natural` -- the deck types exactly what a real keyboard would for this key +
  //     these modifiers ("a" on A, "!" on Shift+1).
  //  2. the tile IS this key's letter, whatever the chord: bold A (`<A>`) on the A key
  //     is still, unmistakably, A. Printing a little "A" in the corner of a key showing
  //     a big bold A tells the user nothing they can't see.
  //
  // Note this deliberately does NOT catch `<'s>` on S: that tile prints "'s", not "s",
  // and the legend is the only thing saying which key produced it.
  QString bare = f->getName();
  if(bare.startsWith('<') && bare.endsWith('>'))
    bare = bare.mid(1, bare.size() - 2);

  const bool sameLetter = (bare.size() == 1)
      && (bare.compare(key, Qt::CaseInsensitive) == 0);

  ret["natural"] = (!natural.isEmpty() && natural == f->getName()) || sameLetter;

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
