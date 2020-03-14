/*
  * Copyright 2019 June Hanabi
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
#ifndef FONTS_H
#define FONTS_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QVector>
#include <QJsonValue>
#include <pse-common/types.h>

class FontSearch;

// With amazing help of Quicktype!!!
// https://app.quicktype.io

// There are 255 font options although most of them are "invalid" and thus
// use the tilemap.

struct FontDBEntry : public QObject {
  Q_OBJECT

  Q_PROPERTY(int ind MEMBER ind NOTIFY indChanged)
  Q_PROPERTY(QString name MEMBER name NOTIFY nameChanged)
  Q_PROPERTY(bool shorthand MEMBER shorthand NOTIFY shorthandChanged)
  Q_PROPERTY(bool picture MEMBER picture NOTIFY pictureChanged)
  Q_PROPERTY(int length MEMBER length NOTIFY lengthChanged)
  Q_PROPERTY(QString alias MEMBER alias NOTIFY aliasChanged)
  Q_PROPERTY(QString tip MEMBER tip NOTIFY tipChanged)
  Q_PROPERTY(bool control MEMBER control NOTIFY controlChanged)
  Q_PROPERTY(bool multiChar MEMBER multiChar NOTIFY multiCharChanged)
  Q_PROPERTY(bool variable MEMBER variable NOTIFY variableChanged)
  Q_PROPERTY(bool singleChar MEMBER singleChar NOTIFY singleCharChanged)
  Q_PROPERTY(bool normal MEMBER normal NOTIFY normalChanged)

signals:
  void indChanged();
  void nameChanged();
  void shorthandChanged();
  void pictureChanged();
  void lengthChanged();
  void aliasChanged();
  void tipChanged();
  void controlChanged();
  void multiCharChanged();
  void variableChanged();
  void singleCharChanged();
  void normalChanged();

public:
  FontDBEntry();
  FontDBEntry(QJsonValue& data);

  // Optional values are only present when true, so we simplify things
  // and mark then false unless they're present skipping dealing with variant

  int ind; // Font Code
  QString name; // Font Output
  bool shorthand = false; // Is Font Output shorthand for longer output?
  bool picture = false; // Does this use the tilemap and not the font
  int length; // Font output length or potentially maximum length
  QString alias; // Alternate font display name
  QString tip; // Font details
  bool control = false; // Is this a control character
  bool multiChar = false; // Does this output a length greater than 1
  bool variable = false; // Does this output a variable
  bool singleChar = false; // Does this output a single char
  bool normal = false; // Would this be an in-game accessible font char
};

class FontsDB : public QObject
{
  Q_OBJECT

public:
  static void load();
  static void index();

  // Pull up a finder to narrow down fonts based on criteria
  // You must delete this when done
  // If you want to change parameters or startover you must obtain a new one
  // everytime
  Q_INVOKABLE static FontSearch* search();

  // Converts a string to in-game font characters
  // The string represents font characters, so special characters like
  // <m> or <pic01> also apply allowing for all 255 font chracters to be
  // leveraged with a simple keyboard
  // Very complex and expensive operation
  static QVector<var8> convertToCode(
      QString str, var8 maxLen = 11, bool autoEnd = true);

  // The opposite, converts a series of codes to a string
  // Very simple and fast operation
  static QString convertFromCode(QVector<var8> codes, var8 maxLen = 11);

  // Font characters also include variables such as "<player>" to insert player
  // name. The above method converts to and from "<player>" and code but
  // this expands the string to include the players name spelled out.
  // Use this to give a sort of "preview" as to how it would look in-game.
  // It's complex and expensive
  static QString expandStr(
      QString msg, var8 maxLen, QString rival = "BLUE", QString player = "RED");

  // QML Interface
  Q_INVOKABLE static int fontCount();
  Q_INVOKABLE static FontDBEntry* fontAt(int code);
  Q_INVOKABLE static FontDBEntry* fontLookup(QString val);

  // Because counting text size is complicated, this makes it just 1 function
  // call and accessible from QML
  Q_INVOKABLE static int countSizeOf(QString val);
  Q_INVOKABLE static int countSizeOfExpanded(QString val);

  static QVector<FontDBEntry*> store;
  static QHash<QString, FontDBEntry*> ind;

private:
  static void splice(QVector<var8>& out, QString in, var8 ind);
};

#endif // FONTS_H
