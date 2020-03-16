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
#include <QScopedPointer>

#include "./db_autoport.h"

class FontSearch;
class FontDBEntry;
class QQmlEngine;

class DB_AUTOPORT FontsDB : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int getStoreSize READ getStoreSize CONSTANT)
  Q_PROPERTY(FontSearch* search READ searchRaw STORED false)

public:
  // Get Instance
  static FontsDB* inst();

  // Get Properties, includes QML array helpers
  const QVector<FontDBEntry*> getStore() const;
  const QHash<QString, FontDBEntry*> getInd() const;
  int getStoreSize() const;

  // QML Methods that can't be a property or slot because they take an argument
  Q_INVOKABLE const FontDBEntry* getStoreAt(const int ind) const;
  Q_INVOKABLE const FontDBEntry* getIndAt(const QString val) const;
  Q_INVOKABLE const FontDBEntry* getStoreByVal(int ind) const;

  // Pull up a finder to narrow down fonts based on criteria
  // You must delete this when done
  // If you want to change parameters or startover you must obtain a new one
  // everytime

  // QML has such limited type compatibility, searchRaw is ambiguously owned and
  // must be manually deleted which works for QML. search is incompatible with
  // qml but is handled through a smart pointer and is explcitly owned by C++
  FontSearch* searchRaw() const;
  const QScopedPointer<const FontSearch, QScopedPointerDeleteLater> search() const;

  // Converts a string to in-game font characters
  // The string represents font characters, so special characters like
  // <m> or <pic01> also apply allowing for all 255 font chracters to be
  // leveraged with a simple keyboard
  // Very complex and expensive operation
  const QVector<int> convertToCode(
      QString str, int maxLen = 11, const bool autoEnd = true) const;

  // The opposite, converts a series of codes to a string
  // Very simple and fast operation
  const QString convertFromCode(const QVector<int> codes,
                                const int maxLen = 11) const;

  // Converts an english format string to code represented as how it would be
  // in-game
  const QString expandStr(const QString msg, const int maxLen,
                          const QString rival, const QString player) const;

  // Because counting text size is complicated, this makes it just 1 function
  // call and accessible from QML
  // Like most of these methods, countSizeOf can be very expensive on each call
  Q_INVOKABLE int countSizeOf(const QString val) const;
  Q_INVOKABLE int countSizeOfExpanded(const QString val) const;

public slots:
  void load();
  void index();
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  // Singleton Constructor
  FontsDB();

  // Something like splice, Kind of miss splice in Javascript
  void splice(QVector<int>& out, const QString in, const int ind) const;

  QVector<FontDBEntry*> store;
  QHash<QString, FontDBEntry*> ind;
};

#endif // FONTS_H
