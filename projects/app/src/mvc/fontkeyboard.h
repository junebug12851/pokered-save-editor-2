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
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief The full keyboard's tile->key map: which game tile sits on which physical key.
 *
 * Backs the redesigned full-screen name editor (`screens/modal/FullKeyboard.qml`),
 * which draws a real ASDF/QWERTY deck whose 36 alphanumeric keys each carry one
 * game tile. There are 255 tiles and 36 keys, so the deck has **8 pages** -- and
 * Shift/Ctrl/Alt give exactly 8 modifier combinations, one per page.
 *
 * The map itself (@ref pageInd) is a hand-authored table, not a generated fill. Its
 * doctrine, in priority order:
 *
 *  1. **Identity beats everything.** If a tile *is* a key it lives on that key --
 *     page 1 is A-Z + 0-9, page 2 (Shift) is a-z, exactly like a real keyboard.
 *  2. **Mnemonic beats ergonomics.** `'s` on **S**, bold B on **B** (Ctrl+B!),
 *     `<player>` on **P**, `<pc>` on **C**, `<dex>` on **X**. The key legend printed
 *     on each cap makes an awkward-but-memorable key beat a comfy random one.
 *  3. **Ergonomics breaks ties.** Otherwise the more-used tile gets the better key
 *     (home row, then upper, then bottom, then the number row).
 *
 * Pages follow the category order (Normal, Single-Char, Multi-Char, Variable,
 * Picture, Control), so the four cheapest modifier combos hold everything you'd ever
 * legitimately put in a name, and the two categories that *glitch* a name sit behind
 * two- and three-key combos.
 *
 * The **Space tile is the one exception**: it rides the deck's real spacebar (@ref
 * spaceInd), which is where every human already expects it. That is the only tile not
 * on an alphanumeric key.
 *
 * Every one of the 255 tiles appears exactly once across the 8 pages + the spacebar.
 * That invariant is not a comment -- it is pinned by `tst_font_keyboard`.
 *
 * @note A page's INDEX IS ITS MODIFIER MASK (shift = 1, ctrl = 2, alt = 4), so Alt is
 *       page **4**, not "the 4th page". @ref pageFor is therefore just the mask, and
 *       the deck can never show a page that disagrees with the keys being held. The
 *       human reading order (by category, cheapest chord first) is @ref pageOrder --
 *       don't confuse the two, or two whole pages silently swap.
 *
 * Exposed to QML as `brg.keyboard`.
 *
 * @see FontsDB (the tiles), notes/plans/full-keyboard-redesign.md (the full design).
 */
class FontKeyboard : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int page READ getPage WRITE setPage NOTIFY pageChanged)   ///< Current page = its modifier mask, 0-7.
  Q_PROPERTY(QStringList pageNames READ getPageNames CONSTANT)         ///< Display name of each page (indexed by mask).
  Q_PROPERTY(QVariantList pageOrder READ getPageOrder CONSTANT)        ///< Pages in the order a human should meet them.
  Q_PROPERTY(int pageCount READ getPageCount CONSTANT)                 ///< How many pages (8).

public:
  /// How a key draws its tile. The tile sheet can't render everything.
  enum Render {
    RenderEmpty = 0,   ///< No tile on this key.
    RenderTile,        ///< A single 8x8 tile, clipped from the shared animated sheet.
    RenderPreview,     ///< Multi-char/variable: render the *expanded* text (`<poke>` -> "Poke").
    RenderLabel,       ///< Control code: no glyph at all, so the cap shows its name.
  };
  Q_ENUM(Render)

  /// Tile category. Order matches the old filter strip = the usage order.
  enum Category {
    CatNone = 0,
    CatNormal,
    CatSingle,
    CatMulti,
    CatVariable,
    CatPicture,
    CatControl,
  };
  Q_ENUM(Category)

  explicit FontKeyboard(QObject* parent = nullptr);

  static constexpr int pageTotal = 8;  ///< Modifier combos: none/S/C/A/SC/SA/CA/SCA.
  /// Assignable keys per page: 26 letters + 10 digits + the 11 punctuation keys
  /// (` - = [ ] \ ; ' , . /). The punctuation keys were dead filler at first; they now
  /// carry the tiles that BELONG on them (`.` types `.`, Shift+`/` types `?`).
  static constexpr int keyTotal = 47;

  int getPage() const;                 ///< @see page property.
  void setPage(int val);               ///< @see page property.
  QStringList getPageNames() const;    ///< @see pageNames property.
  QVariantList getPageOrder() const;   ///< @see pageOrder property.
  int getPageCount() const;            ///< @see pageCount property.

  /// The 36 key labels in deck order (number row, QWERTY, ASDF, ZXCV). Test/QML helper.
  Q_INVOKABLE static QStringList keyLabels();

  /// Which page a modifier combination selects. The whole point of 8 pages / 8 combos.
  Q_INVOKABLE static int pageFor(bool shift, bool ctrl, bool alt);

  /// The page actually on show, once Caps Lock is taken into account.
  ///
  /// Caps LOCKS THE SHIFT PAGE (it is a page selector, not a per-key letter-case
  /// rule): Shift inverts it, Ctrl/Alt ignore it. So every state the deck can be in is
  /// exactly one of the 8 pages -- which is what lets the page strip always be right.
  /// @see the note in the .cpp for why the real-keyboard behaviour was dropped.
  Q_INVOKABLE static int effectivePage(bool shift, bool ctrl, bool alt, bool caps);

  /// keyData() for the page this key actually reads. What the deck binds to.
  Q_INVOKABLE QVariantMap keyDataFor(const QString& key,
                                     bool shift, bool ctrl, bool alt, bool caps) const;

  /// The modifier badge for a page, e.g. "" / "Shift" / "Ctrl+Alt".
  Q_INVOKABLE static QString pageBadge(int page);

  /// The font code on @p key of @p page, or 0 when that key is empty.
  Q_INVOKABLE static int indFor(int page, const QString& key);

  /// Everything a keycap needs: { ind, code, title, tip, category, render, empty }.
  /// Returns an empty-flagged map for an unmapped key -- never a null/undefined.
  Q_INVOKABLE QVariantMap keyData(int page, const QString& key) const;

  /// The same map for the spacebar (the Space tile lives there, not on a letter).
  Q_INVOKABLE QVariantMap spaceData() const;

  /// Drop the last *whole token* off a code string -- `"RED<player>"` -> `"RED"`.
  /// A backspace must never bite one character out of the middle of a `<code>`.
  Q_INVOKABLE static QString chopLastToken(const QString& str);

signals:
  void pageChanged();

private:
  QVariantMap dataForInd(int ind, const QString& key) const; ///< Shared body of keyData/spaceData.

  int page = 0; ///< @see page property.
};
