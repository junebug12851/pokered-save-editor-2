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
 * These six bytes are global in the save (one copy each, in the Main-Data block), but every one is
 * *clearly map-specific*: the two Vermilion Gym trash-can switches, the Cinnabar Gym quiz's next
 * opponent, and the Safari Zone run counters. So the panel is a per-map VIEW over global state: pick a
 * map, edit its stored values. Only maps that HAVE stored values are listed (three today; more as
 * their storage is defined). The map combo pre-selects the map you're standing on when it has any.
 *
 * The model is `world.local` (WorldLocal) -- bound directly, like the Character panel binds `area.npc`.
 * Domain + console verification (5 of 6 survive a Continue; the Safari game-over flag does NOT --
 * it's rewritten every frame): notes/reference/gym-safari-state.md.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: panel
  objectName: "mapStorage"

  /// The panel's one title-bar "?" (MapDock renders it).
  readonly property string panelInfo: qsTr(
    "Persistent save values that each belong to one specific place — the Vermilion Gym trash-can "
    + "switches, the Cinnabar Gym quiz opponent, and the Safari Zone run counters.\n\n"
    + "Pick a map to see its stored values. Only maps that have stored values are listed.\n\n"
    + "These values are global in the save but each belongs to one map, so editing one only takes "
    + "effect while your save is at that place, mid-activity — the game re-sets them when you next "
    + "walk in. Verified on a real Game Boy.")

  /// Harness-visible: the panel resolved its model.
  readonly property bool diagReady: panel.local !== null && panel.local !== undefined

  // BINDINGS, not an imperative refresh -- rebuilt by a Loader, and `local` must re-resolve when a
  // different map loads. `revision` is the dependency a plain property access would miss.
  property int revision: 0
  Connections {
    target: brg.map
    function onChanged() { panel.revision++; panel.syncToCurrentMap(); }
  }

  readonly property var local: {
    panel.revision;   // a dependency, deliberately
    return brg.file.data.dataExpanded.world.local;
  }

  // ── The maps that have persistent storage ─────────────────────────────────────────────────────
  // Each: a display name + the map ids it covers. Ids from pret/pokered constants/map_constants.asm.
  // Safari Zone is COMBINED (Twilight, 2026-07-15): its several sub-maps share one counter set, so
  // they collapse to one entry. Only these three exist today; more get added as their storage is
  // defined (Twilight hasn't delved into the rest yet).
  readonly property var storageMaps: [
    { name: qsTr("Vermilion Gym"), ids: [0x5C] },                                             // $5C
    { name: qsTr("Cinnabar Gym"),  ids: [0xA6] },                                             // $A6
    { name: qsTr("Safari Zone"),   ids: [0x9C, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1] }
  ]

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

  /// Is the shown map the one you're actually on? (Drives the "you're here" / "not here" note.)
  readonly property bool onShownMap: {
    panel.revision;
    return storageMaps[panel.page] !== undefined
           && storageMaps[panel.page].ids.indexOf(brg.map.mapInd) !== -1;
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
          model: panel.storageMaps.map(function(m) { return m.name; })
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

        // ── Vermilion Gym ─────────────────────────────────────────────────────────────────────
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 6
          visible: panel.page === 0

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
          visible: panel.page === 1

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
          visible: panel.page === 2

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

        Item { Layout.preferredHeight: 12 }   // breathing room at the bottom of the scroll
      }
    }
  }
}
