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

#include "../../ui/window/mainwindow.h"
#include "../bridge/bridge.h"
#include "../bridge/router.h"
#include <pse-savefile/filemanagement.h>

namespace {

QObject* findItem(const QString& name)
{
  auto* mw = MainWindow::getInstance();
  if(mw == nullptr) return nullptr;
  QObject* root = mw->qmlRootObject();
  if(root == nullptr) return nullptr;
  if(root->objectName() == name) return root;
  return root->findChild<QObject*>(name);
}

QJsonObject execute(const QJsonObject& c)
{
  const QString cmd = c.value(QStringLiteral("cmd")).toString();
  auto ok  = [](QJsonValue r){ QJsonObject o; o[QStringLiteral("ok")] = true;  o[QStringLiteral("result")] = r; return o; };
  auto err = [](const QString& m){ QJsonObject o; o[QStringLiteral("ok")] = false; o[QStringLiteral("error")]  = m; return o; };

  if(cmd == QStringLiteral("ping")) return ok(QStringLiteral("pong"));

  Bridge* brg = MainWindow::bridge;
  if(brg == nullptr) return err(QStringLiteral("bridge not ready"));

  if(cmd == QStringLiteral("title"))
    return ok(brg->router->title);

  if(cmd == QStringLiteral("screen")) {
    const QString s = c.value(QStringLiteral("arg")).toString();
    if(!Router::screens.contains(s)) return err(QStringLiteral("unknown screen: ") + s);
    if(s == QStringLiteral("pokemonDetails")) {
      if(auto* mw = MainWindow::getInstance())
        mw->debugOpenPartyDetails(c.value(QStringLiteral("index")).toInt(0));
    } else {
      brg->router->changeScreen(s);
    }
    return ok(brg->router->title);
  }

  if(cmd == QStringLiteral("party")) {
    if(auto* mw = MainWindow::getInstance())
      return ok(mw->debugOpenPartyDetails(c.value(QStringLiteral("arg")).toInt(0)));
    return err(QStringLiteral("no window"));
  }

  if(cmd == QStringLiteral("back")) {
    brg->router->closeScreen();
    return ok(brg->router->title);
  }

  if(cmd == QStringLiteral("home")) {
    brg->router->changeScreen(QStringLiteral("home"));
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
    return mw->saveShot(p) ? ok(p) : err(QStringLiteral("grab failed"));
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

  if(cmd == QStringLiteral("reload")) {
    if(auto* mw = MainWindow::getInstance()) { mw->reloadQml(); return ok(QStringLiteral("reloaded")); }
    return err(QStringLiteral("no window"));
  }

  if(cmd == QStringLiteral("list")) {
    const QString sub = c.value(QStringLiteral("arg")).toString();
    auto* mw = MainWindow::getInstance();
    QObject* root = mw ? mw->qmlRootObject() : nullptr;
    if(root == nullptr) return err(QStringLiteral("no root"));
    QJsonArray arr;
    const auto all = root->findChildren<QObject*>();
    for(QObject* o : all) {
      const QString n = o->objectName();
      if(!n.isEmpty() && (sub.isEmpty() || n.contains(sub, Qt::CaseInsensitive)))
        arr.append(n);
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
