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
#ifndef FONTDBENTRY_H
#define FONTDBENTRY_H

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

struct DB_AUTOPORT FontDBEntry : public QObject {
  Q_OBJECT
  Q_PROPERTY(int getInd READ getInd CONSTANT)
  Q_PROPERTY(QString getName READ getName CONSTANT)
  Q_PROPERTY(bool getShorthand READ getShorthand CONSTANT)
  Q_PROPERTY(bool getPicture READ getPicture CONSTANT)
  Q_PROPERTY(int getLength READ getLength CONSTANT)
  Q_PROPERTY(QString getAlias READ getAlias CONSTANT)
  Q_PROPERTY(QString getTip READ getTip CONSTANT)
  Q_PROPERTY(bool getControl READ getControl CONSTANT)
  Q_PROPERTY(bool getMultiChar READ getMultiChar CONSTANT)
  Q_PROPERTY(bool getVariable READ getVariable CONSTANT)
  Q_PROPERTY(bool getSingleChar READ getSingleChar CONSTANT)
  Q_PROPERTY(bool getNormal READ getNormal CONSTANT)

public:
  int getInd() const;
  const QString getName() const;
  bool getShorthand() const;
  bool getPicture() const;
  int getLength() const;
  const QString getAlias() const;
  const QString getTip() const;
  bool getControl() const;
  bool getMultiChar() const;
  bool getVariable() const;
  bool getSingleChar() const;
  bool getNormal() const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

protected:
  FontDBEntry();
  FontDBEntry(const QJsonValue& data);
  void qmlRegister() const;

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
};

#endif // FONTDBENTRY_H
