#include "mainwindow.h"

#include "../../src/common/types.h"
#include "../../src/data/file/filemanagement.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);

  MainWindow::instance = this;
  file = new FileManagement();

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

  // Link up incomming file signals
  connect(file, &FileManagement::pathChanged, this, &MainWindow::onPathChanged);
  connect(file, &FileManagement::recentFilesChanged, this, &MainWindow::reUpdateRecentFiles);

  // Initial setup
  reUpdateRecentFiles(file->getRecentFiles());
  onPathChanged(file->getPath());
  loadState();
}

MainWindow::~MainWindow()
{
  delete file;

  for(var8 i = 0; i < MAX_RECENT_FILES; i++)
    delete recentFileShortcuts[i];

  for(auto tmp : otherShortcuts)
    delete tmp;
}

MainWindow* MainWindow::instance{nullptr};

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
