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
 * @file mainwindow.cpp
 * @brief Implementation of MainWindow -- builds the Bridge, registers QML image
 *        providers, injects `brg`, wires shortcuts, and persists window state.
 *        See mainwindow.h for the documented API.
 */
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QScreen>
#include <QElapsedTimer>
#include <QDebug>
#include <QMessageBox>
#include <QImage>
#include <QVariant>
#include <QQuickItem>
#include <QApplication>

#ifdef QT_DEBUG
#include <QQmlAbstractUrlInterceptor>
#include <QFileSystemWatcher>
#include <QDirIterator>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QUrl>
namespace {
// Redirects qrc:/...qml|js URLs to the matching file on the source tree so the running
// app loads QML from disk (enabling live reload). Falls back to qrc when the source
// file isn't present. DEBUG-only.
class QmlDiskInterceptor : public QQmlAbstractUrlInterceptor {
public:
  explicit QmlDiskInterceptor(const QString& root) : m_root(root) {}
  QUrl intercept(const QUrl& url, DataType) override {
    if(url.scheme() != QLatin1String("qrc")) return url;
    const QString path = url.path(); // e.g. "/ui/app/App.qml"
    if(!path.endsWith(QLatin1String(".qml")) && !path.endsWith(QLatin1String(".js")))
      return url;
    const QString disk = m_root + path;
    if(QFileInfo::exists(disk)) return QUrl::fromLocalFile(disk);
    return url;
  }
private:
  QString m_root;
};
} // namespace
#endif
#include "mainwindow.h"

#include "../../src/bridge/bridge.h"
#include "../../src/bridge/router.h"
#include "../../src/boot/shortcutdefs.h"

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

#ifdef QT_DEBUG
  // DEBUG (--hot): load QML from the source tree + watch it, so edits reload live.
  // Must be installed BEFORE setSource so App.qml itself loads from disk.
  if(QApplication::arguments().contains(QStringLiteral("--hot")))
    setupHotReload();
#endif

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

// DEBUG helper: render the live QML view (the QQuickWidget's framebuffer) to an image
// file. Works regardless of window focus/occlusion, so an automation harness can grab
// the current screen without raising or activating the window. See --shot in
// src/boot/debuglaunch.cpp.
bool MainWindow::saveShot(const QString& path)
{
  const QImage img = ui.app->grabFramebuffer();
  if(img.isNull())
    return false;
  return img.save(path);
}

bool MainWindow::debugOpenPartyDetails(int index)
{
  QObject* root = ui.app->rootObject();
  if(root == nullptr)
    return false;
  QObject* appWindow = root->findChild<QObject*>(QStringLiteral("appWindow"));
  if(appWindow == nullptr)
    return false;
  QVariant ret;
  const bool ok = QMetaObject::invokeMethod(
      appWindow, "debugOpenPartyDetails",
      Q_RETURN_ARG(QVariant, ret), Q_ARG(QVariant, index));
  return ok && ret.toBool();
}

QObject* MainWindow::qmlRootObject()
{
  return ui.app->rootObject();
}

void MainWindow::setupHotReload()
{
#ifdef QT_DEBUG
#ifdef PSE_QML_SOURCE_DIR
  const QString root = QStringLiteral(PSE_QML_SOURCE_DIR);
#else
  const QString root;
#endif
  if(root.isEmpty() || !QDir(root + QStringLiteral("/ui")).exists()) {
    qWarning() << "[hot-reload] QML source dir not found:" << root << "-- staying on qrc.";
    return;
  }
  ui.app->engine()->addUrlInterceptor(new QmlDiskInterceptor(root));

  m_qmlWatcher = new QFileSystemWatcher(this);
  QStringList files;
  QDirIterator it(root + QStringLiteral("/ui"), QStringList{ QStringLiteral("*.qml"), QStringLiteral("*.js") },
                  QDir::Files, QDirIterator::Subdirectories);
  while(it.hasNext()) files << it.next();
  if(!files.isEmpty()) m_qmlWatcher->addPaths(files);
  connect(m_qmlWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& p) {
    // Editors often rewrite/rename on save, which drops the watch -- re-arm it.
    if(!m_qmlWatcher->files().contains(p) && QFileInfo::exists(p))
      m_qmlWatcher->addPath(p);
    if(m_reloadPending) return;              // debounce a burst of change events
    m_reloadPending = true;
    QTimer::singleShot(160, this, [this]{ m_reloadPending = false; reloadQml(); });
  });
  qInfo() << "[hot-reload] watching" << files.size() << "QML files under" << (root + QStringLiteral("/ui"));
#endif
}

void MainWindow::reloadQml()
{
#ifdef QT_DEBUG
  // A hot-reload rebuilds the WHOLE QML tree from App.qml (there is no safe per-file
  // partial reload in a single-QQuickWidget app -- the engine caches whole compiled
  // components, so reloading only the changed file would leave consumers of a shared
  // component holding a stale cached copy, a subtle correctness hazard we refuse). To
  // keep the fast-iteration experience without that hazard, we do the full, reliable
  // reload but RESTORE CONTEXT afterwards: the loaded save already survives untouched
  // (it lives in C++ -- Bridge/FileManagement -- not QML), so we only need to return
  // the user to the screen they were editing instead of dumping them at Home + the
  // New File modal.

  // 1) Remember the current screen (by name) before we tear the tree down. The top of
  //    the nav stack is a Screen*; reverse-look-up its registered name.
  QString currentName;
  if(!Router::stack.isEmpty()) {
    Screen* top = Router::stack.last();
    for(auto it = Router::screens.cbegin(); it != Router::screens.cend(); ++it)
      if(it.value() == top) { currentName = it.key(); break; }
  }

  // 2) Full reload.
  if(engine) engine->clearComponentCache();
  const QUrl src = ui.app->source();
  ui.app->setSource(QUrl());  // tear down the current tree
  ui.app->setSource(src.isValid() ? src : QUrl(QStringLiteral("qrc:/ui/app/App.qml")));
  qInfo() << "[hot-reload] reloaded" << (src.isValid() ? src.toString() : QStringLiteral("App.qml"))
          << "-- restoring screen:" << (currentName.isEmpty() ? QStringLiteral("(none)") : currentName);

  // 3) Only restore a real, non-modal screen. Home needs no action (App.qml lands
  //    there), and we don't auto-reopen a modal (the startup New File modal is
  //    legitimately shown when no save/target is active).
  const bool restore = !currentName.isEmpty()
                    && currentName != QStringLiteral("home")
                    && Router::screens.contains(currentName)
                    && Router::screens.value(currentName) != nullptr
                    && !Router::screens.value(currentName)->modal;
  if(!restore)
    return;

  // 4) App.qml re-seats its startup stack (home + New File modal) on a later event-
  //    loop tick, so poll until it's seated, then dismiss the modal and navigate back
  //    (same sequencing as the --screen debug-launch flag). pokemonDetails needs a
  //    selection, so re-open party mon 0 rather than a bare changeScreen().
  QTimer* timer = new QTimer(this);
  int* tries = new int(0);
  const QString target = currentName;
  connect(timer, &QTimer::timeout, this, [this, timer, tries, target]() {
    Bridge* brg = MainWindow::bridge;
    const bool ready = (brg != nullptr) && Router::stack.size() >= 2;
    if(!ready && ++(*tries) < 200) // wait up to ~10s (50ms x 200)
      return;
    timer->stop();
    timer->deleteLater();
    delete tries;
    if(brg == nullptr)
      return;
    brg->router->closeScreen();               // dismiss the startup New File modal
    if(target == QStringLiteral("pokemonDetails"))
      debugOpenPartyDetails(0);               // needs a selected mon
    else
      brg->router->changeScreen(target);      // back to where we were
    qInfo() << "[hot-reload] restored screen" << target;
  });
  timer->setInterval(50);
  timer->start();
#endif
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
  QSize  savedSize = settings.value("size", QSize(1130, 740)).toSize();
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
  // Create recent files shortcut (Ctrl+Shift+0..4 -> open recent file 0..Max).
  for(var8 i = 0; i < MAX_RECENT_FILES; i++) {

    // Create and link up a shortcut
    // Ensure it's disabled, assign a shortcut, and assign the index to it
    recentFileShortcuts[i] = new QShortcut(this);
    recentFileShortcuts[i]->setEnabled(false);
    recentFileShortcuts[i]->setKey(pse::recentFileShortcutKey(i));
    recentFileShortcuts[i]->setProperty("index", i);
    connect(recentFileShortcuts[i], &QShortcut::activated, this, &MainWindow::onRecentFileClick);
  }

  // Create and link up other shortcuts. Key sequences come from the shared
  // pse::shortcutKeyMap() (single source of truth, asserted by tst_shortcuts);
  // the action→verb wiring stays here. NB: write into the `otherShortcuts` MEMBER
  // directly -- the previous `auto os = otherShortcuts;` copied it, so the member
  // was left empty (the shortcuts only survived via their QObject parent).
  auto& os = otherShortcuts;
  const auto keymap = pse::shortcutKeyMap();
  for (auto it = keymap.constBegin(); it != keymap.constEnd(); ++it)
    os.insert(it.key(), new QShortcut(it.value(), this));

  // Wire each shortcut to its verb via the shared pse::shortcutActions() map (the
  // single source of truth tst_shortcuts fires against). exit/exit2 close the window.
  const auto actions = pse::shortcutActions(file, [this]{ close(); });
  for (auto it = actions.constBegin(); it != actions.constEnd(); ++it) {
    QShortcut* sc = os.value(it.key(), nullptr);
    if (!sc) continue;
    const std::function<void()> verb = it.value();
    connect(sc, &QShortcut::activated, this, [verb]{ verb(); });
  }
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

  // Protect every DB entry from QML's garbage collector (s13f). DB::qmlProtect
  // cascades CppOwnership to all sub-DB entries; without it QML GCs the shared,
  // parentless FontDBEntry (and other) objects mid-session — fonts blank out,
  // picker pills go red/empty, and names stop saving until an app reboot.
  DB::inst()->qmlProtect(engine);
}

void MainWindow::ssConnect()
{
  connect(file, &FileManagement::pathChanged, this, &MainWindow::onPathChanged);
  connect(file, &FileManagement::recentFilesChanged, this, &MainWindow::reUpdateRecentFiles);
}

