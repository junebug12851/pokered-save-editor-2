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

/**
 * @file tst_qml_screens.cpp
 * @brief QML SCREEN SMOKE TEST -- loads EVERY registered screen through a real
 *        QQmlEngine configured exactly like MainWindow and FAILS on any QML
 *        warning or error.
 *
 * Why this exists: the C++ ctest suite never instantiates QML, so a screen that
 * fails to *load* (e.g. "Cannot override FINAL property" -> "Component is not
 * ready") or that loads but degrades (binding `TypeError`s, missing types,
 * missing image providers, anchor-on-null warnings) passes every C++ test yet is
 * broken in the app. Two such bugs reached `main` redesigning the Credits screen
 * on 2026-06-13 (a `Page.contentWidth` FINAL override; an `id: top` colliding
 * with the `top` anchor line) -- see reference/fix-patterns.md. This test is the
 * automated gate for that whole bug class.
 *
 * What it does, per screen (data-driven, one ctest row each):
 *   1. Build a QQmlComponent for the screen's qrc url.
 *   2. Fail on any component load/compile error (component.errors()).
 *   3. Instantiate it INTO A SIZED PARENT (begin/completeCreate) so screens that
 *      `anchors.fill: parent` / read `parent.width` resolve against a real,
 *      non-null parent -- exactly as when the app pushes them onto its StackView.
 *      Instantiating with a null parent would emit spurious anchor warnings.
 *   4. Spin the event loop briefly so `Component.onCompleted` + deferred bindings
 *      run, surfacing runtime QML warnings.
 *   5. FAIL if ANY qWarning/qCritical/qFatal was emitted while the screen loaded.
 *
 * The engine is wired to MATCH MainWindow::injectIntoQML + setupProviders:
 *   - `brg` context property (a real Bridge over the BaseSAV fixture),
 *   - the "tileset" and "font" image providers,
 *   - DB::inst()->qmlProtect(engine) (GC guard),
 *   - bootQmlLinkage() (the exe's full qRegisterMetaType + qmlRegister* set),
 *   - app.qrc compiled in (so qrc:/ui/app/... + qtquickcontrols2.conf resolve;
 *     the .conf auto-selects the Material style, same as the running app).
 * If MainWindow's wiring changes, mirror it here.
 *
 * Runs headless on the `offscreen` platform (set in tests/CMakeLists.txt), so it
 * is a Linux/CI gate and also surfaces platform-difference bugs.
 *
 * The screen list comes straight from Router::loadScreens() (the authoritative
 * registry), so it can never drift from the app's real screen set.
 */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QMutex>
#include <QStringList>
#include <QUrl>

#include "../helpers/savefilefixture.h"

#include <pse-db/db.h>
#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>

#include <bridge/bridge.h>
#include <bridge/router.h>
#include <engine/tilesetprovider.h>
#include <engine/fontpreviewprovider.h>

#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

// bootQmlLinkage() lives in the executable (src/boot/bootQmlLinkage.cpp), not in
// appcore. We compile that same .cpp into this test target (see CMakeLists) so the
// QML type registration stays a SINGLE source of truth shared with the app.
extern void bootQmlLinkage();

using namespace pse_test;

// ---------------------------------------------------------------------------
// Message capture. While a screen is loading, every qWarning/qCritical/qFatal is
// appended to the active sink; a non-empty sink fails that screen's case. The
// QML engine routes its warnings (TypeErrors, missing types, anchor-on-null,
// missing image providers, ...) through the Qt message handler as QtWarningMsg.
// ---------------------------------------------------------------------------
namespace {

QMutex          g_mutex;
QStringList*    g_sink = nullptr;   // nullptr => ignore (outside a screen load)
QtMessageHandler g_prev = nullptr;

// Known-benign, NON-QML platform/test noise to ignore. Keep EMPTY by default;
// only add narrow, justified substrings (with a comment) if the offscreen
// platform ever emits unavoidable noise. Do NOT use this to paper over real
// QML warnings -- those are the whole point of the test.
bool isBenign(const QString& msg)
{
  static const char* const kAllow[] = {
    // Qt 6 no longer ships fonts and the offscreen platform has no system font dir,
    // so QFontDatabase logs this once at first text render. A headless-CI environment
    // artifact, not a QML/app bug (text still lays out via the fallback font).
    "Cannot find font directory",
  };
  for (const char* s : kAllow)
    if (msg.contains(QLatin1String(s)))
      return true;
  return false;
}

void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
  if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
    QMutexLocker lock(&g_mutex);
    if (g_sink != nullptr && !isBenign(msg)) {
      const QString cat = QString::fromLatin1(ctx.category ? ctx.category : "default");
      g_sink->append(cat + QStringLiteral(": ") + msg);
    }
  }
  // Still echo to the previous handler so the messages show in the test log.
  // (Skip forwarding QtFatalMsg to avoid the default abort() mid-run; the captured
  // message already fails the case.)
  if (g_prev != nullptr && type != QtFatalMsg)
    g_prev(type, ctx, msg);
}

} // namespace

class TestQmlScreens : public QObject
{
  Q_OBJECT

  FileManagement* m_file   = nullptr;
  Bridge*         m_brg    = nullptr;
  QQmlEngine*     m_engine = nullptr;
  QQuickWindow*   m_window = nullptr;

private slots:
  void initTestCase();
  void cleanupTestCase();
  void screenLoadsClean_data();
  void screenLoadsClean();
};

void TestQmlScreens::initTestCase()
{
  // Boot the game databases and the screen registry (the registry is also our
  // authoritative list of screens to test).
  DB::inst();
  Router::loadScreens();

  // The exe's full QML type registration (qRegisterMetaType + qmlRegister*).
  // Needed so screens that `import App.*` (enums, uncreatable types) resolve and
  // the whole brg.* pointer chain is traversable rather than `undefined`.
  bootQmlLinkage();

  // A real save behind `brg`, so screens bind to populated data (party, items,
  // dex, badges) -- the realistic path. Fixture is read-only; we copy into memory.
  m_file = new FileManagement;
  const QByteArray bytes = readSaveBytes(QStringLiteral("BaseSAV.sav"));
  QCOMPARE(bytes.size(), kSaveSize);   // fail loudly if the fixture is missing
  loadInto(*m_file->data, bytes);

  m_brg = new Bridge(m_file);

  // Engine wired to MATCH MainWindow::injectIntoQML + setupProviders.
  m_engine = new QQmlEngine(this);
  m_engine->rootContext()->setContextProperty(QStringLiteral("brg"), m_brg);
  m_engine->addImageProvider(QStringLiteral("tileset"), new TilesetProvider);
  m_engine->addImageProvider(QStringLiteral("font"),
                             new FontPreviewProvider(m_file->data->dataExpanded));
  DB::inst()->qmlProtect(m_engine);   // GC guard (s13f) -- mirrors the app

  // A sized offscreen window: screens get a real, non-null, sized parent (as if
  // pushed onto the app's StackView) so anchors/parent.width resolve cleanly.
  m_window = new QQuickWindow;
  m_window->resize(1024, 768);

  g_prev = qInstallMessageHandler(messageHandler);
}

void TestQmlScreens::cleanupTestCase()
{
  qInstallMessageHandler(g_prev);
  delete m_window;
  // m_engine is parented to `this`; providers are owned by the engine. m_brg /
  // m_file live for the process lifetime of the test (no teardown churn needed).
}

void TestQmlScreens::screenLoadsClean_data()
{
  QTest::addColumn<QString>("url");
  QTest::addColumn<bool>("detail");

  // Straight from the Router registry: no drift from the app's real screen set.
  const QList<QString> names = Router::screens.keys();
  for (const QString& name : names) {
    Screen* s = Router::screens.value(name, nullptr);
    if (s == nullptr || s->url.isEmpty())
      continue;   // skip the empty "" fallback screen (no url)
    // Detail screens (homeBtn == false: pokemonDetails, mapDetails) are only ever
    // shown AFTER a parent selection (their `boxData`/map property is null until
    // navigation sets it). Default to LOAD-ONLY coverage (compile/instantiate without
    // component errors -- still catching the Credits-class FINAL/"not ready" bug).
    // EXCEPTION: pokemonDetails is given a REAL party mon as `boxData` below (as the
    // app does) and then held to the full zero-warning bar -- so the Pokemon editor's
    // runtime bindings ARE verified here. mapDetails stays load-only (Maps feature is
    // not wired; no map selection to inject yet).
    QTest::newRow(name.toUtf8().constData()) << s->url << !s->homeBtn;
  }
}

void TestQmlScreens::screenLoadsClean()
{
  QFETCH(QString, url);
  QFETCH(bool, detail);

  QStringList captured;
  { QMutexLocker lock(&g_mutex); g_sink = &captured; }

  QQmlComponent component(m_engine, QUrl(url));

  // (1) Compile/load errors: FINAL-override, "is not a type", bad import, etc.
  if (component.isError()) {
    QStringList errs;
    const auto list = component.errors();
    for (const auto& e : list)
      errs << e.toString();
    { QMutexLocker lock(&g_mutex); g_sink = nullptr; }
    QFAIL(qPrintable(QStringLiteral("%1 failed to load:\n%2").arg(url, errs.join(QLatin1Char('\n')))));
  }

  // (2) Instantiate into the sized parent so anchors resolve (begin/completeCreate).
  QObject* obj = component.beginCreate(m_engine->rootContext());
  QVERIFY2(obj != nullptr, qPrintable(QStringLiteral("beginCreate returned null for ") + url));
  if (auto* item = qobject_cast<QQuickItem*>(obj)) {
    item->setParentItem(m_window->contentItem());
    item->setWidth(m_window->width());
    item->setHeight(m_window->height());
  }

  // The Pokemon editor is a detail screen, but unlike a cold load we can give it a
  // REAL selection -- exactly as the app does (PokemonBoxView.openMonEditor pushes it
  // with `boxData: monData`). With a populated fixture the party has mons, so inject
  // party mon 0 as boxData BEFORE completeCreate; its bindings then resolve against
  // real data and we demand it loads as CLEAN as any other screen (not load-only).
  if (url.contains(QLatin1String("PokemonDetails.qml"))) {
    PlayerPokemon* party = m_file->data->dataExpanded->player->pokemon;
    if (party && party->pokemonCount() > 0) {
      PokemonParty* mon = party->partyAt(0);   // PokemonParty IS-A PokemonBox
      if (mon) {
        obj->setProperty("boxData",   QVariant::fromValue<QObject*>(mon));
        obj->setProperty("partyData", QVariant::fromValue<QObject*>(mon));
        detail = false;   // selection-backed -> hold it to the full zero-warning bar
      }
    }
  }

  component.completeCreate();

  // (3) Let Component.onCompleted + deferred bindings run (runtime TypeErrors surface here).
  QTest::qWait(50);

  { QMutexLocker lock(&g_mutex); g_sink = nullptr; }

  delete obj;   // tear the screen down before the next row reuses the parent

  // Detail screens: load-only (see _data). A cold null-selection state legitimately
  // produces binding warnings here; tolerate them but log so they're visible. Their
  // real-state runtime cleanliness is the detail-flow GUI test's job.
  if (detail) {
    if (!captured.isEmpty())
      qInfo("%s loaded (detail screen, load-only) with %d cold-state binding warning(s) "
            "-- runtime verified by the detail-flow GUI test.", qPrintable(url), int(captured.size()));
    return;
  }

  if (!captured.isEmpty())
    QFAIL(qPrintable(QStringLiteral("%1 produced %2 QML warning(s)/error(s):\n%3")
                       .arg(url).arg(captured.size()).arg(captured.join(QLatin1Char('\n')))));
}

QTEST_MAIN(TestQmlScreens)
#include "tst_qml_screens.moc"
