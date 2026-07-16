/*
 * Copyright 2026 Twilight  --  Apache-2.0 (see LICENSE).
 *
 * @file tst_flag_scenarios.cpp
 * @brief Event-flag scenarios, judged against the actual Game Boy.
 *
 * Sibling of tst_emu_parity: it does NOT drive the emulator itself. It launches the
 * single-shot batch runner scripts/emu/run_flag_scenarios.py as ONE QProcess with a
 * generous timeout (Qt owns the child's lifecycle, so nothing leaks), then reads the
 * JSON it wrote and asserts on it. This is deliberately the ONLY reliable shape for a
 * long emulator job here: a self-terminating subprocess run under CTest -- never an
 * interactive session (which leaks processes when a call times out mid-management).
 * See reference/emulator-verification.md -> "Flag-scenario test framework".
 *
 * The runner forges a save per scenario (any map / position / flag combination), boots
 * the real ROM on Continue, optionally drives the player, and classifies each as
 * healthy / crash / no-boot. This test verifies the control scenario is healthy and
 * every scenario produced a result; the crash outcomes (e.g. the Route 22 rival
 * conflict) are surfaced via qInfo -- crash detection is a heuristic, so it informs
 * rather than hard-fails, keeping the suite reliable.
 *
 * Local-only: needs the gitignored ROM (assets/references/backup.gb) and the
 * tmp/emu-venv PyBoy venv (scripts/emu/setup.ps1). Without either, every case SKIPs
 * and the suite stays green -- nothing here is required to build or ship the editor.
 */

#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace {

QString repoRoot()
{
  return QFileInfo(QString::fromUtf8(PSE_ASSETS_DIR)).absolutePath();
}

QString romPath()       { return repoRoot() + "/assets/references/backup.gb"; }
QString pythonPath()    { return repoRoot() + "/tmp/emu-venv/Scripts/python.exe"; }
QString runnerPath()    { return repoRoot() + "/scripts/emu/run_flag_scenarios.py"; }
QString scenariosPath() { return repoRoot() + "/scripts/emu/flag_scenarios.json"; }
QString resultPath()    { return repoRoot() + "/tmp/emu/flag_scenarios_result.json"; }

QString unavailableReason()
{
  if (!QFile::exists(romPath()))
    return "no ROM at assets/references/backup.gb (local-only verification)";
  if (!QFile::exists(pythonPath()))
    return "no emulator venv -- run scripts/emu/setup.ps1";
  if (!QFile::exists(runnerPath()))
    return "scripts/emu/run_flag_scenarios.py is missing";
  return QString();
}

} // namespace

class TestFlagScenarios : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void controlScenarioIsHealthy();
  void everyScenarioProducedAResult();
  void reportConflictOutcomes();

private:
  QJsonArray m_results;
  QJsonArray m_scenarios;
  bool m_ok = false;
};

void TestFlagScenarios::initTestCase()
{
  const QString why = unavailableReason();
  if (!why.isEmpty())
    QSKIP(qPrintable("flag-scenario verification unavailable: " + why));

  // the scenarios we asked for (to check the runner covered them all)
  QFile sf(scenariosPath());
  QVERIFY2(sf.open(QIODevice::ReadOnly), "cannot read flag_scenarios.json");
  m_scenarios = QJsonDocument::fromJson(sf.readAll()).array();
  sf.close();

  // ONE SCENARIO PER PROCESS. PyBoy cannot be safely re-instantiated inside a
  // single process (the 2nd/3rd instance hangs), so we launch the runner once per
  // scenario with --only. Each is a fresh boot in a fresh process, and Qt owns
  // every child's lifecycle (waitForFinished + kill) -- nothing leaks.
  for (const QJsonValue& sv : m_scenarios) {
    const QString name = sv.toObject().value("name").toString();
    const QString outFile = repoRoot() + "/tmp/emu/scn_" + name + ".json";
    QProcess py;
    py.setWorkingDirectory(repoRoot());
    py.start(pythonPath(), { runnerPath(), "--only", name, "--out", outFile });
    if (!py.waitForFinished(180000)) {   // one boot+drive; generous
      // Kill the WHOLE TREE: the venv python.exe is a LAUNCHER whose child
      // interpreter (and its spinning PyBoy) can survive QProcess::kill(). And a
      // wedge IS a verdict, not a harness failure: a hard-crashed Game Boy CPU
      // executes STOP, the clocks halt, the frame never completes, and PyBoy's
      // tick() spins forever (reproduced 2026-07-16 on a chimera forged save;
      // the glitch-music bad-bank probe recorded the same console behaviour).
      // Classify it as "hang" and carry on -- only the control gate hard-fails.
      QProcess::execute(QStringLiteral("taskkill"),
                        { QStringLiteral("/PID"), QString::number(py.processId()),
                          QStringLiteral("/T"), QStringLiteral("/F") });
      py.waitForFinished(5000);
      QJsonObject hung;
      hung["name"] = name;
      hung["result"] = QStringLiteral("hang");
      hung["expect"] = sv.toObject().value("expect");
      m_results.append(hung);
      qInfo().noquote() << name << "-> hang (console wedged; tree killed)";
      continue;
    }
    if (py.exitCode() == 2)
      QSKIP(qPrintable(QString::fromUtf8(py.readAllStandardError()).trimmed()));
    QCOMPARE(py.exitCode(), 0);
    QFile rf(outFile);
    QVERIFY2(rf.open(QIODevice::ReadOnly),
             qPrintable("no result file for " + name));
    const QJsonArray one =
        QJsonDocument::fromJson(rf.readAll()).object().value("results").toArray();
    rf.close();
    QVERIFY2(!one.isEmpty(), qPrintable("empty result for " + name));
    m_results.append(one.first());
  }
  QVERIFY2(!m_results.isEmpty(), "no scenario results");
  m_ok = true;
}

void TestFlagScenarios::controlScenarioIsHealthy()
{
  if (!m_ok) QSKIP("harness unavailable");
  bool found = false;
  for (const QJsonValue& v : m_results) {
    const QJsonObject o = v.toObject();
    if (o.value("name").toString() == "control-pallet") {
      found = true;
      QCOMPARE(o.value("result").toString(), QString("healthy"));
    }
  }
  QVERIFY2(found, "control-pallet scenario was not run");
}

void TestFlagScenarios::everyScenarioProducedAResult()
{
  if (!m_ok) QSKIP("harness unavailable");
  QCOMPARE(m_results.size(), m_scenarios.size());
  for (const QJsonValue& v : m_results) {
    const QJsonObject o = v.toObject();
    QVERIFY2(o.value("result").toString() != "error",
             qPrintable("scenario errored: " + o.value("name").toString()
                        + " -- " + o.value("error").toString()));
  }
}

void TestFlagScenarios::reportConflictOutcomes()
{
  if (!m_ok) QSKIP("harness unavailable");
  // Surface (not assert) the crash outcomes -- crash detection is heuristic.
  for (const QJsonValue& v : m_results) {
    const QJsonObject o = v.toObject();
    qInfo().noquote() << o.value("name").toString()
                      << "->" << o.value("result").toString()
                      << "(expected" << o.value("expect").toString() << ")";
  }
}

QTEST_APPLESS_MAIN(TestFlagScenarios)
#include "tst_flag_scenarios.moc"
