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
 * @file tst_common_random.cpp
 * @brief The Random chance/coin helpers not covered by tst_common: chanceSuccess/
 *        chanceFailure (integer 0-100 and float 0.00-1.00), the deterministic
 *        threshold edges, statistical variation of the 50/50 paths, and range(float).
 *        Edges are asserted over many iterations so a regression can't slip through
 *        on a lucky single draw.
 */

#include <QtTest>
#include <pse-common/random.h>

class TestCommonRandom : public QObject
{
  Q_OBJECT

  static constexpr int kIters = 400;

private slots:
  void chanceInteger_edgesAreDeterministic();
  void chanceFloat_edgesAreDeterministic();
  void chanceMid_producesBothOutcomes();
  void flipCoin_bothPathsVary();
  void rangeFloat_staysInBounds();
};

void TestCommonRandom::chanceInteger_edgesAreDeterministic()
{
  Random* r = Random::inst();
  for(int i = 0; i < kIters; i++) {
    // success rolls 0..100 <= percent; a 100% chance always succeeds.
    QVERIFY2(r->chanceSuccess(100), "chanceSuccess(100) returned false");
    // failure rolls 0..100 >= percent; a 0% chance always 'fails'.
    QVERIFY2(r->chanceFailure(0), "chanceFailure(0) returned false");
  }
}

void TestCommonRandom::chanceFloat_edgesAreDeterministic()
{
  Random* r = Random::inst();
  for(int i = 0; i < kIters; i++) {
    // range() is [0,1), so:
    QVERIFY2(r->chanceSuccess(1.0f), "chanceSuccess(1.0) returned false"); // roll < 1.0 <= 1.0
    QVERIFY2(r->chanceFailure(0.0f), "chanceFailure(0.0) returned false"); // roll >= 0.0 always
    QVERIFY2(!r->chanceFailure(1.0f), "chanceFailure(1.0) returned true"); // roll < 1.0 never >= 1.0
  }
}

void TestCommonRandom::chanceMid_producesBothOutcomes()
{
  Random* r = Random::inst();
  bool sawTrue = false, sawFalse = false;
  for(int i = 0; i < kIters && !(sawTrue && sawFalse); i++) {
    if(r->chanceSuccess(50)) sawTrue = true; else sawFalse = true;
  }
  QVERIFY2(sawTrue && sawFalse, "chanceSuccess(50) did not vary over 400 trials");
}

void TestCommonRandom::flipCoin_bothPathsVary()
{
  Random* r = Random::inst();
  bool it = false, ifa = false, ft = false, ffa = false;
  for(int i = 0; i < kIters; i++) {
    if(r->flipCoin())  it = true;  else ifa = true;
    if(r->flipCoinF()) ft = true;  else ffa = true;
  }
  QVERIFY2(it && ifa, "flipCoin() did not produce both outcomes");
  QVERIFY2(ft && ffa, "flipCoinF() did not produce both outcomes");
}

void TestCommonRandom::rangeFloat_staysInBounds()
{
  Random* r = Random::inst();
  for(int i = 0; i < kIters; i++) {
    const float v = r->range(10.0f);
    QVERIFY2(v >= 0.0f && v < 10.0f, "range(10.0f) out of [0,10) bounds");
  }
  // A zero span yields zero (bounded(0) is 0).
  QCOMPARE(r->range(0.0f), 0.0f);
}

QTEST_GUILESS_MAIN(TestCommonRandom)
#include "tst_common_random.moc"
