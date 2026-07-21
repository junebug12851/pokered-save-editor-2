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
 * @file tst_gui_moves.cpp
 * @brief End-to-end GUI tests for the Pokemon details "Moves" tab, focused on the
 *        class of bug that bit it in review: the PP / PP-Ups views getting OUT OF
 *        SYNC with each other and with the model. Drives the LIVE editor screen:
 *        - toggling the PP / PP-Ups view must never mutate any move's data;
 *        - the per-view value boxes are independent (editing PP never touches
 *          PP-Ups and vice-versa) and write through to the model;
 *        - both boxes always reflect their own field after model-side changes.
 *
 * The boxes carry stable objectNames ("movePP<i>" / "movePPUp<i>") so the test can
 * read/write them directly. Uses BaseSAV party mon 0 (Charizard, has 4 moves).
 */

#include <QtTest>
#include <QVector>

#include "../helpers/guiapp.h"

#include <pse-db/db.h>
#include <pse-savefile/savefile.h>
#include <pse-savefile/expanded/savefileexpanded.h>
#include <pse-savefile/expanded/player/player.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>

using namespace pse_test;

class TestGuiMoves : public QObject
{
  Q_OBJECT

  // The (id, pp, ppUp) of every move slot, in order -- the full move state.
  static QVector<std::array<int,3>> snapshot(PokemonBox* mon)
  {
    QVector<std::array<int,3>> out;
    for (int i = 0; i < 4; ++i)
      out.append({ mon->moves[i]->moveID, mon->moves[i]->pp, mon->moves[i]->ppUp });
    return out;
  }

  // Find the (first) item with @p objectName under @p root. itemByName() searches
  // from the window root, but an instantiated screen is parented elsewhere, so
  // search rooted at the editor instead.
  static QQuickItem* byName(QQuickItem* root, const QString& name)
  {
    QList<QQuickItem*> out;
    GuiApp::collectItems(root, [&](QQuickItem* i){ return i->objectName() == name; }, out);
    return out.isEmpty() ? nullptr : out.first();
  }

  // Instantiate the live editor bound to @p mon; returns the screen + the MovesTab.
  // Caller owns the screen (delete it).
  static QQuickItem* openEditor(GuiApp& app, PokemonBox* mon, QQuickItem** movesTabOut)
  {
    QVariantHash props;
    props.insert(QStringLiteral("boxData"),   QVariant::fromValue<QObject*>(mon));
    props.insert(QStringLiteral("partyData"), QVariant::fromValue<QObject*>(mon));
    QQuickItem* editor =
        app.instantiate(QStringLiteral("qrc:/ui/app/screens/non-modal/PokemonDetails.qml"), props);
    if (editor && movesTabOut)
      *movesTabOut = app.itemByType(QStringLiteral("MovesTab"), editor);
    return editor;
  }

private slots:
  void initTestCase();
  void viewToggle_neverMutatesData();
  void valueBoxes_areIndependentAndWriteThrough();
  void modelChange_reflectsInBothBoxes();
};

void TestGuiMoves::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("PSE-Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("PSE-Tests"));
  QVERIFY(DB::inst() != nullptr);
  QCOMPARE(readSaveBytes(QStringLiteral("saves/natural-clean/BaseSAV.sav")).size(), kSaveSize);
  GuiApp::installMessageHandler();
}

// Flipping PP <-> PP-Ups (the tab-level showPpUps) must be a pure VIEW change: it
// can never write a move's pp or ppUp. (The first cut shared one text box between
// the two views and the maxLength flip truncated PP into a ppUp write.)
void TestGuiMoves::viewToggle_neverMutatesData()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  PlayerPokemon* party = app.file()->data->dataExpanded->player->pokemon;
  if (party->pokemonCount() < 1) QSKIP("BaseSAV party empty.");
  PokemonParty* mon = party->partyAt(0);
  if (mon->movesCount() < 1) QSKIP("party mon 0 has no moves.");

  // Seed a known, distinct PP / PP-Up on each filled move.
  for (int i = 0; i < mon->movesCount(); ++i)
    mon->moves[i]->changeMove(mon->moves[i]->moveID, 7 + i, i % 4);

  QQuickItem* movesTab = nullptr;
  QQuickItem* editor = openEditor(app, mon, &movesTab);
  QVERIFY(editor != nullptr);
  QVERIFY(movesTab != nullptr);

  const QVector<std::array<int,3>> before = snapshot(mon);

  // Toggle the view many times; data must be byte-identical throughout.
  for (int k = 0; k < 8; ++k) {
    movesTab->setProperty("showPpUps", (k % 2) == 0);
    app.settle(20);
    QCOMPARE(snapshot(mon), before);
  }

  delete editor;
}

// The PP box and the PP-Up box are independent widgets bound to independent fields:
// writing one writes ONLY its field. Driving the boxes' text is exactly the path
// the user's typing takes (onTextChanged).
void TestGuiMoves::valueBoxes_areIndependentAndWriteThrough()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  PlayerPokemon* party = app.file()->data->dataExpanded->player->pokemon;
  if (party->pokemonCount() < 1) QSKIP("BaseSAV party empty.");
  PokemonParty* mon = party->partyAt(0);
  if (mon->movesCount() < 1) QSKIP("party mon 0 has no moves.");

  mon->moves[0]->changeMove(mon->moves[0]->moveID, 5, 0);

  QQuickItem* movesTab = nullptr;
  QQuickItem* editor = openEditor(app, mon, &movesTab);
  QVERIFY(editor != nullptr);

  QQuickItem* ppBox   = byName(editor, QStringLiteral("movePP0"));
  QQuickItem* ppUpBox = byName(editor, QStringLiteral("movePPUp0"));
  QVERIFY2(ppBox   != nullptr, "PP box not found");
  QVERIFY2(ppUpBox != nullptr, "PP-Up box not found");

  // Use values WITHIN the move's max PP -- the model legitimately clamps PP to the
  // cap (PokemonMove::onMoveIdChanged), which isn't what this test is about.
  const int maxPP = mon->moves[0]->getMaxPP();
  const int ppA = qMax(1, maxPP - 2);   // a valid current PP
  const int ppB = qMax(1, maxPP - 5);   // a second valid current PP

  // Edit PP -> only pp changes.
  ppBox->setProperty("text", QString::number(ppA));
  app.settle(20);
  QCOMPARE(mon->moves[0]->pp, ppA);
  QCOMPARE(mon->moves[0]->ppUp, 0);

  // Edit PP-Up -> only ppUp changes; pp untouched.
  ppUpBox->setProperty("text", QStringLiteral("3"));
  app.settle(20);
  QCOMPARE(mon->moves[0]->ppUp, 3);
  QCOMPARE(mon->moves[0]->pp, ppA);

  // Back to PP -> still independent.
  ppBox->setProperty("text", QString::number(ppB));
  app.settle(20);
  QCOMPARE(mon->moves[0]->pp, ppB);
  QCOMPARE(mon->moves[0]->ppUp, 3);

  delete editor;
}

// A model-side change to pp / ppUp must show up in the matching box (each box
// re-seats from its own field), and never in the other.
void TestGuiMoves::modelChange_reflectsInBothBoxes()
{
  GuiApp app(QStringLiteral("saves/natural-clean/BaseSAV.sav"));
  QVERIFY(app.start());
  app.closeTop();

  PlayerPokemon* party = app.file()->data->dataExpanded->player->pokemon;
  if (party->pokemonCount() < 1) QSKIP("BaseSAV party empty.");
  PokemonParty* mon = party->partyAt(0);
  if (mon->movesCount() < 1) QSKIP("party mon 0 has no moves.");

  QQuickItem* movesTab = nullptr;
  QQuickItem* editor = openEditor(app, mon, &movesTab);
  QVERIFY(editor != nullptr);

  QQuickItem* ppBox   = byName(editor, QStringLiteral("movePP0"));
  QQuickItem* ppUpBox = byName(editor, QStringLiteral("movePPUp0"));
  QVERIFY(ppBox && ppUpBox);

  mon->moves[0]->pp = 17;   mon->moves[0]->ppChanged();
  mon->moves[0]->ppUp = 2;  mon->moves[0]->ppUpChanged();
  app.settle(20);

  QCOMPARE(ppBox->property("text").toString(),   QStringLiteral("17"));
  QCOMPARE(ppUpBox->property("text").toString(), QStringLiteral("2"));

  // "Set max" on PP-Ups (the model call the →| button makes) -> ppUp box shows 3,
  // PP box unchanged.
  mon->moves[0]->maxPpUp();
  app.settle(20);
  QCOMPARE(ppUpBox->property("text").toString(), QStringLiteral("3"));
  QCOMPARE(ppBox->property("text").toString(),   QStringLiteral("17"));

  delete editor;
}

QTEST_MAIN(TestGuiMoves)
#include "tst_gui_moves.moc"
