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
#ifndef ROUTER_H
#define ROUTER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QHash>

// An individual screen
struct Screen : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool modal MEMBER modal NOTIFY modalChanged)
  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
  Q_PROPERTY(QString url MEMBER url NOTIFY urlChanged)

signals:
  void modalChanged();
  void titleChanged();
  void urlChanged();

public:
  Screen();
  Screen(bool modal, QString title, QString url);

  // Open this screen as a modal, taking up the entire window
  bool modal = false;

  // Name of this screen
  QString title = "";

  // URL of the screen to the QML file
  QString url = "";
};

// Router for screens
class Router : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
  Q_PROPERTY(QString curScrn MEMBER curScrn NOTIFY curScrnChanged)

signals:
  void goHome();
  void openModal(QString url);
  void openNonModal(QString url);

  void titleChanged();
  void curScrnChanged();

public:
  Q_INVOKABLE void changeScreen(QString name);

  QString title = "";
  QString curScrn = "";

  static void loadScreens();
  static QHash<QString, Screen*> screens;
};

#endif // ROUTER_H
