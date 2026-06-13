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

/**
 * @file router.cpp
 * @brief Implementation of Router/Screen -- screen registry + navigation.
 *        See router.h for the documented API.
 */

#include "./router.h"

#include <QCoreApplication>

Screen::Screen() {}
Screen::Screen(bool modal, QString title, QString url, bool homeBtn)
  : modal(modal),
    title(title),
    url(url),
    homeBtn(homeBtn)
{}

void Router::changeScreen(QString name)
{
  // Get screen, if invalid load invalid screen
  auto scrn = screens.value(name, nullptr);
  if(scrn == nullptr) {
    scrn = screens.value("");
    name = "";
  }

  // Only set title if non-modal screen
  // Otherwise this causes a momentary flicker on the title where it changes
  // to the modal title briefly before displaying modal which looks very odd
  if(!scrn->modal) {
    title = QCoreApplication::translate("Screen", scrn->title.toUtf8().constData());
    titleChanged();
  }

  // If invalid screen, stop here
  if(name == "")
    return;

  // If Home Screen, issue a special signal notifying to pop all elements off
  else if(name == "home")
    goHome();

  // Otherwise notify to open a modal or non-modal screen
  else if(scrn->modal)
    openModal(scrn->url);
  else
    openNonModal(scrn->url);

  // If the home screen it means clear everything first because that's what the
  // UI did
  if(name == "home")
    stack.clear();

  homeBtnShown = scrn->homeBtn;

  // Append new screen
  stack.append(scrn);
  homeBtnShownChanged();
}

void Router::closeScreen()
{
  // Do nothin if we're at root screen or no screen at all
  if(stack.size() <= 1)
    return;

  // Get current screen
  Screen* scrn = stack.last();

  // If an error happened and we're unable to retrive screen data also return
  if(scrn == nullptr)
    return;

  // Figure out if it's modal or not and issue close statement accordingly
  if(scrn->modal)
    closeModal();
  else
    closeNonModal();

  // Pop screen and get new screen
  stack.pop_back();
  scrn = stack.last();

  // Only set title if non-modal screen
  // Otherwise this causes a momentary flicker on the title where it changes
  // to the modal title briefly before displaying modal which looks very odd
  if(!scrn->modal) {
    title = QCoreApplication::translate("Screen", scrn->title.toUtf8().constData());
    titleChanged();
  }

  homeBtnShown = scrn->homeBtn;

  homeBtnShownChanged();
}

void Router::manualStackPush(QString name)
{
  // Get screen, if invalid load invalid screen
  auto scrn = screens.value(name, nullptr);
  if(scrn == nullptr) {
    scrn = screens.value("");
    name = "";
  }

  // If invalid screen, stop here
  if(name == "")
    return;

  if(!scrn->modal) {
    title = QCoreApplication::translate("Screen", scrn->title.toUtf8().constData());
    titleChanged();
  }

  homeBtnShown = scrn->homeBtn;

  // Append new screen
  stack.append(scrn);
  homeBtnShownChanged();
}

void Router::loadScreens()
{
  // Empty
  screens.insert("", new Screen);

  // Modal. Titles are marked with QT_TRANSLATE_NOOP so lupdate extracts them
  // (context "Screen"); they're stored as the en_US source here and translated
  // at point of use (see changeScreen) — loadScreens runs at boot before the
  // translator is installed, so they must not be translated eagerly.
  screens.insert("newFile", new Screen(true, QT_TRANSLATE_NOOP("Screen", "New File"), "qrc:/ui/app/screens/modal/NewFile.qml"));
  screens.insert("fileTools", new Screen(true, QT_TRANSLATE_NOOP("Screen", "File Tools"), "qrc:/ui/app/screens/modal/FileTools.qml"));
  screens.insert("about", new Screen(true, QT_TRANSLATE_NOOP("Screen", "About"), "qrc:/ui/app/screens/modal/About.qml"));
  screens.insert("fullKeyboard", new Screen(true, QT_TRANSLATE_NOOP("Screen", "Full Keyboard"), "qrc:/ui/app/screens/modal/FullKeyboard.qml"));
  screens.insert("fileError", new Screen(true, QT_TRANSLATE_NOOP("Screen", "File Error"), "qrc:/ui/app/screens/modal/FileError.qml"));

  // Non-Modal
  screens.insert("home", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Home"), "qrc:/ui/app/screens/non-modal/Home.qml"));
  screens.insert("trainerCard", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Trainer Card"), "qrc:/ui/app/screens/non-modal/TrainerCard.qml"));
  screens.insert("pokedex", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Pokedex"), "qrc:/ui/app/screens/non-modal/Pokedex.qml"));
  screens.insert("bag", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Items"), "qrc:/ui/app/screens/non-modal/Bag.qml"));
  screens.insert("pokemart", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Pokemart"), "qrc:/ui/app/screens/non-modal/Pokemart.qml"));
  screens.insert("pokemon", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Pokemon"), "qrc:/ui/app/screens/non-modal/Pokemon.qml"));
  screens.insert("pokemonDetails", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Pokemon Details"), "qrc:/ui/app/screens/non-modal/PokemonDetails.qml", false));
  screens.insert("rival", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Rival"), "qrc:/ui/app/screens/non-modal/Rival.qml"));
  screens.insert("maps", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Maps"), "qrc:/ui/app/screens/non-modal/Maps.qml"));
  screens.insert("mapDetails", new Screen(false, QT_TRANSLATE_NOOP("Screen", "Map Details"), "qrc:/ui/app/screens/non-modal/MapDetails.qml", false));
}

QHash<QString, Screen*> Router::screens = QHash<QString, Screen*>();
QVector<Screen*> Router::stack = QVector<Screen*>();
