/*
  * Copyright 2026 Twilight
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

/**
 * @file guiapp.h
 * @brief Header-only harness that boots the REAL application UI headless, exactly
 *        the way MainWindow does, so GUI tests drive the genuine component tree.
 *
 * It loads `qrc:/ui/app/App.qml` (the StackView shell) into a QQuickView shown on
 * the `offscreen` platform, with the same context the app provides:
 *   - `brg` context property (a real Bridge over a FileManagement),
 *   - the "tileset" and "font" image providers,
 *   - DB::inst()->qmlProtect(engine) (GC guard),
 *   - bootQmlLinkage() (the exe's full qRegisterMetaType + qmlRegister* set).
 * Because the real App.qml + AppWindow.qml load, the real C++ Router drives the
 * real StackViews (appRoot for modals, appBody for non-modal pages) -- navigation
 * exercises the true push/pop/instantiation path.
 *
 * The harness also captures QML warnings/errors (any qWarning/qCritical during a
 * scoped block is a failure), finds items in the live tree by objectName or type,
 * and synthesizes real mouse/keyboard input -- so tests can be as "GUI-true" as a
 * user click or as robust as a direct navigation, per the hybrid testing approach
 * (notes/plans/testing.md -> "Broader GUI coverage").
 *
 * Engine/provider wiring MUST stay in sync with MainWindow::injectIntoQML +
 * setupProviders. If that changes, change it here too.
 *
 * Requires: link appcore + Qt6::Quick/Qml/Test, compile in the app's app.qrc and
 * src/boot/bootQmlLinkage.cpp, and run with QT_QPA_PLATFORM=offscreen (all set in
 * tests/CMakeLists.txt). PSE_ASSETS_DIR must be defined (for the fixtures).
 */

#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QMutex>
#include <QPointF>
#include <QElapsedTimer>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickItem>
#include <QQuickWindow>
#include <functional>

#include "savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <engine/tilesetprovider.h>
#include <engine/fontpreviewprovider.h>

// Defined in the app exe (src/boot/bootQmlLinkage.cpp), compiled into each GUI
// test target so QML type registration stays a single shared source of truth.
extern void bootQmlLinkage();

namespace pse_test {

// ---------------------------------------------------------------------------
// QML message capture. While a capture sink is active, every qWarning/qCritical/
// qFatal is appended to it; a non-empty sink means the UI emitted a warning/error
// (binding TypeError, missing type/provider, anchor-on-null, FINAL override, ...).
// `inline` so multiple GUI test TUs can include this header without ODR clashes.
// ---------------------------------------------------------------------------
namespace gui_detail {

inline QMutex          g_mutex;
inline QStringList*    g_sink = nullptr;
inline QtMessageHandler g_prev = nullptr;

inline bool isBenign(const QString& msg)
{
  // Known-benign NON-QML platform/test noise. Keep EMPTY by default; only add a
  // narrow, justified substring (with a comment) -- never to hide a real warning.
  static const char* const kAllow[] = {
    // Qt 6 no longer ships fonts; the offscreen platform has no system font dir, so
    // QFontDatabase logs this once at first text render. Not a QML/app bug -- purely
    // a headless-CI environment artifact (text still lays out via the fallback).
    "Cannot find font directory",
  };
  for (const char* s : kAllow)
    if (msg.contains(QLatin1String(s)))
      return true;
  return false;
}

inline void handler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
  if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
    QMutexLocker lock(&g_mutex);
    if (g_sink != nullptr && !isBenign(msg)) {
      const QString cat = QString::fromLatin1(ctx.category ? ctx.category : "default");
      g_sink->append(cat + QStringLiteral(": ") + msg);
    }
  }
  if (g_prev != nullptr && type != QtFatalMsg)
    g_prev(type, ctx, msg);
}

} // namespace gui_detail

/// RAII scope: capture QML warnings emitted while it is alive into `messages()`.
class QmlWarningScope
{
public:
  QmlWarningScope()  { QMutexLocker l(&gui_detail::g_mutex); gui_detail::g_sink = &m_msgs; }
  ~QmlWarningScope() { QMutexLocker l(&gui_detail::g_mutex); gui_detail::g_sink = nullptr; }
  const QStringList& messages() const { return m_msgs; }
  bool clean() const { return m_msgs.isEmpty(); }
private:
  QStringList m_msgs;
};

// ---------------------------------------------------------------------------
// The headless app.
// ---------------------------------------------------------------------------
class GuiApp
{
public:
  /// @param fixture  a fixture filename (e.g. "BaseSAV.sav") to open, or "" / "new"
  ///                 to start from a blank New File.
  explicit GuiApp(const QString& fixture = QStringLiteral("BaseSAV.sav"))
  {
    bootOnce();

    m_file = new FileManagement;
    if (fixture.isEmpty() || fixture == QStringLiteral("new")) {
      m_file->newFile();
    } else {
      const QByteArray bytes = readSaveBytes(fixture);
      loadInto(*m_file->data, bytes);   // caller asserts fixture present if needed
    }

    m_brg = new Bridge(m_file);

    m_view = new QQuickView;
    m_view->engine()->rootContext()->setContextProperty(QStringLiteral("brg"), m_brg);
    m_view->engine()->addImageProvider(QStringLiteral("tileset"), new TilesetProvider);
    m_view->engine()->addImageProvider(QStringLiteral("font"),
                                       new FontPreviewProvider(m_file->data->dataExpanded));
    DB::inst()->qmlProtect(m_view->engine());

    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    m_view->resize(1024, 768);
  }

  ~GuiApp()
  {
    delete m_view;          // tears down the whole QML tree
    delete m_brg;
    delete m_file;
  }

  /// Load App.qml (the shell) and let it seat its initial stack (home + newFile +
  /// the New File modal). @return true if it loaded without component errors.
  bool start()
  {
    m_view->setSource(QUrl(QStringLiteral("qrc:/ui/app/App.qml")));
    settle();
    return m_view->status() == QQuickView::Ready && m_view->rootObject() != nullptr;
  }

  Bridge*         bridge() const { return m_brg; }
  FileManagement* file()   const { return m_file; }
  QQuickView*     view()   const { return m_view; }
  QObject*        router() const { return qvariant_cast<QObject*>(m_brg->property("router")); }

  /// The router's current screen title (C++-owned; reliable navigation oracle).
  QString routerTitle() const { return router() ? router()->property("title").toString() : QString(); }

  /// Depth of the C++ Router's navigation stack.
  int routerStackDepth() const { return Router::stack.size(); }

  /// Navigate to a registered screen via the REAL Router (brg.router.changeScreen),
  /// the same call the QML buttons make. Pumps the event loop until both StackView
  /// transitions settle so the push/pop + instantiation has actually completed --
  /// a fixed delay is unreliable for heavy screens (e.g. the Pokemon storage grid).
  void navigate(const QString& screen)
  {
    QMetaObject::invokeMethod(router(), "changeScreen", Q_ARG(QString, screen));
    waitForStacksIdle();
  }

  /// Close the top screen (brg.router.closeScreen) and pump until transitions settle.
  void closeTop()
  {
    QMetaObject::invokeMethod(router(), "closeScreen");
    waitForStacksIdle();
  }

  /// Pump the event loop so bindings, Component.onCompleted, and StackView
  /// transitions complete.
  void settle(int ms = 60) { QTest::qWait(ms); }

  /// Spin the event loop (in small slices) until @p done() is true or @p timeoutMs
  /// elapses. Lets tests wait on a real UI condition instead of guessing a delay.
  void settleUntil(const std::function<bool()>& done, int timeoutMs = 4000)
  {
    QElapsedTimer t; t.start();
    while (!done() && t.elapsed() < timeoutMs)
      QTest::qWait(15);
  }

  /// Both StackViews (outer appRoot for modals, inner appBody for pages) have no
  /// in-flight push/pop transition.
  bool stacksIdle() const
  {
    QObject* root = m_view->rootObject();
    QObject* ab   = appBody();
    const bool rootBusy = root && root->property("busy").toBool();
    const bool abBusy   = ab   && ab->property("busy").toBool();
    return !rootBusy && !abBusy;
  }

  /// Wait out any StackView transition, then a short settle for deferred bindings.
  void waitForStacksIdle()
  {
    settleUntil([this]{ return stacksIdle(); });
    settle(20);
  }

  /// The inner non-modal StackView (appBody in AppWindow.qml): the first nested
  /// StackView under the view's root (appRoot is the root itself, excluded).
  ///
  /// NB: a plain `StackView { id: appBody }` instance has the QML-generated metaobject
  /// class name "StackView_QMLTYPE_N" (NOT "QQuickStackView"), so we match the
  /// "StackView" substring -- which covers both the generated and the C++ class names.
  /// appRoot is App.qml's root (class "App_QMLTYPE_N", no "StackView" substring, and
  /// excluded from findChildren anyway), so the only match is appBody.
  QObject* appBody() const
  {
    QObject* root = m_view->rootObject();
    if (!root) return nullptr;
    const auto stacks = root->findChildren<QObject*>();
    for (QObject* o : stacks)
      if (QString::fromLatin1(o->metaObject()->className()).contains(QLatin1String("StackView")))
        return o;   // first nested stack == appBody (appRoot is the root, not a child)
    return nullptr;
  }

  /// The item currently on top of the non-modal stack, or nullptr.
  QQuickItem* currentNonModal() const
  {
    QObject* ab = appBody();
    return ab ? ab->property("currentItem").value<QQuickItem*>() : nullptr;
  }

  // --- item finding ---------------------------------------------------------

  /// Depth-first search of the live item tree for the first item matching @p pred.
  static QQuickItem* findItem(QQuickItem* root, const std::function<bool(QQuickItem*)>& pred)
  {
    if (!root) return nullptr;
    if (pred(root)) return root;
    const auto kids = root->childItems();
    for (QQuickItem* c : kids)
      if (QQuickItem* hit = findItem(c, pred))
        return hit;
    return nullptr;
  }

  /// Collect EVERY item under @p root matching @p pred (depth-first, includes root).
  static void collectItems(QQuickItem* root, const std::function<bool(QQuickItem*)>& pred,
                           QList<QQuickItem*>& out)
  {
    if (!root) return;
    if (pred(root)) out.append(root);
    const auto kids = root->childItems();
    for (QQuickItem* c : kids)
      collectItems(c, pred, out);
  }

  /// All items under @p from (default: current non-modal screen) whose metaobject
  /// class name contains @p typeSub (e.g. "ComboBox", "TextField").
  QList<QQuickItem*> itemsByType(const QString& typeSub, QQuickItem* from = nullptr) const
  {
    QQuickItem* root = from ? from : currentNonModal();
    QList<QQuickItem*> out;
    collectItems(root, [&](QQuickItem* i){
      return QString::fromLatin1(i->metaObject()->className()).contains(typeSub);
    }, out);
    return out;
  }

  /// First item whose objectName == @p name, anywhere under the view root.
  QQuickItem* itemByName(const QString& name) const
  {
    QQuickItem* root = qobject_cast<QQuickItem*>(m_view->rootObject());
    return findItem(root, [&](QQuickItem* i){ return i->objectName() == name; });
  }

  /// First item whose metaObject class name contains @p typeSub (e.g. "TextField",
  /// "Button"), under @p from (defaults to the current non-modal screen).
  QQuickItem* itemByType(const QString& typeSub, QQuickItem* from = nullptr) const
  {
    QQuickItem* root = from ? from : currentNonModal();
    return findItem(root, [&](QQuickItem* i){
      return QString::fromLatin1(i->metaObject()->className()).contains(typeSub);
    });
  }

  // --- real input synthesis -------------------------------------------------

  /// Synthesize a left click at the centre of @p item (real event delivery).
  void clickItem(QQuickItem* item)
  {
    if (!item) return;
    const QPointF c = item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0));
    QTest::mouseClick(m_view, Qt::LeftButton, Qt::NoModifier, c.toPoint());
    settle(20);
  }

  /// Give @p item active focus and type @p text as real key events.
  void typeInto(QQuickItem* item, const QString& text)
  {
    if (!item) return;
    item->forceActiveFocus();
    settle(10);
    keyType(text);
    settle(10);
  }

  /// Type @p text as real per-character key events into the view window.
  /// NB: QTest::keyClicks(const QString&) is a QWidget-only overload; QQuickView is
  /// a QWindow, so we loop the QWindow keyClick(char) overload instead.
  void keyType(const QString& text)
  {
    for (const QChar c : text)
      QTest::keyClick(m_view, c.toLatin1());
  }

  /// Press a single key (e.g. Qt::Key_Return to commit a field) on the window.
  void pressKey(Qt::Key key)
  {
    QTest::keyClick(m_view, key);
    settle(10);
  }

  // --- save / reopen --------------------------------------------------------

  /// Flatten + write the live save to @p path (the app's save path), then assert.
  bool saveTo(const QString& path)
  {
    m_file->setPath(path);
    return m_file->saveFile();
  }

  static void installMessageHandler()   { gui_detail::g_prev = qInstallMessageHandler(gui_detail::handler); }
  static void restoreMessageHandler()   { qInstallMessageHandler(gui_detail::g_prev); }

private:
  static void bootOnce()
  {
    static bool booted = false;
    if (booted) return;
    booted = true;
    (void)DB::inst();   // [[nodiscard]] -- booted for side effects (singleton init)
    Router::loadScreens();
    bootQmlLinkage();
  }

  FileManagement* m_file = nullptr;
  Bridge*         m_brg  = nullptr;
  QQuickView*     m_view = nullptr;
};

} // namespace pse_test
