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
 * @file screenshooter.cpp
 * @brief Headless screenshot + animation-frame capture tool (NOT a CTest test).
 *
 * Boots the REAL application UI headless on the `offscreen` platform via the shared
 * GUI harness (tests/helpers/guiapp.h) -- the exact same engine/provider wiring the
 * app uses -- loads a populated save fixture, then walks the UI grabbing PNGs with
 * QQuickWindow::grabWindow():
 *
 *   - every top-level screen (Home, Trainer Card, Bag, Pokemon, Rival, Pokedex,
 *     Pokemart, ...) over the populated save;
 *   - the "View All" overview drawers (Pokemon + Bag) slid open;
 *   - the three Pokemon-editor tabs (General / DV-EV / Moves) + Glance pane;
 *   - both text-editor modes (quick-edit popup + full keyboard) and the keyboard's
 *     tileset "tileviewer";
 *   - hover states (font pill/tile preview tooltip; Pokedex tiles);
 *   - and FRAME SEQUENCES under frames/<name>/ for assembling GIFs (the animated
 *     in-game name preview / tileviewer, live typing, and a tab-cycle interaction).
 *
 * BYTE-FIDELITY: this tool ONLY ever reads the save in memory. It never calls
 * saveFile()/flattenData() and never writes a save byte -- it just renders the UI.
 *
 * Output dir: argv[1], else the PSE_SCREENSHOTS_DIR compile def (=<repo>/tmp/screenshots).
 * Each capture is independent + logged; one failing step never aborts the run, so a
 * partial UI change degrades to "that one shot is missing", not "no shots".
 *
 * Run headless:  QT_QPA_PLATFORM=offscreen ./screenshooter [outdir]
 * The software scene-graph backend (forced below unless overridden) makes offscreen
 * grabWindow() produce real pixels with no GPU -- so it also works in CI/Docker.
 * (Trade-off: the software backend skips MultiEffect shadows/desaturation -- a minor
 *  cosmetic loss; set QT_QUICK_BACKEND= to try the GPU path for effect-accurate shots.)
 */

#include "../helpers/guiapp.h"

#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QVariantHash>
#include <QPointF>

#include <pse-db/db.h>
#include <bridge/bridge.h>
#include <bridge/router.h>

#include <pse-savefile/filemanagement.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

// ---------------------------------------------------------------------------
// Small capture helpers (file-scope state kept tiny + obvious).
// ---------------------------------------------------------------------------
namespace {

QString g_outDir;
int     g_saved = 0;
int     g_failed = 0;

QString outPath(const QString& rel)
{
  const QString p = g_outDir + QStringLiteral("/") + rel;
  QDir().mkpath(QFileInfo(p).absolutePath());
  return p;
}

// Grab the live window into <outdir>/<rel>. Logs + counts; never throws.
bool grab(GuiApp& app, const QString& rel)
{
  QImage img = app.view()->grabWindow();
  if (img.isNull() || img.size().isEmpty()) {
    qWarning().noquote() << "  [FAIL] null/empty grab ->" << rel;
    ++g_failed;
    return false;
  }
  // grabWindow() returns PHYSICAL pixels (a HiDPI display renders at its DPR, e.g.
  // 1695x1110 on a 150% screen). Downsample to the LOGICAL window size so output is a
  // stable 1130x740 regardless of display scaling (a smooth downscale is, if anything,
  // crisper than a native DPR-1 render).
  const QSize logical(app.view()->width(), app.view()->height());
  if (logical.isValid() && !logical.isEmpty() && img.size() != logical)
    img = img.scaled(logical, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  img.setDevicePixelRatio(1.0);
  const QString p = outPath(rel);
  if (!img.save(p, "PNG")) {
    qWarning().noquote() << "  [FAIL] save ->" << p;
    ++g_failed;
    return false;
  }
  qInfo().noquote() << "  [ok ]" << rel << QStringLiteral("(%1x%2)").arg(img.width()).arg(img.height());
  ++g_saved;
  return true;
}

// First item anywhere under @p root that declares the QML property @p prop
// (custom props like "shown" / "showTileset" / "editorVisible" are distinctive).
QQuickItem* findByProp(QQuickItem* root, const char* prop)
{
  return GuiApp::findItem(root, [&](QQuickItem* i) {
    return i->metaObject()->indexOfProperty(prop) >= 0;
  });
}

QQuickItem* viewRoot(GuiApp& app)
{
  return qobject_cast<QQuickItem*>(app.view()->rootObject());
}

// Deliver a real hover (mouse move) to the centre of @p item so HoverHandlers /
// MouseArea hoverEnabled / ToolTips light up, then let the hover/tooltip settle.
void hover(GuiApp& app, QQuickItem* item, int settleMs = 900)
{
  if (!item) return;
  const QPointF c = item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0));
  QTest::mouseMove(app.view(), c.toPoint());
  app.settle(settleMs);
}

// Size the QQuickOverlay to fill the window. A plain QQuickView does NOT auto-size
// the Controls overlay the way an ApplicationWindow/QQuickWidget does -- it stays 0x0
// at the window centre, so `anchors.centerIn: Overlay.overlay` popups (the quick-edit
// name editor) land bottom-right instead of centered. Forcing the overlay's geometry
// makes those `centerIn` bindings re-evaluate and center correctly.
void fixOverlay(GuiApp& app)
{
  QQuickItem* ci = app.view()->contentItem();
  if (!ci) return;
  for (QObject* o : ci->findChildren<QObject*>())
    if (QString::fromLatin1(o->metaObject()->className()).contains(QLatin1String("QQuickOverlay")))
      if (QQuickItem* ov = qobject_cast<QQuickItem*>(o)) {
        ov->setX(0);
        ov->setY(0);
        ov->setWidth(app.view()->width());
        ov->setHeight(app.view()->height());
      }
}

} // namespace

// ---------------------------------------------------------------------------
// Capture routines. Each is self-contained + defensive.
// ---------------------------------------------------------------------------

// 1) Every registered non-modal screen (the ones with a Home tile), plus the
//    explicit modal screens, over the populated save.
static void captureScreens(GuiApp& app)
{
  qInfo().noquote() << "== screens ==";
  const QList<QString> names = Router::screens.keys();
  for (const QString& name : names) {
    Screen* s = Router::screens.value(name, nullptr);
    if (!s || s->url.isEmpty()) continue;     // the empty fallback screen
    if (s->modal)               continue;     // modals handled separately
    if (!s->homeBtn)            continue;     // detail screens need a selection context
    app.navigate(name);
    app.settle(140);
    grab(app, QStringLiteral("screens/%1.png").arg(name));
  }

  // Explicit modal screens (pushed onto the outer shell stack).
  const char* modals[] = { "about", "fileTools", "newFile" };
  for (const char* m : modals) {
    if (!Router::screens.contains(QString::fromLatin1(m))) continue;
    app.navigate(QString::fromLatin1(m));
    app.settle(160);
    grab(app, QStringLiteral("screens/modal_%1.png").arg(QString::fromLatin1(m)));
    app.closeTop();
    app.settle(60);
  }
}

// First item under @p from whose `text` property equals @p label (DFS).
static QQuickItem* itemByText(QQuickItem* from, const QString& label)
{
  return GuiApp::findItem(from, [&](QQuickItem* i) {
    return i->property("text").toString() == label;
  });
}

// 2) The Pokemon + Bag "View All" overview drawers slid open. Opened by CLICKING
//    the real footer "View All" button (the user action), which is robust across
//    both screens -- guessing the panel by a `shown` property hit the wrong item
//    on the Bag screen.
static void captureViewAll(GuiApp& app)
{
  qInfo().noquote() << "== view all drawers ==";
  const char* screens[] = { "pokemon", "bag" };
  for (const char* sc : screens) {
    app.navigate(QString::fromLatin1(sc));
    app.settle(140);
    QQuickItem* page = app.currentNonModal();
    QQuickItem* btn  = itemByText(page, QStringLiteral("View All"));
    if (!btn) {
      qWarning().noquote() << "  [skip] no View All button on" << sc;
      continue;
    }
    app.clickItem(btn);          // triggers the footer's onBtn1Clicked -> panel.shown = true
    app.settle(320);             // 200ms slide-in + breathing room
    grab(app, QStringLiteral("screens/%1_view_all.png").arg(QString::fromLatin1(sc)));
  }
}

// 3) The Pokemon deep editor: instantiate it on the live engine with a real party
//    mon as boxData, then grab each of the three tabs (General / DV-EV / Moves).
static void capturePokemonEditor(GuiApp& app)
{
  qInfo().noquote() << "== pokemon editor tabs ==";
  app.navigate(QStringLiteral("home"));
  app.settle(60);

  auto* exp = app.file()->data->dataExpanded;
  PokemonParty* mon = (exp->player->pokemon->pokemonCount() > 0)
                        ? exp->player->pokemon->partyAt(0) : nullptr;
  if (!mon) { qWarning().noquote() << "  [skip] save has no party mon"; return; }

  QVariantHash props;
  props.insert(QStringLiteral("boxData"),  QVariant::fromValue<PokemonBox*>(mon));
  props.insert(QStringLiteral("partyData"), QVariant::fromValue<PokemonBox*>(mon));
  QQuickItem* details =
      app.instantiate(QStringLiteral("qrc:/ui/app/screens/non-modal/PokemonDetails.qml"), props);
  if (!details) { qWarning().noquote() << "  [skip] PokemonDetails failed to instantiate"; return; }

  QQuickItem* bar = app.itemByType(QStringLiteral("TabBar"), details);
  const char* tabRel[] = { "editor/pokemon_editor_general.png",
                           "editor/pokemon_editor_dvev.png",
                           "editor/pokemon_editor_moves.png" };
  for (int t = 0; t < 3; ++t) {
    if (bar) bar->setProperty("currentIndex", t);
    app.settle(160);
    grab(app, QString::fromLatin1(tabRel[t]));
  }
  delete details;
  app.settle(40);
}

// 4) Both text-editor modes + the keyboard tileset, plus hover tooltips.
static void captureTextEditors(GuiApp& app)
{
  qInfo().noquote() << "== text editors ==";

  // -- Quick-edit popup: open it on the Trainer Card's player-name display (renders
  //    in the shell overlay, so it shows in the grab). --
  app.navigate(QStringLiteral("trainerCard"));
  app.settle(140);
  // The player-name display (a NameDisplay) is found by its distinctive custom
  // `editorVisible` property rather than by metatype name.
  if (QQuickItem* nd = findByProp(app.currentNonModal(), "editorVisible")) {
    nd->setProperty("editorVisible", true);
    app.settle(260);
    fixOverlay(app);            // re-center the popup (overlay sizing)
    app.settle(60);
    grab(app, QStringLiteral("editor/text_quick_popup.png"));
    nd->setProperty("editorVisible", false);
    app.settle(80);
  } else {
    qWarning().noquote() << "  [skip] no name editor on trainer card";
  }

  // -- Full keyboard modal. --
  if (Router::screens.contains(QStringLiteral("fullKeyboard"))) {
    app.navigate(QStringLiteral("fullKeyboard"));
    app.settle(240);
    grab(app, QStringLiteral("editor/text_full_keyboard.png"));

    QQuickItem* root = viewRoot(app);

    // Hover a real character "pill" (a SearchResults delegate -- has a `fontInd`
    // property) in the GRID view to raise its TilePreview tooltip. Pick a visible,
    // non-control pill (control codes get no tooltip) and wait out the ToolTip's
    // 250ms delay before grabbing.
    QList<QQuickItem*> pills;
    GuiApp::collectItems(root, [](QQuickItem* i) {
      return i->metaObject()->indexOfProperty("fontInd") >= 0;
    }, pills);
    QQuickItem* pill = nullptr;
    for (QQuickItem* p : pills) {
      QObject* f = p->property("f").value<QObject*>();
      if (!f) continue;
      const QString nm = f->property("name").toString().trimmed();
      const QPointF c = p->mapToScene(QPointF(p->width() / 2.0, p->height() / 2.0));
      // Skip control codes (no tooltip) and the blank "Space" (empty preview); pick a
      // visible glyph that's within the viewport.
      if (!f->property("control").toBool() && !nm.isEmpty() && nm != QLatin1String("Space")
          && c.y() > 0 && c.y() < app.view()->height()) {
        pill = p;
        break;
      }
    }
    if (pill) {
      hover(app, pill, 1000);      // exceed the ToolTip's 250ms delay so it shows
      grab(app, QStringLiteral("editor/text_keyboard_hover_tile.png"));
    } else {
      qWarning().noquote() << "  [skip] no hoverable keyboard pill found";
    }

    // Flip the Grid/Tileset paged toggle to show the tileset "tileviewer".
    if (QQuickItem* paged = findByProp(root, "showTileset")) {
      paged->setProperty("showTileset", true);
      app.settle(280);
      grab(app, QStringLiteral("editor/text_keyboard_tileset.png"));
    }

    app.closeTop();
    app.settle(80);
  }
}

// 6a) Animation frames: the tileset's animated tiles (water/flower) cycle across 8
//     frames. Only OUTDOOR tilesets actually move tiles, so set Overworld/outdoor,
//     then drive the TilesetDisplay's curFrame 0..7 deterministically (its own
//     animation Timer is stopped so each grab is a distinct, stable frame). This is
//     the "tileviewer" animation the full editor shows.
static void captureFramesTileset(GuiApp& app)
{
  qInfo().noquote() << "== frames: tileset animation ==";

  // Animated tiles only move outdoors -> use the Overworld tileset.
  if (QObject* st = qvariant_cast<QObject*>(app.bridge()->property("settings"))) {
    st->setProperty("previewTileset", QStringLiteral("Overworld"));
    st->setProperty("previewOutdoor", true);
  }
  if (!Router::screens.contains(QStringLiteral("fullKeyboard"))) {
    qWarning().noquote() << "  [skip] no fullKeyboard screen";
    return;
  }
  app.navigate(QStringLiteral("fullKeyboard"));
  app.settle(220);
  QQuickItem* root = viewRoot(app);
  if (QQuickItem* paged = findByProp(root, "showTileset")) {
    paged->setProperty("showTileset", true);
    app.settle(280);
  }

  // The animated tileset image: has curFrame AND an image://tileset source.
  QQuickItem* tile = GuiApp::findItem(root, [](QQuickItem* i) {
    return i->metaObject()->indexOfProperty("curFrame") >= 0
        && i->property("source").toString().contains(QStringLiteral("image://tileset"));
  });
  if (!tile) {
    qWarning().noquote() << "  [skip] no animated TilesetDisplay found";
    app.closeTop();
    return;
  }
  // Stop its self-running Timer so our manual curFrame steps aren't overwritten.
  for (QObject* c : tile->findChildren<QObject*>())
    if (QString::fromLatin1(c->metaObject()->className()).contains(QLatin1String("Timer")))
      c->setProperty("running", false);

  for (int f = 0; f < 8; ++f) {
    tile->setProperty("curFrame", f);
    app.settle(110);     // let the (uncached) provider re-render the frame
    grab(app, QStringLiteral("frames/tileset_anim/frame_%1.png").arg(f, 3, 10, QLatin1Char('0')));
  }
  app.closeTop();
  app.settle(80);
}

// 6b) Animation frames: live typing into the quick-edit name field.
static void captureFramesTyping(GuiApp& app)
{
  qInfo().noquote() << "== frames: typing ==";
  app.navigate(QStringLiteral("trainerCard"));
  app.settle(140);
  QQuickItem* nd = findByProp(app.currentNonModal(), "editorVisible");
  if (!nd) { qWarning().noquote() << "  [skip] no name editor"; return; }
  nd->setProperty("editorVisible", true);
  app.settle(300);
  fixOverlay(app);             // center the popup (overlay sizing)
  app.settle(60);

  // The popup's editable NAME field. The Popup is a QML child of this NameDisplay
  // (even though its visuals render in Overlay.overlay), so the field is reachable as
  // a descendant of nd. Identify it by its unique placeholder "Enter a name" (NOT the
  // first TextField -- that's the tileset combo's). Driving THIS field's text controls
  // the value: its onTextChanged pushes up to NameDisplay.str, so the field AND the
  // live GB-font preview update together (the textbox owns the value).
  QQuickItem* field = nullptr;
  for (QQuickItem* o : nd->findChildren<QQuickItem*>())   // QObject tree (incl. the overlay popup)
    if (o->property("placeholderText").toString().contains(QStringLiteral("Enter a name"))) {
      field = o; break;
    }
  if (!field) {   // fallback: the LAST TextField under nd (the name field follows the combo)
    for (QObject* o : nd->findChildren<QObject*>())
      if (QString::fromLatin1(o->metaObject()->className()).contains(QLatin1String("TextField")))
        field = qobject_cast<QQuickItem*>(o);
  }
  if (!field) {
    qWarning().noquote() << "  [skip] popup name field not found";
    nd->setProperty("editorVisible", false);
    return;
  }

  const QStringList steps = { QStringLiteral(""), QStringLiteral("P"), QStringLiteral("PI"),
                              QStringLiteral("PIK"), QStringLiteral("PIKA"),
                              QStringLiteral("PIKAC"), QStringLiteral("PIKACHU") };
  int f = 0;
  for (const QString& s : steps) {
    field->setProperty("text", s);          // type THROUGH the textbox; preview follows
    app.settle(170);
    grab(app, QStringLiteral("frames/typing/frame_%1.png").arg(f++, 3, 10, QLatin1Char('0')));
  }

  // Close without committing (don't mutate the in-memory player name / OT data).
  nd->setProperty("suppressNextCommit", true);
  nd->setProperty("editorVisible", false);
  app.settle(80);
}

// 6c) Animation frames: a tab-cycle interaction on the Pokemon editor.
static void captureFramesTabs(GuiApp& app)
{
  qInfo().noquote() << "== frames: editor tab cycle ==";
  app.navigate(QStringLiteral("home"));
  app.settle(60);
  auto* exp = app.file()->data->dataExpanded;
  PokemonParty* mon = (exp->player->pokemon->pokemonCount() > 0)
                        ? exp->player->pokemon->partyAt(0) : nullptr;
  if (!mon) { qWarning().noquote() << "  [skip] no party mon"; return; }

  QVariantHash props;
  props.insert(QStringLiteral("boxData"),  QVariant::fromValue<PokemonBox*>(mon));
  props.insert(QStringLiteral("partyData"), QVariant::fromValue<PokemonBox*>(mon));
  QQuickItem* details =
      app.instantiate(QStringLiteral("qrc:/ui/app/screens/non-modal/PokemonDetails.qml"), props);
  if (!details) { qWarning().noquote() << "  [skip] PokemonDetails failed"; return; }

  QQuickItem* bar = app.itemByType(QStringLiteral("TabBar"), details);
  const int seq[] = { 0, 1, 2, 1, 0 };
  int f = 0;
  for (int idx : seq) {
    if (bar) bar->setProperty("currentIndex", idx);
    app.settle(200);
    grab(app, QStringLiteral("frames/tab_cycle/frame_%1.png").arg(f++, 3, 10, QLatin1Char('0')));
  }
  delete details;
  app.settle(40);
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
  // NOTE: we deliberately do NOT force QT_QUICK_BACKEND=software. The software
  // backend silently drops MultiEffect/layered items (Credits cards, Home disabled
  // tiles, shadows). We let Qt use its default RHI so effects render. Set
  // PSE_FORCE_SOFTWARE=1 to opt into the software backend (last-resort CI fallback).
  if (!qEnvironmentVariableIsEmpty("PSE_FORCE_SOFTWARE"))
    qputenv("QT_QUICK_BACKEND", "software");


  // The offscreen platform uses Qt's FreeType font DB, which finds NO fonts unless
  // pointed at a font directory (otherwise all UI text renders as tofu boxes). Point
  // it at the OS font dir so labels/titles/buttons render. (Linux/CI: fontconfig
  // already serves fonts, so only set it when empty; honour any pre-set value.)
  if (qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR")) {
#if defined(Q_OS_WIN)
    QByteArray winDir = qgetenv("WINDIR");
    if (winDir.isEmpty()) winDir = "C:\\Windows";
    qputenv("QT_QPA_FONTDIR", winDir + "\\Fonts");
#elif defined(Q_OS_MAC)
    qputenv("QT_QPA_FONTDIR", "/System/Library/Fonts");
#endif
  }

  QGuiApplication qapp(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("PokeredSaveEditor"));

  g_outDir = (argc > 1)
               ? QString::fromLocal8Bit(argv[1])
#ifdef PSE_SCREENSHOTS_DIR
               : QStringLiteral(PSE_SCREENSHOTS_DIR);
#else
               : QDir::currentPath() + QStringLiteral("/screenshots");
#endif
  QDir(g_outDir).removeRecursively();   // start clean so no stale shots/frames linger
  QDir().mkpath(g_outDir);
  qInfo().noquote() << "Screenshot output:" << g_outDir;

  // Boot the real UI on the populated fixture; dismiss the startup New File modal.
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  if (!app.start()) {
    qCritical().noquote() << "App.qml failed to load -- aborting.";
    return 2;
  }
  // Render via a REAL GPU-backed window so MultiEffect / layered content renders
  // exactly like the app's QQuickWidget. The offscreen+software path silently drops
  // those items (missing Credits cards + Home disabled tiles, washed-out shadows).
  // Frameless + Tool + an off-screen position keeps it from flashing on the desktop.
  // (Under QT_QPA_PLATFORM=offscreen these are simply no-ops -- a CI fallback that
  // still runs, with the known effect-rendering limitation.)
  {
    QQuickView* v = app.view();
    v->setFlags(v->flags() | Qt::FramelessWindowHint | Qt::Tool);
    const QString plat = QGuiApplication::platformName();
    if (plat == QLatin1String("windows") || plat == QLatin1String("cocoa"))
      v->setPosition(-4000, -4000);   // off the visible desktop: renders on GPU, no flash
    else
      v->setPosition(0, 0);           // xvfb/X (CI): stay on the virtual screen so it exposes
    v->show();
  }
  app.settle(300);             // let the window expose + do its first GPU render
  fixOverlay(app);             // size the Controls overlay to the window (popup centering)
  app.closeTop();              // dismiss the New File modal -> Home
  app.settle(120);

  captureScreens(app);
  captureViewAll(app);
  capturePokemonEditor(app);
  captureTextEditors(app);
  captureFramesTileset(app);
  captureFramesTyping(app);
  captureFramesTabs(app);

  qInfo().noquote() << QStringLiteral("Done: %1 saved, %2 failed.").arg(g_saved).arg(g_failed);
  return 0;        // a tool: partial capture is still a successful run
}
