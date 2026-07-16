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
 * @file debugserver.cpp
 * @brief DEBUG-ONLY live control channel. A tiny newline-delimited-JSON server on
 *        127.0.0.1:8766 that lets an external tool drive and inspect the running app:
 *        send verbs (navigate, open a mon, click, set a property, screenshot) and ask
 *        for information (current title, a property value, matching objectNames).
 *
 *        Protocol: send one JSON object per line, get one JSON reply per line:
 *          -> {"cmd":"ping"}                              <- {"ok":true,"result":"pong"}
 *          -> {"cmd":"screen","arg":"trainerCard"}        navigate; reply = new title
 *          -> {"cmd":"party","arg":0}                     open party mon 0's details
 *          -> {"cmd":"back"} / {"cmd":"home"}             pop / go home
 *          -> {"cmd":"sav","arg":"C:/.../x.sav"}          load a save
 *          -> {"cmd":"shot","arg":"C:/.../out.png"}       grab the view to a PNG
 *          -> {"cmd":"title"}                             <- current screen title
 *          -> {"cmd":"get","obj":"trainerMoneyField","prop":"text"}   read a property
 *          -> {"cmd":"set","obj":"...","prop":"...","val":...}        write a property
 *          -> {"cmd":"click","obj":"..."}                 emit a control's clicked()
 *          -> {"cmd":"list","arg":"substr"}               <- matching objectNames
 *
 *        All handling runs on the GUI thread (QTcpServer/QTcpSocket signals fire there),
 *        so it is safe to touch QML/widgets directly. Compiled to a no-op in release
 *        (guarded by QT_DEBUG) -- the server does not exist in shipped binaries.
 */

#include <QApplication>

#ifdef QT_DEBUG

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QMetaObject>
#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMouseEvent>
#include <QPointF>
#include <QQuickItem>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariant>

#include "../../ui/window/mainwindow.h"
#include "../bridge/bridge.h"
#include "../bridge/router.h"
#include <pse-savefile/filemanagement.h>

namespace {

// Walk the WHOLE reachable tree: the QObject parent-child tree AND the Qt Quick VISUAL tree
// (QQuickItem::childItems()). This is the load-bearing bit -- a `Repeater`/model delegate (every
// dock rail button, every list row, anything built from an array or model) lives in the visual
// tree but is frequently NOT returned by QObject::findChildren, because its QObject parent is the
// creation context rather than the positioner it sits in. `findChildren` alone therefore exposes
// hand-declared items (mapRightPanel) but hides model-built ones (dockBtn_charstate) -- which breaks
// the harness promise that EVERYTHING is reachable by objectName. Walking childItems() too keeps it.
void collectTree(QObject* obj, QSet<QObject*>& seen, QObjectList& out)
{
  if(obj == nullptr || seen.contains(obj)) return;
  seen.insert(obj);
  out.append(obj);
  const auto kids = obj->children();
  for(QObject* k : kids) collectTree(k, seen, out);
  if(auto* item = qobject_cast<QQuickItem*>(obj)) {
    const auto items = item->childItems();
    for(QQuickItem* ci : items) collectTree(ci, seen, out);
  }
}

QObjectList reachableObjects()
{
  auto* mw = MainWindow::getInstance();
  QObject* root = mw ? mw->qmlRootObject() : nullptr;
  QObjectList out; QSet<QObject*> seen;
  if(root != nullptr) collectTree(root, seen, out);
  return out;
}

// One node's DIRECT children, in a stable, intuitive order: the VISUAL children first (what you see,
// so an index matches the on-screen order) then any remaining QObject children. This is the unit of
// downward navigation -- see resolvePath.
QObjectList directChildren(QObject* node)
{
  QObjectList out; QSet<QObject*> seen;
  if(auto* item = qobject_cast<QQuickItem*>(node)) {
    const auto items = item->childItems();
    for(QQuickItem* ci : items) if(!seen.contains(ci)) { out.append(ci); seen.insert(ci); }
  }
  const auto kids = node->children();
  for(QObject* k : kids) if(!seen.contains(k)) { out.append(k); seen.insert(k); }
  return out;
}

// Step down ONE level: a numeric segment is an index into directChildren; a bareword is the
// objectName of a direct child. Returns nullptr if the step doesn't resolve.
QObject* childStep(QObject* node, const QString& seg)
{
  if(node == nullptr) return nullptr;
  bool isNum = false;
  const int idx = seg.toInt(&isNum);
  const QObjectList kids = directChildren(node);
  if(isNum)
    return (idx >= 0 && idx < kids.size()) ? kids.at(idx) : nullptr;
  for(QObject* k : kids)
    if(k->objectName() == seg) return k;
  return nullptr;
}

QObject* qmlRoot()
{
  auto* mw = MainWindow::getInstance();
  return mw ? mw->qmlRootObject() : nullptr;
}

// Resolve a node by NAME or by PATH. A path is '/'-separated: the first segment names a starting
// node (any objectName anywhere, or "root"/"" for the QML root, or a bare index into the root), and
// each following segment steps DOWN one level (index or child objectName). This is what lets you
// reach ANYTHING -- even an unnamed control -- from a named ancestor, e.g. "mapRightPanel/2/0".
QObject* findByName(const QString& name)
{
  QObject* root = qmlRoot();
  if(root == nullptr) return nullptr;
  if(name.isEmpty() || name == QStringLiteral("root")) return root;
  if(root->objectName() == name) return root;
  if(QObject* hit = root->findChild<QObject*>(name)) return hit;   // fast QObject-tree path
  const auto all = reachableObjects();                             // then the full visual-tree walk
  for(QObject* o : all) if(o->objectName() == name) return o;
  return nullptr;
}

QObject* findItem(const QString& spec)
{
  if(!spec.contains(QLatin1Char('/')))
    return findByName(spec);

  const QStringList segs = spec.split(QLatin1Char('/'), Qt::SkipEmptyParts);
  if(segs.isEmpty()) return qmlRoot();
  QObject* node = findByName(segs.first());          // start at a named node (or root/index)
  if(node == nullptr) { bool n = false; segs.first().toInt(&n); if(n) node = childStep(qmlRoot(), segs.first()); }
  for(int i = 1; node != nullptr && i < segs.size(); i++)
    node = childStep(node, segs.at(i));
  return node;
}

// The registered NAME of the screen currently on top of the Router's stack (the
// reverse lookup reloadQml() uses). Empty if the stack is empty / unregistered.
QString currentScreenName()
{
  if(Router::stack.isEmpty()) return {};
  Screen* top = Router::stack.last();
  for(auto it = Router::screens.constBegin(); it != Router::screens.constEnd(); ++it)
    if(it.value() == top) return it.key();
  return {};
}

// Is either StackView (the modal layer `appRoot` or the page layer `appBody`)
// mid-transition right now?
bool anyStackBusy()
{
  const char* names[] = { "appRoot", "appBody" };
  for(const char* n : names)
    if(QObject* o = findByName(QString::fromLatin1(n)))
      if(o->property("busy").toBool()) return true;
  return false;
}

// Wait -- WITHOUT freezing the GUI thread -- until the StackView transitions are
// done (or the cap elapses). A local event loop keeps animations/timers running;
// we reply to the client only once the view has actually settled. This is what
// makes a `shot` taken right after a navigation clean instead of a half-faded
// mid-transition frame (the "distorted screenshots" of 2026-07-16).
void settleTransitions(int capMs = 1500)
{
  QElapsedTimer t;
  t.start();
  QEventLoop loop;
  QTimer tick;
  tick.setInterval(16);
  int calm = 0;
  QObject::connect(&tick, &QTimer::timeout, &loop, [&]() {
    calm = anyStackBusy() ? 0 : calm + 1;
    if(calm >= 3 || t.elapsed() >= capMs) loop.quit();
  });
  tick.start();
  loop.exec();
}

// A plain bounded event-processing pause (for `shot`'s optional settle).
void settleMs(int ms)
{
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

QJsonObject execute(const QJsonObject& c)
{
  const QString cmd = c.value(QStringLiteral("cmd")).toString();
  auto ok  = [](QJsonValue r){ QJsonObject o; o[QStringLiteral("ok")] = true;  o[QStringLiteral("result")] = r; return o; };
  auto err = [](const QString& m){ QJsonObject o; o[QStringLiteral("ok")] = false; o[QStringLiteral("error")]  = m; return o; };

  if(cmd == QStringLiteral("ping")) return ok(QStringLiteral("pong"));

  Bridge* brg = MainWindow::bridge;
  if(brg == nullptr) return err(QStringLiteral("bridge not ready"));

  if(cmd == QStringLiteral("title")) {
    // Reply carries BOTH the human title and the registered screen NAME, so a
    // client can compare against what it navigates by (names, not titles).
    QJsonObject o;
    o[QStringLiteral("ok")] = true;
    o[QStringLiteral("result")] = brg->router->title;
    o[QStringLiteral("screen")] = currentScreenName();
    return o;
  }

  if(cmd == QStringLiteral("screen")) {
    const QString s = c.value(QStringLiteral("arg")).toString();
    if(!Router::screens.contains(s)) return err(QStringLiteral("unknown screen: ") + s);
    // Refuse the duplicate push AT THE SOURCE (harness trap #1, dev-harness.md):
    // the Router pushes without checking, so navigating to the screen you are
    // already on stacks a dead copy behind the live one and every later
    // get/set/shot silently hits the dead one. Every client is protected here.
    // (pokemonDetails is exempt: re-navigating there switches the open mon.)
    if(s != QStringLiteral("pokemonDetails") && s == currentScreenName()) {
      QJsonObject o;
      o[QStringLiteral("ok")] = true;
      o[QStringLiteral("result")] = brg->router->title;
      o[QStringLiteral("already")] = true;   // told apart from a real navigation
      return o;
    }
    if(s == QStringLiteral("pokemonDetails")) {
      if(auto* mw = MainWindow::getInstance())
        mw->debugOpenPartyDetails(c.value(QStringLiteral("index")).toInt(0));
    } else {
      brg->router->changeScreen(s);
    }
    settleTransitions();   // reply only when the transition has finished
    return ok(brg->router->title);
  }

  if(cmd == QStringLiteral("party")) {
    if(auto* mw = MainWindow::getInstance()) {
      const bool opened = mw->debugOpenPartyDetails(c.value(QStringLiteral("arg")).toInt(0));
      settleTransitions();
      return ok(opened);
    }
    return err(QStringLiteral("no window"));
  }

  if(cmd == QStringLiteral("back")) {
    brg->router->closeScreen();
    settleTransitions();
    return ok(brg->router->title);
  }

  if(cmd == QStringLiteral("home")) {
    brg->router->changeScreen(QStringLiteral("home"));
    settleTransitions();
    return ok(QStringLiteral("home"));
  }

  if(cmd == QStringLiteral("sav")) {
    brg->file->setPath(c.value(QStringLiteral("arg")).toString());
    brg->file->reopenFile();
    return ok(brg->file->getPath());
  }

  if(cmd == QStringLiteral("shot")) {
    auto* mw = MainWindow::getInstance();
    if(mw == nullptr) return err(QStringLiteral("no window"));
    const QString p = c.value(QStringLiteral("arg")).toString();
    if(p.isEmpty()) return err(QStringLiteral("shot needs 'arg' path"));
    // Optional `obj` (name or path): crop the shot to that component's bounds -- grab one panel with
    // no manual cropping (Twilight, 2026-07-15).
    QObject* item = nullptr;
    if(c.contains(QStringLiteral("obj"))) {
      item = findItem(c.value(QStringLiteral("obj")).toString());
      if(item == nullptr) return err(QStringLiteral("no object"));
    }
    // Optional `settle` ms: let animations/fades finish (events keep running)
    // before grabbing, and always wait out any in-flight stack transition --
    // a grab mid-transition is the "distorted screenshot" (2026-07-16).
    if(anyStackBusy()) settleTransitions();
    const int settle = c.value(QStringLiteral("settle")).toInt(0);
    if(settle > 0) settleMs(qMin(settle, 5000));
    return mw->saveShot(p, item) ? ok(p) : err(QStringLiteral("grab failed"));
  }

  if(cmd == QStringLiteral("get")) {
    QObject* it = findItem(c.value(QStringLiteral("obj")).toString());
    if(it == nullptr) return err(QStringLiteral("no object: ") + c.value(QStringLiteral("obj")).toString());
    const QByteArray prop = c.value(QStringLiteral("prop")).toString().toUtf8();
    return ok(QJsonValue::fromVariant(it->property(prop.constData())));
  }

  if(cmd == QStringLiteral("set")) {
    QObject* it = findItem(c.value(QStringLiteral("obj")).toString());
    if(it == nullptr) return err(QStringLiteral("no object"));
    const QByteArray prop = c.value(QStringLiteral("prop")).toString().toUtf8();
    const bool okSet = it->setProperty(prop.constData(), c.value(QStringLiteral("val")).toVariant());
    return okSet ? ok(true) : err(QStringLiteral("set failed (no such property?)"));
  }

  if(cmd == QStringLiteral("click")) {
    QObject* it = findItem(c.value(QStringLiteral("obj")).toString());
    if(it == nullptr) return err(QStringLiteral("no object"));
    const bool okClick = QMetaObject::invokeMethod(it, "clicked");
    return okClick ? ok(true) : err(QStringLiteral("no clicked() signal"));
  }

  // ── invoke: trigger ANY signal / slot / Q_INVOKABLE by name (Twilight, 2026-07-15: "trigger
  // anything and any event/signal"). Emits a signal or calls a method on the target, with up to a
  // few string/number/bool args. `click` is the special case of this for `clicked()`.
  //   {"cmd":"invoke","obj":"someItem","method":"toggle"}
  //   {"cmd":"invoke","obj":"mapDock","method":"show","args":["charstate"]}
  if(cmd == QStringLiteral("invoke")) {
    QObject* it = findItem(c.value(QStringLiteral("obj")).toString());
    if(it == nullptr) return err(QStringLiteral("no object"));
    const QString method = c.value(QStringLiteral("method")).toString();
    if(method.isEmpty()) return err(QStringLiteral("invoke needs 'method'"));
    const QJsonArray a = c.value(QStringLiteral("args")).toArray();
    QVariantList vargs;
    for(const QJsonValue& v : a) vargs.append(v.toVariant());
    QGenericArgument g[4];
    for(int i = 0; i < vargs.size() && i < 4; i++)
      g[i] = QGenericArgument("QVariant", &vargs[i]);
    const bool okInv = QMetaObject::invokeMethod(it, method.toUtf8().constData(), Qt::DirectConnection,
                                                 g[0], g[1], g[2], g[3]);
    return okInv ? ok(true)
                 : err(QStringLiteral("no invokable/signal '") + method
                       + QStringLiteral("' (or arg types don't match)"));
  }

  // ── tap: a REAL mouse press+release, on the window ────────────────────────────────────────────
  //
  // ⚠️ `click` emits a control's `clicked()` signal directly. That is fine for driving a button, and
  // **useless for a whole class of bug**: anything to do with how a pointer event is DELIVERED --
  // which item grabs it, which handler consumes it, what it falls through to. Emitting the signal
  // skips all of that.
  //
  // That is exactly the bug that sent me out of our own tooling and into clicking the screen by hand
  // (2026-07-13, and Twilight rightly asked why). The answer is: the harness could not do it. Now it
  // can. `tap` posts a genuine QMouseEvent to the window at a scene coordinate, through Qt's real
  // delivery path -- grabs, handlers, propagation and all.
  //
  //   {"cmd":"tap","x":120,"y":300}          -- a scene coordinate
  //   {"cmd":"tap","obj":"someItem"}         -- ...or the centre of a named item
  if(cmd == QStringLiteral("tap")) {
    auto* mw = MainWindow::getInstance();
    if(mw == nullptr) return err(QStringLiteral("no window"));

    QPointF at;

    if(c.contains(QStringLiteral("obj"))) {
      QObject* o = findItem(c.value(QStringLiteral("obj")).toString());
      auto* item = qobject_cast<QQuickItem*>(o);
      if(item == nullptr)
        return err(QStringLiteral("no item: ") + c.value(QStringLiteral("obj")).toString());

      at = item->mapToScene(QPointF(item->width() / 2.0, item->height() / 2.0));
    }
    else {
      at = QPointF(c.value(QStringLiteral("x")).toDouble(),
                   c.value(QStringLiteral("y")).toDouble());
    }

    // single or double, and which button -- so double-clicks and right-click menus drive too.
    const bool dbl = c.value(QStringLiteral("double")).toBool()
                     || c.value(QStringLiteral("clicks")).toInt() >= 2;
    const QString b = c.value(QStringLiteral("button")).toString();
    const Qt::MouseButton button = (b == QStringLiteral("right"))  ? Qt::RightButton
                                 : (b == QStringLiteral("middle")) ? Qt::MiddleButton
                                                                   : Qt::LeftButton;
    if(!mw->debugTap(at, dbl ? 2 : 1, button))
      return err(QStringLiteral("tap failed"));

    QJsonObject where;
    where[QStringLiteral("x")] = at.x();
    where[QStringLiteral("y")] = at.y();
    return ok(where);
  }

  if(cmd == QStringLiteral("reload")) {
    if(auto* mw = MainWindow::getInstance()) { mw->reloadQml(); return ok(QStringLiteral("reloaded")); }
    return err(QStringLiteral("no window"));
  }

  if(cmd == QStringLiteral("list")) {
    auto typeName = [](QObject* o) {
      QString t = QString::fromUtf8(o->metaObject()->className());
      const int cut = t.indexOf(QStringLiteral("_QML"));   // strip "_QMLTYPE_123" noise
      return cut > 0 ? t.left(cut) : t;
    };

    // WITH `obj`: enumerate that node's DIRECT children so you can navigate DOWN by index -- reaching
    // even UNNAMED controls. Each entry: "[i] Type name" (name blank if unnamed). Pair the index with
    // a path: `list obj=mapRightPanel` -> pick `[2]` -> `click obj=mapRightPanel/2`.
    if(c.contains(QStringLiteral("obj"))) {
      QObject* node = findItem(c.value(QStringLiteral("obj")).toString());
      if(node == nullptr) return err(QStringLiteral("no object"));
      QJsonArray arr;
      const QObjectList kids = directChildren(node);
      for(int i = 0; i < kids.size(); i++)
        arr.append(QStringLiteral("[%1] %2 %3").arg(i).arg(typeName(kids.at(i)), kids.at(i)->objectName()));
      return ok(arr);
    }

    // WITHOUT `obj`: the FULL reachable tree (QObject + visual), matched by objectName substring.
    const QString sub = c.value(QStringLiteral("arg")).toString();
    const auto all = reachableObjects();
    QJsonArray arr;
    QSet<QString> seen;   // an objectName can repeat across rows; report each once
    for(QObject* o : all) {
      const QString n = o->objectName();
      if(n.isEmpty() || seen.contains(n)) continue;
      if(sub.isEmpty() || n.contains(sub, Qt::CaseInsensitive)) { arr.append(n); seen.insert(n); }
    }
    return ok(arr);
  }

  return err(QStringLiteral("unknown cmd: ") + cmd);
}

} // namespace

void startDebugServer()
{
  auto* server = new QTcpServer(qApp);
  const quint16 port = 8766;
  if(!server->listen(QHostAddress::LocalHost, port)) {
    qWarning() << "[debug-server] could NOT listen on 127.0.0.1:" << port
               << "--" << server->errorString();
    return;
  }
  qInfo() << "[debug-server] listening on 127.0.0.1:" << port
          << "(newline-delimited JSON)";

  QObject::connect(server, &QTcpServer::newConnection, qApp, [server]() {
    while(server->hasPendingConnections()) {
      QTcpSocket* sock = server->nextPendingConnection();
      QObject::connect(sock, &QTcpSocket::readyRead, sock, [sock]() {
        while(sock->canReadLine()) {
          const QByteArray line = sock->readLine().trimmed();
          if(line.isEmpty()) continue;
          QJsonParseError pe;
          const QJsonDocument doc = QJsonDocument::fromJson(line, &pe);
          QJsonObject resp;
          if(pe.error != QJsonParseError::NoError || !doc.isObject()) {
            resp[QStringLiteral("ok")] = false;
            resp[QStringLiteral("error")] = QStringLiteral("bad json: ") + pe.errorString();
          } else {
            resp = execute(doc.object());
          }
          sock->write(QJsonDocument(resp).toJson(QJsonDocument::Compact));
          sock->write("\n");
          sock->flush();
        }
      });
      QObject::connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }
  });
}

#else

void startDebugServer() {}

#endif
