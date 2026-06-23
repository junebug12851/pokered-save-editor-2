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
 * @file tst_visual_regression.cpp
 * @brief Visual-regression FLOOR: every screen renders real, non-blank content.
 *
 * Deliberately NOT a pixel-perfect baseline diff. The project is in an active
 * UI-polish phase and treats pixel/layout assertions as a trap (notes/context/
 * principles.md, plans/testing.md -> "QML / UI testing"): a per-pixel baseline would
 * fail on every intentional design tweak and fight the design work. Committing
 * baseline PNGs would also violate the "screenshots are git-ignored, never committed"
 * rule.
 *
 * Instead this catches the class tst_qml_screens can't: a screen that LOADS cleanly
 * (no QML warning) yet RENDERS BLANK -- an all-transparent/solid frame from a broken
 * binding, content positioned offscreen, a provider that yields nothing, etc. We grab
 * each screen via the real offscreen software renderer (same path as the screenshooter
 * tool) and require the frame to contain a non-trivial spread of distinct colors. A
 * blank/uniform frame has 1-3; any real screen (backgrounds, icons, anti-aliased text)
 * has hundreds, so the threshold is generous and stable across design changes.
 *
 * Runs headless on the offscreen platform with the software scene-graph backend
 * (forced in main(), like the screenshooter) so grabWindow() produces real pixels with
 * no GPU -- gating both CI jobs.
 */

#include <QtTest>
#include <QGuiApplication>
#include <QImage>
#include <QSet>
#include <QStringList>

#include "../helpers/guiapp.h"
#include <bridge/router.h>

using namespace pse_test;

namespace {
// Distinct colours over a coarse sample grid. A blank/uniform render yields ~1-3;
// any genuinely-painted screen yields far more. Sampling (not every pixel) keeps it
// fast and is plenty to distinguish "blank" from "painted".
int distinctSampledColors(const QImage& img)
{
  QSet<QRgb> seen;
  const int step = qMax(1, qMin(img.width(), img.height()) / 64); // ~64 samples/side
  for (int y = 0; y < img.height(); y += step)
    for (int x = 0; x < img.width(); x += step)
      seen.insert(img.pixel(x, y));
  return seen.size();
}

constexpr int kMinDistinctColors = 16; // generous: blank ~1-3, real screens hundreds
} // namespace

class TestVisualRegression : public QObject
{
  Q_OBJECT

private:
  GuiApp* m_app = nullptr;

private slots:
  void initTestCase();
  void cleanupTestCase();

  void everyHomeScreenRendersNonBlank();
};

void TestVisualRegression::initTestCase()
{
  // Fixture must exist (BaseSAV); GuiApp asserts the bytes on load.
  m_app = new GuiApp(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY2(m_app->start(), "App.qml shell failed to load");
  // First frame: prove the renderer actually paints (guards the whole approach --
  // if even the home shell were blank, every later assertion would be meaningless).
  m_app->settle(120);
  const QImage first = m_app->view()->grabWindow();
  QVERIFY2(!first.isNull() && !first.size().isEmpty(), "initial grabWindow() was null/empty");
  QVERIFY2(distinctSampledColors(first) >= kMinDistinctColors,
           "the app shell itself rendered blank -- offscreen software backend not painting?");
}

void TestVisualRegression::cleanupTestCase()
{
  delete m_app; m_app = nullptr;
}

void TestVisualRegression::everyHomeScreenRendersNonBlank()
{
  // Every non-modal, home-reachable screen (detail screens need a parent selection,
  // so they're excluded here -- same boundary the screenshooter uses).
  // 'maps' is a deliberately-unbuilt WIP feature (home tile disabled, map randomize
  // commented out -- see status.md / decisions). Its screen is intentionally sparse,
  // so exclude it from the non-blank floor rather than weaken the threshold for real
  // screens. Re-include it once the Maps feature is actually built.
  static const QSet<QString> kWipSkip = { QStringLiteral("maps") };

  QStringList bad;
  const QList<QString> names = Router::screens.keys();
  for (const QString& name : names) {
    Screen* s = Router::screens.value(name, nullptr);
    if (!s || s->url.isEmpty() || s->modal || !s->homeBtn)
      continue;
    if (kWipSkip.contains(name))
      continue;

    m_app->navigate(name);
    m_app->settle(80);

    const QImage img = m_app->view()->grabWindow();
    if (img.isNull() || img.size().isEmpty()) {
      bad << (name + QStringLiteral(" (null/empty grab)"));
      continue;
    }
    const int colors = distinctSampledColors(img);
    if (colors < kMinDistinctColors)
      bad << QStringLiteral("%1 (only %2 distinct colors -- blank/uniform render)")
                .arg(name).arg(colors);
  }

  QVERIFY2(bad.isEmpty(),
           qPrintable(QStringLiteral("screen(s) rendered blank/uniform: ") + bad.join(QStringLiteral(", "))));
}

int main(int argc, char** argv)
{
  // offscreen + software backend so grabWindow() yields real pixels with no GPU
  // (mirrors the screenshooter tool). Set before QGuiApplication.
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND"))
    qputenv("QT_QUICK_BACKEND", "software");
  QGuiApplication app(argc, argv);
  TestVisualRegression tc;
  return QTest::qExec(&tc, argc, argv);
}

#include "tst_visual_regression.moc"
