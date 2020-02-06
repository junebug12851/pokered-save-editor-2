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

#include "./router.h"

Screen::Screen() {}
Screen::Screen(bool modal, QString title, QString url)
  : modal(modal),
    title(title),
    url(url)
{}

void Router::changeScreen(QString name)
{
  // Get screen, if invalid load invalid screen
  auto scrn = screens.value(name, nullptr);
  if(scrn == nullptr) {
    scrn = screens.value("");
    name = "";
  }

  // Adjust curScrn and title
  curScrn = name;
  curScrnChanged();

  title = scrn->title;
  titleChanged();

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
}

void Router::loadScreens()
{
  // Empty
  screens.insert("", new Screen);

  // Modal
  screens.insert("newFile", new Screen(true, "New File", "qrc:/ui/app/screens/modal/NewFile.qml"));
  screens.insert("fileTools", new Screen(true, "File Tools", "qrc:/ui/app/screens/modal/FileTools.qml"));
  screens.insert("about", new Screen(true, "About", "qrc:/ui/app/screens/modal/About.qml"));

  // Non-Modal
  screens.insert("home", new Screen(false, "Home", "qrc:/ui/app/screens/non-modal/Home.qml"));
  screens.insert("trainerCard", new Screen(false, "Trainer Card", "qrc:/ui/app/screens/non-modal/TrainerCard.qml"));
}

QHash<QString, Screen*> Router::screens = QHash<QString, Screen*>();
