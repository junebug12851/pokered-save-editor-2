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
 * ▣ **MAP STORAGE** -- persistent save values that each belong to one specific map.
 *
 * A per-map VIEW over global save state. Every page is one place, and shows (top to bottom):
 *
 *   1. **The map's SCRIPT** -- its per-map script-progress value (`w<Map>CurScript`, the 97-byte
 *      block at save 0x289C, `world.scripts`). A dropdown of the map's named steps with a
 *      progression description each; "Something else…" opens the raw path (full byte range, with
 *      a WARNING beyond the map's own table -- steps dispatch through an unbounded jp-hl pointer
 *      table, so an out-of-range step is a real crash risk; shown, never refused).
 *   2. **The legacy minigame bytes** where the map has them (Vermilion trash cans, Cinnabar quiz,
 *      Safari counters -- `world.local`; notes/reference/gym-safari-state.md).
 *   3. **The map's FILTER FLAGS** (Twilight's term; pret/the game call them "missables") -- its
 *      hide/show object bits (`wToggleableObjectFlags`, 32 bytes at save 0x2852, `world.missables`;
 *      bit SET = HIDDEN). Each a Shown switch with a description,
 *      pret's known-issue oddities flagged amber, and the linked event flags surfaced live (the
 *      conflict hooks: a flag that says "done" beside an object that says otherwise is exactly the
 *      kind of combination the conflicts system watches).
 *
 * Everything full-range, hack values shown and never silently rewritten.
 * Reference: notes/reference/map-scripts-missables.md.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: panel
  objectName: "mapStorage"

  /// The panel's one title-bar "?" (MapDock renders it).
  readonly property string panelInfo: qsTr(
    "Everything the save stores about one specific place: the map's script progress (how far its "
    + "story events have advanced), its minigame values (trash cans, quiz, Safari counters), and "
    + "its Filter Flags — which people, items and Pokémon are on the map at all.\n\n"
    + "Pick a map; the one you're standing on is pre-selected when it has storage. Script steps "
    + "read like a story — each option says what's happening at that stage.\n\n"
    + "Custom values are always available and never refused; values the game can't handle are "
    + "flagged in words instead.")

  /// Harness-visible: the panel resolved its model.
  readonly property bool diagReady: panel.local !== null && panel.local !== undefined

  // BINDINGS, not an imperative refresh -- rebuilt by a Loader, and the models must re-resolve when
  // a different map/save loads. `revision` is the dependency a plain property access would miss;
  // `editTick` re-reads values after our own writes (invokable reads aren't notified).
  property int revision: 0
  property int editTick: 0
  Connections {
    target: brg.map
    function onChanged() { panel.revision++; panel.syncToCurrentMap(); }
  }

  readonly property var local: {
    panel.revision;   // a dependency, deliberately
    return brg.file.data.dataExpanded.world.local;
  }
  readonly property var wScripts: {
    panel.revision;
    return brg.file.data.dataExpanded.world.scripts;
  }
  readonly property var wMissables: {
    panel.revision;
    return brg.file.data.dataExpanded.world.missables;
  }
  readonly property var wEvents: {
    panel.revision;
    return brg.file.data.dataExpanded.world.events;
  }

  // ── The pages: every map with storage (script progress, minigame bytes, or missables) ─────────
  // Built by MapModel::storagePages(): one page per script-progress entry (97), plus missable-only
  // maps, with the legacy trio merged in (Safari COMBINED — Twilight, 2026-07-15).
  readonly property var storageMaps: { panel.revision; return brg.map.storagePages(); }

  property int page: 0   // index into storageMaps -- which map's storage is shown

  function pageForMap(mapInd) {
    for (let i = 0; i < storageMaps.length; i++)
      if (storageMaps[i].ids.indexOf(mapInd) !== -1)
        return i;
    return -1;
  }
  /// Pre-select the combo to the map you're standing on, when it has storage.
  function syncToCurrentMap() {
    const i = pageForMap(brg.map.mapInd);
    if (i !== -1)
      panel.page = i;
  }
  Component.onCompleted: syncToCurrentMap()

  readonly property var curPage: storageMaps[panel.page]

  /// Is the shown map the one you're actually on? (Drives the "you're here" / "not here" note.)
  readonly property bool onShownMap: {
    panel.revision;
    return curPage !== undefined && curPage.ids.indexOf(brg.map.mapInd) !== -1;
  }

  // ── field kit ─────────────────────────────────────────────────────────────────────────────────

  // A numeric value: title, a compact right-aligned number field, and a plain-English line. Full
  // storable range is accepted (hack values shown, never refused -- project rule); the blurb names
  // the range a real game uses.
  component NumRow: ColumnLayout {
    id: nrow
    property string title: ""
    property string blurb: ""
    property int value: 0
    property int maxStorable: 255      // a byte; the Safari step counter is a word (65535)
    signal setValue(int v)

    Layout.fillWidth: true
    spacing: 1

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      Label {
        text: nrow.title
        font.pixelSize: 12
        color: brg.settings.textColorDark
        Layout.fillWidth: true
        wrapMode: Text.Wrap
      }

      TextField {
        Layout.preferredWidth: 64
        font.pixelSize: 12
        horizontalAlignment: TextInput.AlignRight
        text: nrow.value
        inputMethodHints: Qt.ImhDigitsOnly
        validator: IntValidator { bottom: 0; top: nrow.maxStorable }
        onEditingFinished: {
          const v = parseInt(text);
          if (!isNaN(v))
            nrow.setValue(Math.max(0, Math.min(nrow.maxStorable, v)));
        }
      }
    }

    Label {
      text: nrow.blurb
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      font.pixelSize: 10
      opacity: 0.55
    }
  }

  // A boolean value the game REWRITES on its own -- shown, but flagged temporary (the ⚠ is a label,
  // never a reason to hide; the Character panel set the precedent).
  component TempFlagRow: ColumnLayout {
    id: trow
    property string title: ""
    property string blurb: ""
    property string tempWhy: ""
    property bool value: false
    signal toggle()

    Layout.fillWidth: true
    spacing: 1

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      MapWarnIcon { text: trow.tempWhy }

      Label {
        text: trow.title
        font.pixelSize: 12
        color: brg.settings.textColorDark
        Layout.fillWidth: true
        wrapMode: Text.Wrap
      }

      MapSwitch {
        checked: trow.value
        onToggled: trow.toggle()
      }
    }

    Label {
      text: trow.blurb
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      font.pixelSize: 10
      opacity: 0.55
    }
  }

  component ArmedNote: Label {
    Layout.fillWidth: true
    Layout.topMargin: 2
    wrapMode: Text.Wrap
    font.pixelSize: 10
    font.italic: true
    opacity: 0.5
  }

  readonly property string tempWhy: qsTr("The game rewrites this every frame — it's forced back to "
    + "\"not over\" the instant you're not in the Safari Zone, so editing it here has no lasting "
    + "effect. Verified on a real cartridge. Shown because nothing is hidden.")

  // ── body ────────────────────────────────────────────────────────────────────────────────────
  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // Reserve the 16px scrollbar lane (ui-patterns.md rule 2) so right-aligned fields don't slide
    // under the overlay bar.
    ColumnLayout {
      width: scroller.availableWidth - 16
      spacing: 6

      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6

        Label {
          Layout.fillWidth: true
          text: qsTr("Stored values that belong to a specific map")
          wrapMode: Text.Wrap
          opacity: 0.6
          font.pixelSize: 11
        }

        // The map picker -- only maps that HAVE storage. Pre-selected to the current map when it has any.
        ComboBox {
          id: mapCombo
          Layout.fillWidth: true
          font.pixelSize: 12
          model: panel.storageMaps.map(function(m) { return m.title; })
          currentIndex: panel.page
          onActivated: function(i) { panel.page = i; }
        }

        // "You're here" / "you're elsewhere" -- so it's clear when an edit is live vs. dormant.
        Label {
          Layout.fillWidth: true
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
          text: panel.onShownMap
                ? qsTr("You're on this map — edits here are live.")
                : qsTr("You're not on this map right now. The values are stored, but they only take "
                       + "effect once your save is here, mid-activity.")
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 2
          implicitHeight: 1
          color: brg.settings.dividerColor
        }

        // ── THE MAP'S SCRIPT (top — Twilight, 2026-07-16) ─────────────────────────────────────
        // The per-map script-progress value (w<Map>CurScript). The dropdown reads like a story:
        // each named step describes what's happening at that stage of the map's events.
        ColumnLayout {
          id: scriptSection
          Layout.fillWidth: true
          spacing: 4
          visible: panel.curPage !== undefined && panel.curPage.scriptInd >= 0

          readonly property int scriptInd: panel.curPage !== undefined ? panel.curPage.scriptInd : -1
          readonly property int stepCount: panel.curPage !== undefined ? panel.curPage.steps : 0
          readonly property var steps: {
            panel.revision;
            return scriptInd >= 0 ? brg.map.storageScriptSteps(scriptInd) : [];
          }
          readonly property int value: {
            panel.revision; panel.editTick;
            return panel.wScripts && scriptInd >= 0 ? panel.wScripts.scriptsAt(scriptInd) : 0;
          }
          readonly property bool valueIsNamed: {
            for (let i = 0; i < steps.length; i++)
              if (steps[i].value === value) return true;
            return false;
          }
          property bool customMode: false

          Label {
            text: qsTr("Map script")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: panel.curPage !== undefined && panel.curPage.desc ? panel.curPage.desc : ""
            visible: text !== ""
          }

          // Which option matches the stored value ("Something else…" = the last slot).
          readonly property int comboIndex: {
            panel.revision; panel.editTick;
            if (customMode || !valueIsNamed)
              return steps.length;
            for (let i = 0; i < steps.length; i++)
              if (steps[i].value === value) return i;
            return steps.length;
          }

          // The dropdown: the named steps + "Something else…" for a custom value.
          ComboBox {
            id: stepCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            font.pixelSize: 12
            visible: scriptSection.steps.length > 0
            textRole: "name"
            model: {
              const l = scriptSection.steps.slice();
              l.push({ value: -1, name: qsTr("Something else…"), desc: "" });
              return l;
            }
            onActivated: function(i) {
              if (i === model.length - 1) {           // "Something else…"
                scriptSection.customMode = true;
                return;
              }
              scriptSection.customMode = false;
              if (panel.wScripts) {
                panel.wScripts.scriptsSet(scriptSection.scriptInd, scriptSection.steps[i].value);
                panel.editTick++;
              }
            }
            // A model change (page switch) makes the ComboBox clamp currentIndex
            // itself — re-assert the stored step's index AFTER the new model lands.
            onModelChanged: currentIndex = scriptSection.comboIndex
          }
          // A Binding element, NOT a plain currentIndex binding: the combo's own
          // internal writes would sever a plain binding. `delayed` coalesces to the
          // end of the event loop, after the model rebuild — otherwise the index can
          // be applied to the OLD model and clamped (the Oak's-Lab-shows-"Default"
          // bug the screenshot review caught, twice).
          Binding {
            target: stepCombo
            property: "currentIndex"
            value: scriptSection.comboIndex
            delayed: true
          }

          // The selected step's progression description — the sense of WHERE the story is.
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            visible: stepCombo.visible && !scriptSection.customMode && text !== ""
            text: {
              panel.revision; panel.editTick;
              for (let i = 0; i < scriptSection.steps.length; i++)
                if (scriptSection.steps[i].value === scriptSection.value)
                  return scriptSection.steps[i].desc !== undefined ? scriptSection.steps[i].desc : "";
              return "";
            }
          }

          // The custom path: full byte range, never refused — warned when it's past the map's
          // own table (the unbounded jp-hl dispatch; same crash mechanism the event-flag research
          // documented).
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: scriptSection.steps.length === 0 || scriptSection.customMode
                     || !scriptSection.valueIsNamed

            Label {
              text: qsTr("Custom step value")
              font.pixelSize: 11
              color: brg.settings.textColorMid
              Layout.fillWidth: true
            }
            TextField {
              Layout.preferredWidth: 64
              font.pixelSize: 12
              horizontalAlignment: TextInput.AlignRight
              text: scriptSection.value
              inputMethodHints: Qt.ImhDigitsOnly
              validator: IntValidator { bottom: 0; top: 255 }
              onEditingFinished: {
                const v = parseInt(text);
                if (!isNaN(v) && panel.wScripts) {
                  panel.wScripts.scriptsSet(scriptSection.scriptInd,
                                            Math.max(0, Math.min(255, v)));
                  panel.editTick++;
                }
              }
            }
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            color: "#d55e00"
            visible: scriptSection.stepCount > 0 && scriptSection.value >= scriptSection.stepCount
            text: qsTr("⚠ Step %1 is beyond this map's script table (0–%2). The game dispatches "
                       + "steps through an unbounded pointer table — an out-of-range step makes it "
                       + "jump to garbage, a real crash risk. Stored as asked, never rewritten.")
                  .arg(scriptSection.value).arg(scriptSection.stepCount - 1)
          }
          ArmedNote {
            visible: scriptSection.visible
            text: qsTr("Durable in the save. It steers this map's scripted events the moment your "
                       + "save is there.")
          }

          Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 2
            implicitHeight: 1
            color: brg.settings.dividerColor
            visible: (panel.curPage !== undefined && panel.curPage.legacy >= 0)
                     || missableSection.list.length > 0
          }
        }

        // ── Vermilion Gym ─────────────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 0

          NumRow {
            title: qsTr("Trash-can switch 1")
            blurb: qsTr("Which trash can holds the FIRST hidden switch (0–14, even). Randomised when "
                        + "you enter the gym.")
            value: panel.local ? panel.local.lock1 : 0
            onSetValue: function(v) { if (panel.local) panel.local.lock1 = v; }
          }
          NumRow {
            title: qsTr("Trash-can switch 2")
            blurb: qsTr("Which trash can holds the SECOND switch (0–14). Chosen once you flip the "
                        + "first correctly.")
            value: panel.local ? panel.local.lock2 : 0
            onSetValue: function(v) { if (panel.local) panel.local.lock2 = v; }
          }
          ArmedNote {
            text: qsTr("Takes effect while you're mid-puzzle in Vermilion Gym.")
          }
        }

        // ── Cinnabar Gym ──────────────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 1

          NumRow {
            title: qsTr("Next wrong-answer opponent")
            blurb: qsTr("The trainer you battle if you get the NEXT quiz-gate answer wrong (a sprite "
                        + "index; 0 = none queued). Set fresh as you work through the gates.")
            value: panel.local ? panel.local.quizOpp : 0
            onSetValue: function(v) { if (panel.local) panel.local.quizOpp = v; }
          }
          ArmedNote {
            text: qsTr("Takes effect while you're mid-quiz in Cinnabar Gym.")
          }
        }

        // ── Safari Zone (combined) ────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.curPage !== undefined && panel.curPage.legacy === 2

          NumRow {
            title: qsTr("Steps left")
            blurb: qsTr("Steps remaining in this Safari run (a real run starts at 502). Reset to 502 "
                        + "when you enter through the gate.")
            value: panel.local ? panel.local.safariSteps : 0
            maxStorable: 65535     // a 2-byte counter (big-endian in the save)
            onSetValue: function(v) { if (panel.local) panel.local.safariSteps = v; }
          }
          NumRow {
            title: qsTr("Safari Balls")
            blurb: qsTr("Balls remaining (a run starts with 30). Reset to 30 on entry, 0 on the way out.")
            value: panel.local ? panel.local.safariBallCount : 0
            onSetValue: function(v) { if (panel.local) panel.local.safariBallCount = v; }
          }

          TempFlagRow {
            title: qsTr("Game over")
            blurb: qsTr("Marks the current Safari run as ended.")
            tempWhy: panel.tempWhy
            value: panel.local ? panel.local.safariGameOver : false
            onToggle: function() { if (panel.local) panel.local.safariGameOver = !panel.local.safariGameOver; }
          }

          ArmedNote {
            text: qsTr("Steps and balls take effect only while your save is inside the Safari Zone; "
                       + "the gate resets them to 502 / 30 on entry.")
          }
        }

        // ── FILTER FLAGS — who's on this map at all ────────────────────────────────────────────
        // The map's hide/show object bits (wToggleableObjectFlags, save 0x2852; bit SET = HIDDEN,
        // so the switch shows the intuitive direction: checked = on the map). "Filter Flags" is
        // Twilight's preferred term (2026-07-16, cased as a proper name) for what the game/pret
        // calls missables — the model classes keep the pret name; the UI says Filter Flags.
        // Sorted into its own group;
        // pret's known-issue oddballs wear an amber note; linked event flags surfaced live.
        ColumnLayout {
          id: missableSection
          Layout.fillWidth: true
          spacing: 6

          readonly property var list: {
            panel.revision;
            return panel.curPage !== undefined ? brg.map.storageMissables(panel.curPage.ids) : [];
          }
          visible: list.length > 0

          Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: brg.settings.dividerColor
            visible: panel.curPage !== undefined
                     && (panel.curPage.scriptInd >= 0 || panel.curPage.legacy >= 0)
          }

          Label {
            text: qsTr("Filter Flags — who's on the map")
            font.pixelSize: 12
            font.bold: true
            color: brg.settings.textColorDark
          }
          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: qsTr("Each switch is one person, item ball or Pokémon this map can hide or show. "
                       + "ON = it's there. The game flips these as the story advances.")
          }

          Repeater {
            model: missableSection.list

            delegate: ColumnLayout {
              id: mrow
              required property var modelData
              Layout.fillWidth: true
              spacing: 1

              readonly property bool hidden: {
                panel.revision; panel.editTick;
                return panel.wMissables ? panel.wMissables.missablesAt(modelData.ind) : false;
              }

              RowLayout {
                Layout.fillWidth: true
                spacing: 6

                MapWarnIcon {
                  visible: mrow.modelData.oddity !== ""
                  text: qsTr("pret's own note on this one: %1. The bit exists and is stored, but "
                             + "toggling it does nothing visible.").arg(mrow.modelData.oddity)
                }

                Label {
                  text: mrow.modelData.name
                  font.pixelSize: 12
                  color: brg.settings.textColorDark
                  Layout.fillWidth: true
                  wrapMode: Text.Wrap
                }

                MapSwitch {
                  checked: !mrow.hidden          // bit SET = HIDDEN; the switch says "on the map"
                  onToggled: {
                    if (panel.wMissables) {
                      panel.wMissables.missablesSet(mrow.modelData.ind, mrow.hidden ? false : true);
                      panel.editTick++;
                    }
                  }
                }
              }

              Label {
                text: mrow.modelData.desc
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                opacity: 0.55
                visible: text !== ""
              }

              // The conflict hooks: the event flags the game consults around this object,
              // with their LIVE state — so a contradiction is visible at a glance.
              Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 10
                color: "#b07d10"
                visible: mrow.modelData.linked.length > 0
                text: {
                  panel.revision; panel.editTick;
                  let parts = [];
                  for (let i = 0; i < mrow.modelData.linked.length; i++) {
                    const l = mrow.modelData.linked[i];
                    let state = qsTr("not modelled");
                    if (l.eventIndex >= 0 && panel.wEvents)
                      state = panel.wEvents.eventsAt(l.eventIndex) ? qsTr("ON") : qsTr("off");
                    parts.push(l.flag + " (" + state + ")");
                  }
                  return qsTr("Tied to %1 — the game toggles this object around that flag. A flag "
                              + "and a switch that disagree is untested territory.").arg(parts.join(", "));
                }
              }
            }
          }
        }

        Item { Layout.preferredHeight: 12 }   // breathing room at the bottom of the scroll
      }
    }
  }
}
