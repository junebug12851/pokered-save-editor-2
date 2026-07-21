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
#include <QString>
#include <QJsonValue>

#include "../db_autoport.h"

class QQmlEngine;
class FontsDB;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// There are 255 font options although most of them are "invalid" and thus
// use the tilemap.

/**
 * @brief One in-game font character: its code, output text, and classification flags.
 *
 * QObject-getter style DB entry. Beyond the code/name it carries a set of boolean
 * classification flags (control/picture/variable/single-/multi-char/normal) that
 * the on-screen keyboard and FontSearch use to filter and categorise glyphs. The
 * field comments below define each flag. FontSearch is a friend so it can read the
 * filter fields.
 *
 * @see FontsDB, FontSearch, the name editors / keyboard.
 */
struct DB_AUTOPORT FontDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(int ind READ getInd CONSTANT)              ///< Font code.
  Q_PROPERTY(QString name READ getName CONSTANT)        ///< Output text.
  Q_PROPERTY(bool shorthand READ getShorthand CONSTANT) ///< Output is shorthand for longer text.
  Q_PROPERTY(bool picture READ getPicture CONSTANT)     ///< Uses the tilemap, not the font.
  Q_PROPERTY(int length READ getLength CONSTANT)        ///< Output length (or max length).
  Q_PROPERTY(QString alias READ getAlias CONSTANT)      ///< Alternate display name.
  Q_PROPERTY(QString tip READ getTip CONSTANT)          ///< Glyph details/tooltip.
  Q_PROPERTY(bool control READ getControl CONSTANT)     ///< Control character.
  Q_PROPERTY(bool multiChar READ getMultiChar CONSTANT) ///< Outputs more than one char.
  Q_PROPERTY(bool variable READ getVariable CONSTANT)   ///< Outputs a variable.
  Q_PROPERTY(bool singleChar READ getSingleChar CONSTANT) ///< Outputs a single char.
  Q_PROPERTY(bool normal READ getNormal CONSTANT)       ///< In-game accessible normal char.

public:
  int getInd() const;            ///< @see ind property.
  const QString getName() const; ///< @see name property.
  bool getShorthand() const;     ///< @see shorthand property.
  bool getPicture() const;       ///< @see picture property.
  int getLength() const;         ///< @see length property.
  const QString getAlias() const; ///< @see alias property.
  const QString getTip() const;  ///< @see tip property.
  bool getControl() const;       ///< @see control property.
  bool getMultiChar() const;     ///< @see multiChar property.
  bool getVariable() const;      ///< @see variable property.
  bool getSingleChar() const;    ///< @see singleChar property.
  bool getNormal() const;        ///< @see normal property.

public slots:
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

protected:
  FontDBEntry();                    ///< Empty entry (built by FontsDB).
  FontDBEntry(const QJsonValue& data); ///< Build from a JSON value.
  void qmlRegister() const;         ///< Register with QML.

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  int ind = 0; // Font Code
  QString name = ""; // Font Output
  bool shorthand = false; // Is Font Output shorthand for longer output?
  bool picture = false; // Does this use the tilemap and not the font
  int length = 0; // Font output length or potentially maximum length
  QString alias = ""; // Alternate font display name
  QString tip = ""; // Font details
  bool control = false; // Is this a control character
  bool multiChar = false; // Does this output a length greater than 1
  bool variable = false; // Does this output a variable
  bool singleChar = false; // Does this output a single char
  bool normal = false; // Would this be an in-game accessible font char

  friend class FontsDB;
  friend class FontSearch; // reads filter fields in deepLink
};
