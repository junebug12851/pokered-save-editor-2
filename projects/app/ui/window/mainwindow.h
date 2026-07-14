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
#include <QString>
#include <QHash>
#include <QShortcut>
#include <QSettings>
#include <QQmlEngine>

#include "ui_mainwindow.h"

// The Q_PROPERTY(FileManagement* file) below makes MOC require the COMPLETE type
// (not a forward decl), and FileManagement is traversed in QML via brg.file so it
// must NOT be Q_DECLARE_OPAQUE_POINTER'd. This was previously satisfied only as a
// side effect of the single-target unity MOC; now that the app logic is a library
// (appcore), include it explicitly. See notes/reference/qt6-patterns.md.
#include <pse-savefile/filemanagement.h>

class RecentFilesModel;
class QAbstractItemModel;
class Bridge;
class QFileSystemWatcher;

/**
 * @brief The top-level window -- a QMainWindow hosting the QML UI in a QQuickWidget.
 *
 * Created last in the boot sequence (`createApp()`), it is where C++ meets QML at
 * runtime: it constructs the @ref bridge, registers the image providers, and
 * injects `brg` into the QML engine (injectIntoQML()). It also owns the live
 * @ref file (FileManagement), wires global keyboard @ref otherShortcuts and the
 * recent-file shortcuts, and persists window state across runs.
 *
 * @note This is the one C++ class living under `app/ui` (with the QML) rather than
 *       `app/src`; it's included in the docs via a dedicated Doxyfile INPUT entry.
 * @see Bridge (the `brg` aggregate it creates), the boot sequence in
 *      [the system map](../../../../notes/systems/app.md).
 */
class MainWindow : public QMainWindow
{
  Q_OBJECT

  Q_PROPERTY(FileManagement* file MEMBER file NOTIFY fileChanged) ///< The live save controller.

public:
  MainWindow(QWidget* parent = nullptr);
  virtual ~MainWindow();

  static MainWindow* getInstance(); ///< The single MainWindow instance.
  static Bridge* bridge;            ///< The `brg` aggregate (created here, injected into QML).
  static QQmlEngine* engine;        ///< The QML engine behind the hosted QQuickWidget.

  /// DEBUG: render the live QML view to an image file (focus/occlusion-independent).
  /// Backs the --shot debug launch flag. @return false on failure.
  bool saveShot(const QString& path);

  /// DEBUG: a **real** mouse press+release at @p at (view coordinates), through Qt's real delivery
  /// path -- grabs, pointer handlers, propagation and all.
  ///
  /// ⚠️ NOT the same thing as the harness's `click`, which emits a control's `clicked()` signal and
  /// therefore exercises **none** of the delivery machinery. If a bug is about *which item gets the
  /// event* -- who grabs it, who consumes it, what it falls through to -- `click` cannot see it and
  /// this can. @see debugserver.cpp -> "tap"
  bool debugTap(const QPointF& at);

  /// DEBUG: open the details editor for party mon @p index (drives the QML
  /// AppWindow.debugOpenPartyDetails). Backs --screen pokemonDetails. @return false
  /// if the item/mon can't be resolved.
  bool debugOpenPartyDetails(int index);

  /// DEBUG: the root QML object of the hosted view (for the debug control server's
  /// object lookups). @return null before QML is loaded.
  QObject* qmlRootObject();

  /// DEBUG (--hot): clear the QML cache and reload the view from the source files on
  /// disk (live QML refresh). C++ state (the Bridge/save) survives; QML re-instantiates.
  /// Restores the screen you were on afterwards (full-tree reload otherwise dumps you
  /// at Home + the New File modal) -- see the implementation for why partial reload is
  /// deliberately not attempted.
  void reloadQml();

  FileManagement* file = nullptr;   ///< @see file property.

  // MAX_RECENT_FILES
  QShortcut* recentFileShortcuts[5];          ///< Ctrl+1..5 open-recent shortcuts.
  QHash<QString, QShortcut*> otherShortcuts;  ///< Other global keyboard shortcuts by name.

signals:
  void fileChanged(); ///< The live save was replaced.

private slots:
  void reUpdateRecentFiles(QList<QString> files); ///< Refresh recent-file shortcuts/menu.
  void onRecentFileClick();  ///< A recent-file shortcut/menu item was triggered.
  void onPathChanged(QString path); ///< The active save path changed (update title, etc.).

private:
  Ui::MainWindow ui;     ///< The generated .ui form (hosts the QQuickWidget).
  QSettings settings;    ///< Persistent window state.
  void closeEvent(QCloseEvent* event); ///< Save state on close.

  void saveState();  ///< Persist window geometry/state.
  void loadState();  ///< Restore window geometry/state.

  void setupShortcuts(); ///< Wire the global keyboard shortcuts.
  void setupProviders(); ///< Register the QML image providers (font/tileset).
  void injectIntoQML();  ///< Create the Bridge and expose it to QML as `brg`.
  void setupHotReload(); ///< DEBUG (--hot): install the disk URL interceptor + watch QML files.
  QFileSystemWatcher* m_qmlWatcher = nullptr; ///< DEBUG: watches the source QML tree.
  bool m_reloadPending = false;               ///< DEBUG: debounce flag for reloads.
  void ssConnect();      ///< Connect FileManagement signals to the window.

  static MainWindow* instance; ///< Backing singleton pointer.
};
