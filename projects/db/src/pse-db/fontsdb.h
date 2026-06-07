/*
  * Copyright 2019 Twilight
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
#include <QHash>
#include <QVector>
#include <QJsonValue>
#include <QScopedPointer>

#include "./db_autoport.h"

class FontSearch;
class FontDBEntry;
class QQmlEngine;

/**
 * @brief The font database -- the in-game character set and the text codec.
 *
 * More than a lookup table: FontsDB owns the glyph entries (keyed by name) AND the
 * conversion between human-readable strings and the game's font-code bytes. This is
 * what powers the name editors and the on-screen keyboard. Beyond the standard
 * DB-singleton accessors it exposes:
 * - @ref convertToCode / @ref convertFromCode -- string <-> font-code round trip
 *   (special tokens like `<m>` / `<pic01>` give access to all 255 characters).
 * - @ref expandStr -- expand an English-format string into its in-game form,
 *   substituting rival/player names.
 * - @ref countSizeOf / @ref countSizeOfExpanded -- measure rendered text length.
 * - @ref search / @ref searchRaw -- obtain a FontSearch to filter glyphs.
 *
 * @note Many of these are expensive per call (noted inline). The two `search*`
 *       accessors differ only in ownership (see the inline note) -- one for QML,
 *       one C++-owned via a smart pointer.
 * @see FontDBEntry (a glyph), FontSearch (the filter), FontFilter helpers.
 */
class DB_AUTOPORT FontsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)          ///< Number of font glyphs.
  Q_PROPERTY(FontSearch* search READ searchRaw STORED false)       ///< A fresh glyph finder (QML-owned; see note).

public:
  // Get Instance
  static FontsDB* inst(); ///< The process-wide FontsDB singleton.

  // Get Properties, includes QML array helpers
  const QVector<FontDBEntry*> getStore() const;       ///< All glyphs.
  const QHash<QString, FontDBEntry*> getInd() const;  ///< Name->glyph index.
  int getStoreSize() const;                           ///< Glyph count.

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE FontDBEntry* getStoreAt(const int ind) const;     ///< Glyph by store index (for QML).
  Q_INVOKABLE FontDBEntry* getIndAt(const QString val) const;   ///< Glyph by name key (for QML).
  Q_INVOKABLE const FontDBEntry* getStoreByVal(int ind) const;  ///< Glyph by its font-code value.

  // QML-friendly aliases used throughout the UI
  Q_INVOKABLE FontDBEntry* fontAt(int ind) const; ///< Alias for getStoreAt (1-based-friendly UI accessor).
  Q_INVOKABLE int fontCount() const;              ///< Alias for getStoreSize.

  // Pull up a finder to narrow down fonts based on criteria
  // You must delete this when done
  // If you want to change parameters or startover you must obtain a new one
  // everytime

  // QML has such limited type compatibility, searchRaw is ambiguously owned and
  // must be manually deleted which works for QML. search is incompatible with
  // qml but is handled through a smart pointer and is explcitly owned by C++
  FontSearch* searchRaw() const; ///< Raw finder for QML (caller must delete; see note).
  QScopedPointer<FontSearch, QScopedPointerDeleteLater> search() const; ///< C++-owned finder (smart pointer).

  // Converts a string to in-game font characters
  // The string represents font characters, so special characters like
  // <m> or <pic01> also apply allowing for all 255 font chracters to be
  // leveraged with a simple keyboard
  // Very complex and expensive operation
  const QVector<int> convertToCode(
      QString str, int maxLen = 11, const bool autoEnd = true) const; ///< String -> font codes (see note; expensive).

  // The opposite, converts a series of codes to a string
  // Very simple and fast operation
  const QString convertFromCode(const QVector<int> codes,
                                const int maxLen = 11) const; ///< Font codes -> string (fast).

  // Converts an english format string to code represented as how it would be
  // in-game
  const QString expandStr(const QString msg, const int maxLen,
                          const QString rival, const QString player) const; ///< Expand English text to in-game form (substitutes rival/player).

  // Because counting text size is complicated, this makes it just 1 function
  // call and accessible from QML
  // Like most of these methods, countSizeOf can be very expensive on each call
  Q_INVOKABLE int countSizeOf(const QString val) const;         ///< Rendered length of @p val (expensive).
  Q_INVOKABLE int countSizeOfExpanded(const QString val) const; ///< Rendered length after expandStr (expensive).

public slots:
  void load();   ///< Load glyphs from JSON.
  void index();  ///< Build the name->glyph index.
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  // Singleton Constructor
  FontsDB(); ///< Private -- use inst().

  QVector<FontDBEntry*> store;       ///< The loaded glyphs.
  QHash<QString, FontDBEntry*> ind;  ///< Name->glyph lookup.

  // Something like splice, Kind of miss splice in Javascript
  void splice(QVector<int>& out, const QString in, const int position) const; ///< Insert @p in into @p out at @p position (JS-splice-like).
};
