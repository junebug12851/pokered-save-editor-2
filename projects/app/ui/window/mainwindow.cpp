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

#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QScreen>
#include <QElapsedTimer>
#include <QDebug>
#include <QMessageBox>
#include "mainwindow.h"

#include "../../src/bridge/bridge.h"

#include <pse-common/types.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/pokemonstorageset.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>

#include <pse-db/db.h>
#include <pse-db/fontsdb.h>
#include <pse-db/entries/fontdbentry.h>

#include "../../src/engine/tilesetprovider.h"
#include "../../src/engine/fontpreviewprovider.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  QElapsedTimer t;
  t.start();

  // First setup UI
  ui.setupUi(this);
  qDebug() << "[MainWindow] setupUi —" << t.elapsed() << "ms";

  // Save global class instance
  MainWindow::instance = this;

  // Create the file management class which kickstarts all the data classes and
  // data management, etc... Basically a whole thing here lol
  file = new FileManagement();
  qDebug() << "[MainWindow] FileManagement —" << t.elapsed() << "ms";

  // Inject several C++ class instances into QML
  injectIntoQML();
  qDebug() << "[MainWindow] injectIntoQML —" << t.elapsed() << "ms";

  // Setup providers to QML
  setupProviders();
  qDebug() << "[MainWindow] setupProviders —" << t.elapsed() << "ms";

  // Report QML load errors. In debug builds this shows a message box so
  // problems are immediately visible during development; release builds stay
  // silent (the app continues running as gracefully as it can).
  connect(ui.app, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status) {
    if (status == QQuickWidget::Error) {
      QString msg;
      for (const auto& err : ui.app->errors())
        msg += err.toString() + "\n";
      qCritical() << "[QML]" << msg;
#ifdef QT_DEBUG
      QMessageBox::critical(this, "QML Error", msg);
#endif
    }
  });

  // Now load the QML page, has to be done after setup and injection
  ui.app->setSource(QUrl(QStringLiteral("qrc:/ui/app/App.qml")));
  qDebug() << "[MainWindow] setSource —" << t.elapsed() << "ms";

  // Setup global shortcuts
  setupShortcuts();

  // Link up Signal & Slots
  ssConnect();

  // Initial setup
  reUpdateRecentFiles(file->getRecentFiles());
  onPathChanged(file->getPath());
  loadState();
  qDebug() << "[MainWindow] constructor done —" << t.elapsed() << "ms";
}

MainWindow::~MainWindow()
{
  file->deleteLater();

  for(var8 i = 0; i < MAX_RECENT_FILES; i++)
    recentFileShortcuts[i]->deleteLater();

  for(auto tmp : otherShortcuts)
    tmp->deleteLater();
}

MainWindow* MainWindow::instance{nullptr};
Bridge* MainWindow::bridge = nullptr;
QQmlEngine* MainWindow::engine = nullptr;

MainWindow *MainWindow::getInstance()
{
  return MainWindow::instance;
}

void MainWindow::reUpdateRecentFiles(QList<QString> files)
{
  // Disable all recent files shortcuts
  for(var8 i{0}; i < MAX_RECENT_FILES; i++) {
    recentFileShortcuts[i]->setEnabled(false);
  }

  // Re-enable them based on how many recent files
  for(var8 i{0}; i < MAX_RECENT_FILES && i < files.size(); i++) {
    QString file{files.at(i)};
    if(file == "")
      continue;

    recentFileShortcuts[i]->setEnabled(true);
  }
}

void MainWindow::onRecentFileClick()
{
  QShortcut* shortcut{qobject_cast<QShortcut*>(sender())};
  var8 index{static_cast<var8>(shortcut->property("index").toInt())};
  file->openFileRecent(index);
}

void MainWindow::onPathChanged(QString path)
{
  if(path == "")
    this->setWindowTitle("Pokered Save Editor - New File");
  else
    this->setWindowTitle("Pokered Save Editor - " + path);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  this->saveState();
  event->accept();
}

void MainWindow::saveState()
{
  settings.beginGroup("WindowState");
  settings.setValue("size", this->size());
  settings.setValue("pos", this->pos());
  settings.endGroup();
}

void MainWindow::loadState()
{
  settings.beginGroup("WindowState");
  QSize  savedSize = settings.value("size", QSize(640, 480)).toSize();
  QPoint savedPos  = settings.value("pos",  QPoint(200, 200)).toPoint();
  settings.endGroup();

  this->resize(savedSize);

  // Guard against off-screen positions (e.g. disconnected monitor).
  // Accept the saved position only if the title bar area is on some screen.
  bool onScreen = false;
  const QPoint titleBarPt = savedPos + QPoint(savedSize.width() / 2, 10);
  for (const QScreen* screen : QGuiApplication::screens()) {
    if (screen->availableGeometry().contains(titleBarPt)) {
      onScreen = true;
      break;
    }
  }
  this->move(onScreen ? savedPos : QPoint(200, 200));
}

void MainWindow::setupShortcuts()
{
  // Create recent files shortcut
  for(var8 i = 0; i < MAX_RECENT_FILES; i++) {

    // Ctrl + Shift + #
    // Opens recent file 0 to Max
    // Key_0 is 0x30 - add i to 0x30 to get shortcut offset
    var8 shortcutKey{static_cast<var8>(0x30 + i)};

    // Create and link up a shortcut
    // Ensure it's disabled, assign a shortcut, and assign the index to it
    recentFileShortcuts[i] = new QShortcut(this);
    recentFileShortcuts[i]->setEnabled(false);
    recentFileShortcuts[i]->setKey(QKeySequence(Qt::CTRL | Qt::SHIFT | shortcutKey));
    recentFileShortcuts[i]->setProperty("index", i);
    connect(recentFileShortcuts[i], &QShortcut::activated, this, &MainWindow::onRecentFileClick);
  }

  // Create and link up other shortcuts
  auto os = otherShortcuts;

  os.insert("new", new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this));
  os.insert("open", new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_O), this));
  os.insert("reopen", new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this));
  os.insert("save", new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this));
  os.insert("saveas", new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this));
  os.insert("savecopyas", new QShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S), this));
  os.insert("scrub", new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), this));
  os.insert("clear-recentfiles", new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus), this));
  os.insert("exit", new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this));
  os.insert("exit2", new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F4), this));
  os.insert("random", new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R), this));

  connect(os.value("new"), &QShortcut::activated, file, &FileManagement::newFile);
  connect(os.value("open"), &QShortcut::activated, file, &FileManagement::openFile);
  connect(os.value("reopen"), &QShortcut::activated, file, &FileManagement::reopenFile);
  connect(os.value("clear-recentfiles"), &QShortcut::activated, file, &FileManagement::clearRecentFiles);
  connect(os.value("save"), &QShortcut::activated, file, &FileManagement::saveFile);
  connect(os.value("saveas"), &QShortcut::activated, file, &FileManagement::saveFileAs);
  connect(os.value("savecopyas"), &QShortcut::activated, file, &FileManagement::saveFileCopy);
  connect(os.value("scrub"), &QShortcut::activated, file, &FileManagement::wipeUnusedSpace);
  connect(os.value("exit"), &QShortcut::activated, this, &MainWindow::close);
  connect(os.value("exit2"), &QShortcut::activated, this, &MainWindow::close);
  connect(os.value("random"), &QShortcut::activated, file->data, &SaveFile::randomizeExpansion);
}

void MainWindow::setupProviders()
{
  auto engine = ui.app->engine();
  engine->addImageProvider("tileset", new TilesetProvider);
  engine->addImageProvider("font", new FontPreviewProvider(file->data->dataExpanded));
}

void MainWindow::injectIntoQML()
{
  auto* context = ui.app->rootContext();
  bridge = new Bridge(file);
  context->setContextProperty("brg", bridge);
  MainWindow::engine = ui.app->engine();

  // CRITICAL: hand the QML engine CppOwnership of every DB entry (FontDBEntry,
  // move/species/item entries, etc.). These C++ objects live in DB QVectors with
  // no QObject parent, so Q