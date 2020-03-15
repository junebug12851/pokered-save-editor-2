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
#ifndef FONTSEARCH_H
#define FONTSEARCH_H

#include <QObject>
#include <QVector>
#include <QString>

#include "../db_autoport.h"

class FontsDB;
class FontDBEntry;
class QQmlEngine;

class DB_AUTOPORT FontSearch : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int fontCount READ getFontCount NOTIFY fontCountChanged STORED false)
  Q_PROPERTY(FontSearch* startOver READ startOver)
  Q_PROPERTY(FontSearch* andShorthand READ andShorthand)
  Q_PROPERTY(FontSearch* notShorthand READ notShorthand)
  Q_PROPERTY(FontSearch* andNormal READ andNormal)
  Q_PROPERTY(FontSearch* notNormal READ notNormal)
  Q_PROPERTY(FontSearch* andControl READ andControl)
  Q_PROPERTY(FontSearch* notControl READ notControl)
  Q_PROPERTY(FontSearch* andPicture READ andPicture)
  Q_PROPERTY(FontSearch* notPicture READ notPicture)
  Q_PROPERTY(FontSearch* andSingleChar READ andSingleChar)
  Q_PROPERTY(FontSearch* notSingleChar READ notSingleChar)
  Q_PROPERTY(FontSearch* andMultiChar READ andMultiChar)
  Q_PROPERTY(FontSearch* notMultiChar READ notMultiChar)
  Q_PROPERTY(FontSearch* andVariable READ andVariable)
  Q_PROPERTY(FontSearch* notVariable READ notVariable)

signals:
  void fontCountChanged();

public:
  FontSearch();

  FontSearch* startOver();
  FontSearch* andShorthand();
  FontSearch* notShorthand();
  FontSearch* andNormal();
  FontSearch* notNormal();
  FontSearch* andControl();
  FontSearch* notControl();
  FontSearch* andPicture();
  FontSearch* notPicture();
  FontSearch* andSingleChar();
  FontSearch* notSingleChar();
  FontSearch* andMultiChar();
  FontSearch* notMultiChar();
  FontSearch* andVariable();
  FontSearch* notVariable();

  // QML Interface
  const QVector<FontDBEntry*> getFonts() const;
  int getFontCount() const;
  Q_INVOKABLE const FontDBEntry* fontAt(const int ind) const;

public slots:
  void qmlProtect(const QQmlEngine* const engine) const;

private slots:
  void qmlRegister() const;

private:
  QVector<FontDBEntry*> results;
};

#endif // FONTSEARCH_H
