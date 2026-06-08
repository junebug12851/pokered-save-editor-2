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
 * @file main.cpp (tests/qml)
 * @brief Phase-10 Qt Quick Test runner. QUICK_TEST_MAIN scans QUICK_TEST_SOURCE_DIR
 *        (set in CMake) for tst_*.qml cases and runs them. Runs headless via the
 *        `offscreen` platform plugin.
 *
 * This establishes the QML test harness. The current case (cases/tst_smoke.qml) is
 * self-contained (no `brg` context). Screen-level tests that bind to `brg.*` need
 * the app's Bridge + DB + registered types booted into the test's QML engine -- a
 * larger harness, noted as future work in notes/plans/testing.md.
 */

#include <QtQuickTest/quicktest.h>

QUICK_TEST_MAIN(pse_qml)
