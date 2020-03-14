
#include <QQmlEngine>
#include <QQmlContext>
#include "mainwindow.h"

#include "../../src/bridge/bridge.h"

#include <pse-common/types.h>
#include "../../src/data/file/filemanagement.h"
#include "../../src/data/file/savefile.h"
#include "../../src/data/file/expanded/savefileexpanded.h"
#include "../../src/data/file/expanded/storage.h"
#include "../../src/data/file/expanded/fragments/pokemonstorageset.h"
#include "../../src/data/file/expanded/fragments/pokemonstoragebox.h"
#include "../../src/data/file/expanded/player/player.h"
#include "../../src/data/file/expanded/player/playerpokemon.h"

#include <pse-db/fonts.h>

#include "../../src/engine/tilesetprovider.h"
#include "../../src/engine/fontpreviewprovider.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  // First setup UI
  ui.setupUi(this);

  // Save global class instance
  MainWindow::instance = this;

  // Create the file management class which kickstarts all the data classes and
  // data management, etc... Basically a whole thing here lol
  file = new FileManagement();

  // Inject several C++ class instances into QML
  injectIntoQML();

  // Setup providers to QML
  setupProviders();

  // Now load the QML page, has to be done after setup and injection
  ui.app->setSource(QUrl(QStringLiteral("qrc:/ui/app/App.qml")));

  // Setup global shortcuts
  setupShortcuts();

  // Link up Signal & Slots
  ssConnect();

  // Initial setup
  reUpdateRecentFiles(file->getRecentFiles());
  onPathChanged(file->getPath());
  loadState();
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
  this->resize(settings.value("size", QSize(640,480)).toSize());
  this->move(settings.value("pos", QPoint(200,200)).toPoint());
  settings.endGroup();
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
    recentFileShortcuts[i]->setKey(QKeySequence(Qt::CTRL + Qt::SHIFT + shortcutKey));
    recentFileShortcuts[i]->setProperty("index", i);
    connect(recentFileShortcuts[i], &QShortcut::activated, this, &MainWindow::onRecentFileClick);
  }

  // Create and link up other shortcuts
  auto os = otherShortcuts;

  os.insert("new", new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), this));
  os.insert("open", new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O), this));
  os.insert("reopen", new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O), this));
  os.insert("save", new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this));
  os.insert("saveas", new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S), this));
  os.insert("savecopyas", new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S), this));
  os.insert("scrub", new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W), this));
  os.insert("clear-recentfiles", new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Minus), this));
  os.insert("exit", new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this));
  os.insert("exit2", new QShortcut(QKeySequence(Qt::ALT + Qt::Key_F4), this));
  os.insert("random", new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R), this));

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
  // Now grab the QML instance from UI
  auto engine = ui.app->engine();
  this->engine = engine;

  auto qml = engine->rootContext();

  // Inject singleton instances needed and save as static property
  auto bridge = new Bridge(file);
  this->bridge = bridge;
  qml->setContextProperty("brg", bridge);

  // Mark pointers MINE, NOT QML

  // fontSearch returns a pointer to itself for chain linking, this causes
  // issues as QML sees a pointer retruned from a Q_INVOKABLE and thinks it
  // owns the pointer and thus needs to GC/Auto-Destroy it
  engine->setObjectOwnership(bridge->fontSearch, QQmlEngine::CppOwnership);

  // FontsDB allows QML to retrieve font information via a Q_INVOKABLE, like
  // above the pointer returned can cause QML to take ownership and start
  // deleting font data. Mark all font information is Mine, Not QML
  for(auto font : FontsDB::store)
    engine->setObjectOwnership(font, QQmlEngine::CppOwnership);

  // Force QML to see all Pokemon boxes including party box as strictly owned by
  // CPP
  for(auto elSet : bridge->file->data->dataExpanded->storage->pokemon) {
    engine->setObjectOwnership(elSet, QQmlEngine::CppOwnership);

    for(auto elBox : elSet->boxes) {
      engine->setObjectOwnership(elBox, QQmlEngine::CppOwnership);
    }
  }

  engine->setObjectOwnership(bridge->file->data->dataExpanded->player->pokemon, QQmlEngine::CppOwnership);
}

void MainWindow::ssConnect()
{
  // Link up incomming file signals
  connect(file, &FileManagement::pathChanged, this, &MainWindow::onPathChanged);
  connect(file, &FileManagement::recentFilesChanged, this, &MainWindow::reUpdateRecentFiles);
}
