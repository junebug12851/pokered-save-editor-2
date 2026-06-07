/*
  * Copyright 2020 Twilight
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
#include <QUrl>
#include <QHash>
#include <QVector>

/**
 * @brief One registered screen: its QML url, title, and modal/home-button flags.
 *
 * A small record in the Router's screen registry. @ref modal screens take over
 * the whole window; @ref homeBtn controls whether the header shows a home button.
 *
 * @see Router.
 */
// An individual screen
struct Screen : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool modal MEMBER modal NOTIFY modalChanged)       ///< Opens as a full-window modal.
  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)    ///< Header title for the screen.
  Q_PROPERTY(QString url MEMBER url NOTIFY urlChanged)          ///< QML file url for the screen.
  Q_PROPERTY(bool homeBtn MEMBER homeBtn NOTIFY homeBtnChanged) ///< Whether the home button shows.

signals:
  void modalChanged();
  void titleChanged();
  void urlChanged();
  void homeBtnChanged();

public:
  Screen(); ///< Empty screen.
  Screen(bool modal, QString title, QString url, bool homeBtn = true); ///< Fully-specified screen.

  // Open this screen as a modal, taking up the entire window
  bool modal = false; ///< @see modal property.

  // Name of this screen
  QString title = ""; ///< @see title property.

  // URL of the screen to the QML file
  QString url = ""; ///< @see url property.

  bool homeBtn = true; ///< @see homeBtn property.
};

/**
 * @brief Screen navigation for the UI -- the QML StackView's controller.
 *
 * Holds the registry of named @ref screens and the current navigation @ref stack.
 * QML drives navigation through changeScreen()/closeScreen(); the Router emits
 * signals (goHome, openModal, closeModal, ...) that the QML shell acts on. Exposed
 * to QML as `brg.router`. loadScreens() registers the app's screen set at boot.
 *
 * @see Screen, Bridge.
 */
// Router for screens
class Router : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)                    ///< Current screen title.
  Q_PROPERTY(bool homeBtnShown MEMBER homeBtnShown NOTIFY homeBtnShownChanged)  ///< Whether the home button is shown now.

signals:
  void goHome();                 ///< Request: return to the home screen.
  void openModal(QString url);   ///< Request: open @p url as a modal.
  void openNonModal(QString url); ///< Request: open @p url as a normal screen.
  void closeModal();             ///< Request: close the current modal.
  void closeNonModal();          ///< Request: close the current non-modal.

  void titleChanged();
  void homeBtnShownChanged();

public:
  Q_INVOKABLE void changeScreen(QString name); ///< Navigate to the registered screen @p name.
  Q_INVOKABLE void closeScreen();              ///< Close the top screen.

  // For internal use only by StackView
  // Silently adds a screen onto the stack
  Q_INVOKABLE void manualStackPush(QString name); ///< StackView-internal: push @p name without side effects.

  QString title = "";       ///< @see title property.
  bool homeBtnShown = true; ///< @see homeBtnShown property.

  static void loadScreens(); ///< Register the app's screen set (called at boot).

  static QVector<Screen*> stack;          ///< The live navigation stack.
  static QHash<QString, Screen*> screens; ///< The registry of named screens.
};
