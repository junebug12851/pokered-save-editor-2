/*
  * Copyright 2026 Fairy Fox
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
 * @file tst_common_qml.cpp
 * @brief The QML-integration helpers of Utility/Random not reachable from the
 *        pure-logic common tests: random() accessor, qmlProtectUtil/qmlProtect
 *        (GC-ownership guards), qmlHook (context property), and the once-only
 *        qmlRegister latch. A headless QQmlEngine drives the ownership/context
 *        calls; the private qmlRegister slot is re-invoked via the meta-object to
 *        cover its already-registered early-return.
 */

#include <QtTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QObject>

#include <pse-common/utility.h>
#include <pse-common/random.h>

class TestCommonQml : public QObject
{
  Q_OBJECT

private slots:
  void utility_randomAccessor();
  void qmlProtect_and_hook_runWithEngine();
  void qmlRegister_secondCallIsNoOp();
};

void TestCommonQml::utility_randomAccessor()
{
  QCOMPARE(Utility::inst()->random(), Random::inst());
}

void TestCommonQml::qmlProtect_and_hook_runWithEngine()
{
  QQmlEngine engine;

  // qmlProtect cascades: Utility protects itself + the Random it exposes. Both
  // end up CppOwnership so QML can't GC them.
  Utility::inst()->qmlProtect(&engine);
  QCOMPARE(QQmlEngine::objectOwnership(Utility::inst()), QQmlEngine::CppOwnership);
  QCOMPARE(QQmlEngine::objectOwnership(Random::inst()), QQmlEngine::CppOwnership);

  // Random's own qmlProtect (the cascade target, also callable directly).
  Random::inst()->qmlProtect(&engine);

  // The static helper on an arbitrary object.
  QObject obj;
  Utility::qmlProtectUtil(&obj, &engine);
  QCOMPARE(QQmlEngine::objectOwnership(&obj), QQmlEngine::CppOwnership);

  // qmlHook exposes Utility as a context property.
  Utility::inst()->qmlHook(engine.rootContext());
  QCOMPARE(engine.rootContext()->contextProperty("pseCommon").value<QObject*>(),
           static_cast<QObject*>(Utility::inst()));
}

void TestCommonQml::qmlRegister_secondCallIsNoOp()
{
  // qmlRegister() is a private slot already run once from each constructor; invoke
  // it again through the meta-object to exercise the already-registered branch.
  QVERIFY(QMetaObject::invokeMethod(Utility::inst(), "qmlRegister", Qt::DirectConnection));
  QVERIFY(QMetaObject::invokeMethod(Random::inst(),  "qmlRegister", Qt::DirectConnection));
}

QTEST_GUILESS_MAIN(TestCommonQml)
#include "tst_common_qml.moc"
