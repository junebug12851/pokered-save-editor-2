/*
  * Copyright 2026 Fairy Fox
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
 * @file tst_font_keyboard.cpp
 * @brief Pins FontKeyboard's tile->key map.
 *
 * The map is a hand-authored table of 8 pages x 36 keys. The failure modes that
 * matter are all SILENT ones: a tile that appears twice, or -- far worse -- a tile
 * that appears nowhere and is simply unreachable from the UI forever. Neither shows
 * up as a crash, a warning, or a visibly broken screen. So they're asserted here.
 */

#include <QtTest>
#include <QSet>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>

#include "../../app/src/mvc/fontkeyboard.h"

class TstFontKeyboard : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();

  void everyTileIsReachableExactlyOnce();
  void bordersAreLaidOutAsABox();
  void naturalKeysNeedNoLegend();
  void noPageOverflowsOrCollides();
  void everyMappedIndResolves();
  void identityContract_data();
  void identityContract();
  void pageForModifiers();
  void capsLockLocksTheShiftPage();
  void pageOrderCoversEveryPage();
  void categoriesAndRenderModes();
  void chopLastToken_data();
  void chopLastToken();
  void pageProperty();
};

void TstFontKeyboard::initTestCase()
{
  QVERIFY(DB::inst() != nullptr);
}

/**
 * THE invariant. 255 tiles; 254 of them on the 8x36 alphanumeric grid, plus Space
 * on the spacebar. Each exactly once -- no duplicate, and (the one that would be
 * unforgivable) no tile stranded with no key at all.
 */
void TstFontKeyboard::everyTileIsReachableExactlyOnce()
{
  FontKeyboard kb;
  QVector<int> seen(256, 0);

  for(int page = 0; page < FontKeyboard::pageTotal; page++) {
    for(const QString& key : FontKeyboard::keyLabels()) {
      const int ind = FontKeyboard::indFor(page, key);
      if(ind == 0)
        continue;

      QVERIFY2(ind >= 1 && ind <= 255,
               qPrintable(QString("page %1 key %2 -> out-of-range ind %3")
                          .arg(page).arg(key).arg(ind)));
      seen[ind]++;
    }
  }

  // The spacebar carries the 255th tile.
  const int spaceInd = kb.spaceData()["ind"].toInt();
  QCOMPARE(spaceInd, 127);
  seen[spaceInd]++;

  // The box-frame edges are the ONE deliberate exception: the horizontal (122) and
  // vertical (124) edges each sit on two keys of the Tiles I page, because you cannot
  // draw a box with a single vertical edge (see kPageTiles1 -- the keys are laid out AS
  // the box). Everything else is on exactly one key.
  const QSet<int> allowedTwice = { 122, 124 };

  QStringList missing;
  QStringList duplicated;

  for(int ind = 1; ind <= 255; ind++) {
    const int want = allowedTwice.contains(ind) ? 2 : 1;

    if(seen[ind] == 0)
      missing.append(QString::number(ind));
    else if(seen[ind] != want)
      duplicated.append(QString("%1 (x%2, expected x%3)").arg(ind).arg(seen[ind]).arg(want));
  }

  QVERIFY2(missing.isEmpty(),
           qPrintable("tiles with NO key (unreachable): " + missing.join(", ")));
  QVERIFY2(duplicated.isEmpty(),
           qPrintable("tiles on the wrong number of keys: " + duplicated.join(", ")));
}

/// The box-frame page IS the box: Q W E / A _ D / Z X C draws it. That's why two tiles
/// are duplicated, and it's worth pinning so nobody "tidies" it away.
void TstFontKeyboard::bordersAreLaidOutAsABox()
{
  FontKeyboard kb;
  const int p = FontKeyboard::pageFor(true, true, false);   // Shift+Ctrl -- Tiles I

  QCOMPARE(kb.keyData(p, "Q")["code"].toString(), QString("<ul>"));    // top-left
  QCOMPARE(kb.keyData(p, "W")["code"].toString(), QString("<horz>"));  // top edge
  QCOMPARE(kb.keyData(p, "E")["code"].toString(), QString("<ur>"));    // top-right
  QCOMPARE(kb.keyData(p, "A")["code"].toString(), QString("<vert>"));  // left edge
  QVERIFY(kb.keyData(p, "S")["empty"].toBool());                       // the hollow middle
  QCOMPARE(kb.keyData(p, "D")["code"].toString(), QString("<vert>"));  // right edge
  QCOMPARE(kb.keyData(p, "Z")["code"].toString(), QString("<bl>"));    // bottom-left
  QCOMPARE(kb.keyData(p, "X")["code"].toString(), QString("<horz>"));  // bottom edge
  QCOMPARE(kb.keyData(p, "C")["code"].toString(), QString("<br>"));    // bottom-right

  // The arrows keep the "things that point" family together on , . /
  QCOMPARE(kb.keyData(p, ",")["code"].toString(), QString("<arr-r>"));
  QCOMPARE(kb.keyData(p, ".")["code"].toString(), QString("<arr-d>"));
  QCOMPARE(kb.keyData(p, "/")["code"].toString(), QString("<arr-r2>"));
}

/// A tile that lands on the key a real keyboard would put it on is flagged `natural`,
/// and the cap then drops its corner legend -- the key IS the character, so a label
/// saying which key to press is noise. Chorded pages can never be natural.
void TstFontKeyboard::naturalKeysNeedNoLegend()
{
  FontKeyboard kb;

  QCOMPARE(FontKeyboard::naturalChar("A", false), QString("a"));
  QCOMPARE(FontKeyboard::naturalChar("A", true),  QString("A"));
  QCOMPARE(FontKeyboard::naturalChar("1", false), QString("1"));
  QCOMPARE(FontKeyboard::naturalChar("1", true),  QString("!"));
  QCOMPARE(FontKeyboard::naturalChar("/", true),  QString("?"));
  QCOMPARE(FontKeyboard::naturalChar(";", true),  QString(":"));

  // The base + shift layers are the keyboard, so they're natural...
  QVERIFY(kb.keyData(0, "A")["natural"].toBool());   // a
  QVERIFY(kb.keyData(0, "7")["natural"].toBool());   // 7
  QVERIFY(kb.keyData(0, ".")["natural"].toBool());   // .
  QVERIFY(kb.keyData(1, "A")["natural"].toBool());   // A
  QVERIFY(kb.keyData(1, "1")["natural"].toBool());   // !
  QVERIFY(kb.keyData(1, "9")["natural"].toBool());   // (
  QVERIFY(kb.keyData(1, "/")["natural"].toBool());   // ?
  QVERIFY(kb.keyData(1, ";")["natural"].toBool());   // :

  // ...but the ones with nowhere natural to be are not, and keep their legend.
  QVERIFY(!kb.keyData(1, "2")["natural"].toBool());  // male sign on the "@" key
  QVERIFY(!kb.keyData(1, "8")["natural"].toBool());  // x (multiply) on the "*" key

  // A bold letter on its own letter is silent too: a little "B" in the corner of a key
  // showing a big bold B tells you nothing you can't already see.
  QVERIFY(kb.keyData(2, "B")["natural"].toBool());   // bold B on B
  QVERIFY(kb.keyData(2, "S")["natural"].toBool());   // bold S on S

  // But a contraction is NOT its letter -- `<'s>` prints "'s", and the legend is the
  // only thing telling you which key made it.
  QVERIFY(!kb.keyData(4, "S")["natural"].toBool());  // 's
  QVERIFY(!kb.keyData(4, "P")["natural"].toBool());  // <player>
  QVERIFY(!kb.keyData(2, "K")["natural"].toBool());  // e-acute on K

  // ...and nothing may pretend: no period on a slash key while a real period exists.
  QCOMPARE(kb.keyData(2, "/")["ind"].toInt(), 0);
}

/// No page may map two tiles to the same key, and every page has exactly 36 keys.
void TstFontKeyboard::noPageOverflowsOrCollides()
{
  const QStringList keys = FontKeyboard::keyLabels();

  QCOMPARE(keys.size(), FontKeyboard::keyTotal);
  QCOMPARE(QSet<QString>(keys.begin(), keys.end()).size(), FontKeyboard::keyTotal);

  // The two box-frame edges are on two keys of the Tiles I page BY DESIGN (you can't
  // draw a box with one vertical edge). Nothing else may repeat within a page.
  const QSet<int> allowedTwice = { 122, 124 };

  for(int page = 0; page < FontKeyboard::pageTotal; page++) {
    QSet<int> used;

    for(const QString& key : keys) {
      const int ind = FontKeyboard::indFor(page, key);
      if(ind == 0 || allowedTwice.contains(ind))
        continue;

      QVERIFY2(!used.contains(ind),
               qPrintable(QString("page %1 maps ind %2 twice").arg(page).arg(ind)));
      used.insert(ind);
    }
  }
}

/// Every code in the table must be a real tile in FontsDB -- a typo'd number would
/// otherwise just render a blank key nobody notices.
void TstFontKeyboard::everyMappedIndResolves()
{
  FontKeyboard kb;

  for(int page = 0; page < FontKeyboard::pageTotal; page++) {
    for(const QString& key : FontKeyboard::keyLabels()) {
      const QVariantMap d = kb.keyData(page, key);
      const int ind = d["ind"].toInt();

      if(ind == 0) {
        QVERIFY(d["empty"].toBool());
        continue;
      }

      QVERIFY2(FontsDB::inst()->getStoreByVal(ind) != nullptr,
               qPrintable(QString("page %1 key %2 -> ind %3 is not in FontsDB")
                          .arg(page).arg(key).arg(ind)));
      QVERIFY(!d["empty"].toBool());
      QVERIFY(!d["code"].toString().isEmpty());
      QVERIFY(!d["title"].toString().isEmpty());
    }
  }
}

void TstFontKeyboard::identityContract_data()
{
  QTest::addColumn<int>("page");
  QTest::addColumn<QString>("key");
  QTest::addColumn<QString>("code");

  // The base layer: press a letter key, get the LOWERCASE letter; press a digit, get
  // the digit -- exactly like every keyboard the user has ever touched. If this ever
  // breaks, the whole design premise breaks with it.
  QTest::newRow("A is a")       << 0 << "A" << "a";
  QTest::newRow("Z is z")       << 0 << "Z" << "z";
  QTest::newRow("M is m")       << 0 << "M" << "m";
  QTest::newRow("7 is 7")       << 0 << "7" << "7";
  QTest::newRow("0 is 0")       << 0 << "0" << "0";

  // Punctuation is on ITS OWN KEY, not exiled to a number row it has no business being
  // on. This is the whole reason the deck grew from 36 keys to 47.
  QTest::newRow(". is .")       << 0 << "." << ".";
  QTest::newRow(", is ,")       << 0 << "," << ",";
  QTest::newRow("/ is /")       << 0 << "/" << "/";
  QTest::newRow("; is ;")       << 0 << ";" << ";";
  QTest::newRow("' is '")       << 0 << "'" << "'";
  QTest::newRow("- is -")       << 0 << "-" << "-";
  QTest::newRow("[ is [")       << 0 << "[" << "[";
  QTest::newRow("] is ]")       << 0 << "]" << "]";

  // Shift: uppercase, and the real shifted keys wherever the game has that glyph.
  QTest::newRow("shift A is A") << 1 << "A" << "A";
  QTest::newRow("shift Z is Z") << 1 << "Z" << "Z";
  QTest::newRow("shift 1 is !") << 1 << "1" << "!";
  QTest::newRow("shift 4 is $") << 1 << "4" << "$";
  QTest::newRow("shift 9 is (") << 1 << "9" << "(";
  QTest::newRow("shift 0 is )") << 1 << "0" << ")";
  QTest::newRow("shift 8 is x") << 1 << "8" << "<x>";   // the "*" key = multiply
  QTest::newRow("shift / is ?") << 1 << "/" << "?";
  QTest::newRow("shift ; is :") << 1 << ";" << ":";
  QTest::newRow("shift ' is \"") << 1 << "'" << "<o\">";

  // Page 3 (Ctrl): the bold letters sit on their own letters. Ctrl+B = bold B.
  QTest::newRow("ctrl B is bold B") << 2 << "B" << "<B>";
  QTest::newRow("ctrl S is bold S") << 2 << "S" << "<S>";

  // Alt (mask 4): mnemonics -- the contraction on its letter, the codes on theirs.
  QTest::newRow("alt S is 's")      << 4 << "S" << "<'s>";
  QTest::newRow("alt T is 't")      << 4 << "T" << "<'t>";
  QTest::newRow("alt P is player")  << 4 << "P" << "<player>";
  QTest::newRow("alt O is rival")   << 4 << "O" << "<rival>";
  QTest::newRow("alt C is pc")      << 4 << "C" << "<pc>";

  // Shift+Ctrl (mask 3): the frames are laid out AS a box -- pinned in full by
  // bordersAreLaidOutAsABox().
  QTest::newRow("frame corner on Q") << 3 << "Q" << "<ul>";
  QTest::newRow("frame corner on C") << 3 << "C" << "<br>";

  // Shift+Ctrl+Alt (mask 7): the dangerous codes, on mnemonic keys behind the
  // hardest chord on the deck.
  QTest::newRow("all E is end")     << 7 << "E" << "<end>";
  QTest::newRow("all X is dex")     << 7 << "X" << "<dex>";
}

void TstFontKeyboard::identityContract()
{
  QFETCH(int, page);
  QFETCH(QString, key);
  QFETCH(QString, code);

  FontKeyboard kb;
  QCOMPARE(kb.keyData(page, key)["code"].toString(), code);
}

/// 8 pages, 8 modifier combos, one each -- the fact that makes the design work.
/// A page's number IS its modifier mask (shift 1, ctrl 2, alt 4), so Alt is page 4.
void TstFontKeyboard::pageForModifiers()
{
  QCOMPARE(FontKeyboard::pageFor(false, false, false), 0);
  QCOMPARE(FontKeyboard::pageFor(true,  false, false), 1);
  QCOMPARE(FontKeyboard::pageFor(false, true,  false), 2);
  QCOMPARE(FontKeyboard::pageFor(true,  true,  false), 3);
  QCOMPARE(FontKeyboard::pageFor(false, false, true),  4);
  QCOMPARE(FontKeyboard::pageFor(true,  false, true),  5);
  QCOMPARE(FontKeyboard::pageFor(false, true,  true),  6);
  QCOMPARE(FontKeyboard::pageFor(true,  true,  true),  7);

  // Every combination lands on a distinct page: no page is unreachable by keyboard.
  QSet<int> pages;
  for(int i = 0; i < 8; i++)
    pages.insert(FontKeyboard::pageFor(i & 1, i & 2, i & 4));

  QCOMPARE(pages.size(), FontKeyboard::pageTotal);

  // The badge on a page must be the chord that actually reaches it -- a strip that
  // teaches the wrong shortcut is worse than no strip.
  for(int i = 0; i < FontKeyboard::pageTotal; i++) {
    const QString badge = FontKeyboard::pageBadge(i);

    QCOMPARE(badge.contains("Shift"), (i & 1) != 0);
    QCOMPARE(badge.contains("Ctrl"),  (i & 2) != 0);
    QCOMPARE(badge.contains("Alt"),   (i & 4) != 0);
  }
}

/// Caps Lock LOCKS THE SHIFT PAGE -- it is a page selector, not a per-key letter-case
/// rule. That means every state the deck can be in is exactly one of the 8 pages, so
/// the page strip is always telling the truth. Shift inverts caps; Ctrl/Alt ignore it.
///
/// (The real-keyboard behaviour -- caps on letters only, digits still digits -- was
/// built first and rejected: it produces a layer that ISN'T one of the pages, and the
/// deck then looks like it's lying about where you are.)
void TstFontKeyboard::capsLockLocksTheShiftPage()
{
  FontKeyboard kb;

  // Caps on = the Shift page, WHOLE. Letters uppercase...
  QCOMPARE(FontKeyboard::effectivePage(false, false, false, true), 1);
  QCOMPARE(kb.keyDataFor("A", false, false, false, true)["code"].toString(), QString("A"));
  QCOMPARE(kb.keyDataFor("K", false, false, false, true)["code"].toString(), QString("K"));

  // ...and the number row comes with it (this is the deliberate trade: one page at a
  // time, and the deck SHOWS the row change rather than hiding it).
  QCOMPARE(kb.keyDataFor("1", false, false, false, true)["code"].toString(), QString("!"));

  // Shift INVERTS caps: caps + Shift is the base page, as caps + Shift is lowercase on
  // any keyboard.
  QCOMPARE(FontKeyboard::effectivePage(true, false, false, true), 0);
  QCOMPARE(kb.keyDataFor("A", true, false, false, true)["code"].toString(), QString("a"));
  QCOMPARE(kb.keyDataFor("1", true, false, false, true)["code"].toString(), QString("1"));

  // Shift alone (no caps) is the same page as caps alone.
  QCOMPARE(FontKeyboard::effectivePage(true, false, false, false), 1);
  QCOMPARE(kb.keyDataFor("A", true, false, false, false)["code"].toString(), QString("A"));
  QCOMPARE(kb.keyDataFor("1", true, false, false, false)["code"].toString(), QString("!"));

  // Ctrl/Alt IGNORE caps: Ctrl+B is bold B with the caps light on or off. Without this,
  // typing a name in caps and then reaching for Ctrl would land on Tiles I instead of
  // Symbols -- which nobody would expect.
  QCOMPARE(FontKeyboard::effectivePage(false, true, false, true), 2);
  QCOMPARE(kb.keyDataFor("B", false, true, false, true)["code"].toString(), QString("<B>"));
  QCOMPARE(FontKeyboard::effectivePage(false, false, true, true), 4);
  QCOMPARE(kb.keyDataFor("S", false, false, true, true)["code"].toString(), QString("<'s>"));
  QCOMPARE(kb.keyDataFor("S", false, false, true, false)["code"].toString(), QString("<'s>"));

  // Caps off + nothing held = the base layer.
  QCOMPARE(FontKeyboard::effectivePage(false, false, false, false), 0);
  QCOMPARE(kb.keyDataFor("A", false, false, false, false)["code"].toString(), QString("a"));

  // Whatever the caps state, the deck is always on ONE of the 8 real pages.
  for(int i = 0; i < 16; i++) {
    const int p = FontKeyboard::effectivePage(i & 1, i & 2, i & 4, i & 8);
    QVERIFY(p >= 0 && p < FontKeyboard::pageTotal);
  }
}

/// The strip's reading order is by category (cheapest chord first), NOT the mask
/// order -- but it must still list every page exactly once.
void TstFontKeyboard::pageOrderCoversEveryPage()
{
  FontKeyboard kb;
  const QVariantList order = kb.getPageOrder();

  QCOMPARE(order.size(), FontKeyboard::pageTotal);

  QSet<int> seen;
  for(const QVariant& v : order)
    seen.insert(v.toInt());

  QCOMPARE(seen.size(), FontKeyboard::pageTotal);

  // Alt (a one-key chord) comes before Shift+Ctrl (a two-key one), which is the
  // whole reason this order isn't just 0..7.
  QVERIFY(order.indexOf(QVariant(4)) < order.indexOf(QVariant(3)));
}

/// The cap can't draw a control code as a tile (it has no glyph) nor a multi-char as
/// one tile (it isn't one). Those must come back as Label/Preview or the deck would
/// silently render garbage tiles.
void TstFontKeyboard::categoriesAndRenderModes()
{
  FontKeyboard kb;

  const QVariantMap upperA = kb.keyData(0, "A");
  QCOMPARE(upperA["category"].toInt(), static_cast<int>(FontKeyboard::CatNormal));
  QCOMPARE(upperA["render"].toInt(),   static_cast<int>(FontKeyboard::RenderTile));

  const QVariantMap digit7 = kb.keyData(0, "7");
  QCOMPARE(digit7["category"].toInt(), static_cast<int>(FontKeyboard::CatSingle));
  QCOMPARE(digit7["render"].toInt(),   static_cast<int>(FontKeyboard::RenderTile));

  const QVariantMap player = kb.keyData(4, "P");
  QCOMPARE(player["category"].toInt(), static_cast<int>(FontKeyboard::CatVariable));
  QCOMPARE(player["render"].toInt(),   static_cast<int>(FontKeyboard::RenderPreview));

  const QVariantMap pkmn = kb.keyData(4, "K");
  QCOMPARE(pkmn["category"].toInt(), static_cast<int>(FontKeyboard::CatMulti));
  QCOMPARE(pkmn["render"].toInt(),   static_cast<int>(FontKeyboard::RenderPreview));

  const QVariantMap tile = kb.keyData(3, "Q");
  QCOMPARE(tile["category"].toInt(), static_cast<int>(FontKeyboard::CatPicture));
  QCOMPARE(tile["render"].toInt(),   static_cast<int>(FontKeyboard::RenderTile));

  const QVariantMap end = kb.keyData(7, "E");
  QCOMPARE(end["category"].toInt(), static_cast<int>(FontKeyboard::CatControl));
  QCOMPARE(end["render"].toInt(),   static_cast<int>(FontKeyboard::RenderLabel));

  // An empty key is well-formed, not null.
  const QVariantMap empty = kb.keyData(2, "6");
  QVERIFY(empty["empty"].toBool());
  QCOMPARE(empty["ind"].toInt(), 0);
  QCOMPARE(empty["render"].toInt(), static_cast<int>(FontKeyboard::RenderEmpty));

  // The spacebar's tile really is Space.
  QCOMPARE(kb.spaceData()["code"].toString(), QString(" "));
}

void TstFontKeyboard::chopLastToken_data()
{
  QTest::addColumn<QString>("in");
  QTest::addColumn<QString>("out");

  QTest::newRow("empty")        << ""            << "";
  QTest::newRow("plain char")   << "RED"         << "RE";
  QTest::newRow("last char")    << "R"           << "";
  QTest::newRow("whole token")  << "RED<player>" << "RED";
  QTest::newRow("only a token") << "<player>"    << "";
  QTest::newRow("token then char") << "<player>x" << "<player>";
  QTest::newRow("two tokens")   << "<pk><mn>"    << "<pk>";
  QTest::newRow("quote token")  << "A<o\">"      << "A";
  QTest::newRow("stray gt")     << "A>"          << "A";
}

/// Backspace deletes a whole <code>, never one character out of the middle of one --
/// that would leave a string the codec can't round-trip.
void TstFontKeyboard::chopLastToken()
{
  QFETCH(QString, in);
  QFETCH(QString, out);

  QCOMPARE(FontKeyboard::chopLastToken(in), out);
}

void TstFontKeyboard::pageProperty()
{
  FontKeyboard kb;
  QSignalSpy spy(&kb, &FontKeyboard::pageChanged);

  QCOMPARE(kb.getPage(), 0);
  QCOMPARE(kb.getPageCount(), FontKeyboard::pageTotal);
  QCOMPARE(kb.getPageNames().size(), FontKeyboard::pageTotal);
  QCOMPARE(FontKeyboard::pageBadge(0), QString(""));
  QCOMPARE(FontKeyboard::pageBadge(4), QString("Alt"));
  QCOMPARE(FontKeyboard::pageBadge(7), QString("Shift+Ctrl+Alt"));

  kb.setPage(3);
  QCOMPARE(kb.getPage(), 3);
  QCOMPARE(spy.count(), 1);

  // Same page: no churn. Out of range: refused, not clamped into a wrong page.
  kb.setPage(3);
  kb.setPage(99);
  kb.setPage(-1);
  QCOMPARE(kb.getPage(), 3);
  QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TstFontKeyboard)
#include "tst_font_keyboard.moc"
