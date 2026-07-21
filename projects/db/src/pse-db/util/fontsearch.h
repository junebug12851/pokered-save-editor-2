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
#include <QVector>
#include <QString>

#include "../db_autoport.h"

class FontsDB;
class FontDBEntry;
class QQmlEngine;

/**
 * @brief A chainable filter ("finder") over the font glyphs.
 *
 * Obtained from FontsDB (search()/searchRaw()). Starts with all glyphs in
 * @ref results and narrows them with chainable calls -- each `and*` / `not*`
 * method returns `this`, so the UI/keyboard can write
 * `search()->andNormal()->notControl()`. keepAnyOf() is the one OR-combiner (see
 * its note); the rest are AND filters. @ref fontAt / @ref getFonts read the result.
 *
 * @note `fontCount` is live and notifies, so a bound view updates as filters apply.
 * @see FontsDB, FontDBEntry, MapSearch (the analogous map finder).
 */
class DB_AUTOPORT FontSearch : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int fontCount READ getFontCount NOTIFY fontCountChanged STORED false) ///< Live count of current results.

signals:
  void fontCountChanged(); ///< Result count changed (after a filter).

public:
  FontSearch(); ///< Start with all glyphs in the result set.

  Q_INVOKABLE FontSearch* startOver(); ///< Reset to all glyphs. Returns @c this.
  Q_INVOKABLE FontSearch* clear();     ///< Empty the result set. Returns @c this.

  // Sets results to the UNION of the selected categories — an entry is kept if
  // it matches ANY enabled trait (OR, not AND). Nothing enabled → empty.
  Q_INVOKABLE FontSearch* keepAnyOf(bool normal, bool control, bool picture,
                                    bool singleChar, bool multiChar, bool variable); ///< OR-union filter (see note). Returns @c this.
  Q_INVOKABLE FontSearch* andShorthand();   ///< Keep only shorthand glyphs. Returns @c this.
  Q_INVOKABLE FontSearch* notShorthand();   ///< Drop shorthand glyphs. Returns @c this.
  Q_INVOKABLE FontSearch* andNormal();      ///< Keep only normal glyphs.
  Q_INVOKABLE FontSearch* notNormal();      ///< Drop normal glyphs.
  Q_INVOKABLE FontSearch* andControl();     ///< Keep only control glyphs.
  Q_INVOKABLE FontSearch* notControl();     ///< Drop control glyphs.
  Q_INVOKABLE FontSearch* andPicture();     ///< Keep only picture glyphs.
  Q_INVOKABLE FontSearch* notPicture();     ///< Drop picture glyphs.
  Q_INVOKABLE FontSearch* andSingleChar();  ///< Keep only single-char glyphs.
  Q_INVOKABLE FontSearch* notSingleChar();  ///< Drop single-char glyphs.
  Q_INVOKABLE FontSearch* andMultiChar();   ///< Keep only multi-char glyphs.
  Q_INVOKABLE FontSearch* notMultiChar();   ///< Drop multi-char glyphs.
  Q_INVOKABLE FontSearch* andVariable();    ///< Keep only variable glyphs.
  Q_INVOKABLE FontSearch* notVariable();    ///< Drop variable glyphs.

  // QML Interface
  const QVector<FontDBEntry*> getFonts() const; ///< The current result set.
  int getFontCount() const;                     ///< Result count (backs @c fontCount).
  Q_INVOKABLE const FontDBEntry* fontAt(const int ind) const; ///< Result @p ind (for QML).

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  QVector<FontDBEntry*> results; ///< The current (narrowed) glyph set.
};
