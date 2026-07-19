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
 * The DETAILS panel -- it edits whatever is selected, and it is never blank.
 *
 * With **nothing selected** it shows the MAP's own details. With a **sprite selected** it shows
 * every byte that sprite has, grouped and explained, each one editable across its **full range** --
 * hack and glitch values included, flagged in words, never refused and never quietly corrected.
 *
 * Right now sprites are the only selectable thing on the map (the ground is deliberately not
 * clickable); warps, signs and connections join them later, on the same machinery.
 *
 * ⚠️ **REBUILT 2026-07-13.** The first cut was a list of raw byte boxes under headings called Who,
 * Where and When, and Twilight took it apart -- rightly. What changed, and why, is written up in
 * MapModel::npcFields (the schema) and SpriteField.qml (the controls). The short version:
 *
 *   * every field declares a **kind**, and the kind picks the control -- a picture is a grid of
 *     ARTWORK, an X/Y pair is ONE control, a countdown is drawn as a countdown;
 *   * the raw number box appears **only when the combo cannot say the value**;
 *   * the "Talking to it" group **changes shape with the sprite**: the item picker exists for an
 *     item ball and the trainer roster for a trainer, because the save's own bits say which it is;
 *   * a byte the console recomputes on load wears a yellow **!**;
 *   * the panel's paragraph is a **?** in the title bar, and the ✕ is a **Delete** button that says
 *     Delete.
 *
 * @see notes/plans/map-screen.md -> Phase 4d
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: details
  objectName: "mapDetails"   // the DEBUG harness reads the two diag properties below

  /// The map canvas, so we know what is selected. Handed over by MapDock (a Loader's content
  /// cannot see the ids around it), so it is briefly null before that happens.
  property var canvas: null

  /// Harness-visible: how many field rows the panel THINKS it has, and for whom. A panel that is
  /// empty when it shouldn't be is otherwise very hard to tell apart from one that is correct.
  readonly property int diagFieldCount: (details.fields || []).length
  readonly property int diagSlot: details.slot

  readonly property int slot: canvas ? canvas.selectedNpc : -1
  readonly property bool hasSprite: slot > 0

  /// The selected DOOR, or -1. One selection at a time -- the canvas enforces it, so `hasDoor` and
  /// `hasSprite` can never both be true.
  readonly property int door: canvas ? canvas.selectedWarp : -1
  readonly property bool hasDoor: door >= 0

  readonly property var doorData: {
    details.revision;
    return details.hasDoor ? brg.map.warpAt(details.door) : ({});
  }

  readonly property var doorFields: {
    details.revision;
    return details.hasDoor ? brg.map.warpFields(details.door) : [];
  }

  /// The selected SIGN, or -1. One selection at a time -- the canvas enforces it, so `hasSign` can
  /// never be true alongside `hasDoor` or `hasSprite`.
  readonly property int sign: canvas ? canvas.selectedSign : -1
  readonly property bool hasSign: sign >= 0

  readonly property var signData: {
    details.revision;
    return details.hasSign ? brg.map.signAt(details.sign) : ({});
  }

  readonly property var signFieldsData: {
    details.revision;
    return details.hasSign ? brg.map.signFields(details.sign) : [];
  }

  /// The selected edge CONNECTION's direction (0-3), or -1. One selection at a time, canvas-enforced.
  readonly property int connection: canvas ? canvas.selectedConnection : -1
  readonly property bool hasConnection: connection >= 0

  readonly property var connEdge: {
    details.revision;
    if (!details.hasConnection) return ({});
    const l = brg.map.connectionEditList();
    for (let i = 0; i < l.length; i++)
      if (l[i].dir === details.connection) return l[i];
    return ({});
  }

  readonly property var connFieldsData: {
    details.revision;
    return details.hasConnection ? brg.map.connectionFields(details.connection) : [];
  }

  /// The break-sync switch (the power path). Reset whenever the selection changes -- a fresh connection
  /// starts synced (its raw fields read-only) until you deliberately break it. A connection that is
  /// ALREADY desynced (raw-edited) shows its fields editable regardless.
  property bool connBreakSync: false
  onConnectionChanged: details.connBreakSync = false

  readonly property bool connRawEditable: details.connBreakSync
                                        || (details.hasConnection && details.connEdge.synced === false)

  // (The map's warp STATE lives in its own right-dock panel -- @see WarpStatePanel.qml.)

  /// The player is slot 0. He is selectable and draggable like anybody else (Twilight, 2026-07-13),
  /// but he has no NPC record -- his bytes live in the save's player block, not the sprite table --
  /// so he gets his own short list rather than an empty one.
  readonly property bool hasPlayer: slot === 0

  /// The panel's "?". MapDock puts it in the title bar; it is the one tooltip icon this panel gets.
  readonly property string panelInfo: qsTr(
    "Everything the save holds about whoever is selected — and it is genuinely everything, including "
    + "the values no real game would ever write. Nothing here is refused and nothing is quietly "
    + "corrected.\n\n"
    + "A yellow ! means the game works that value out again when it loads your save: real bytes, "
    + "yours to set, but they won't survive Continue.\n\n"
    + "Nothing selected? Then this is the map itself.")

  // ⚠️ BINDINGS, not an imperative refresh().
  //
  // The first version called a refresh() from `onSlotChanged` + `Component.onCompleted`, and it
  // silently never ran: the panel is created by a Loader with `canvas` still null (so slot is -1),
  // and MapDock hands the canvas over in `onLoaded` -- AFTER completion. The panel came up
  // permanently blank while every C++ test passed, because the C++ was fine and the QML never
  // asked it anything. Bind it and the question gets asked whenever the answer could have changed.
  //
  // `revision` is what makes an EDIT re-ask: the model's own values change under us, and a binding
  // on npcAt(slot) alone would not know that.
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { details.revision++; }

    // A door moving / being re-aimed does NOT emit changed() (that would re-render the whole map
    // image). It gets its own signal, and the panel has to listen to it or it goes stale mid-drag.
    function onWarpsChanged() { details.revision++; }

    // Signs, same story -- their own signal, or the panel goes stale mid-drag.
    function onSignsChanged() { details.revision++; }
  }

  readonly property var sprite: {
    details.revision;   // a dependency, deliberately
    return details.hasSprite ? brg.map.npcAt(details.slot) : ({});
  }

  readonly property var fields: {
    details.revision;
    return details.hasSprite ? brg.map.npcFields(details.slot) : [];
  }

  // ⚠️ These strings must match MapModel::npcFields' group names EXACTLY -- they are what the rows
  // are filtered by. (Was Who / Where / When, which Twilight called "really really dumb", and she is
  // right: they told you nothing about what was under them.)
  readonly property var groupOrder: ["Character", "Where", "Movement", "Talking to it",
                                     "Right now", "The drawing"]

  // The PLAYER's 26-byte map-state block (@see MapModel::playerFields). Bound on `revision` so an
  // edit re-asks -- the same reason the sprite fields are. The durable groups show always; the ten
  // the game rewrites on load and the three it never reads are in the last group, filtered out by
  // the model unless the toolbar's "Useless edits" toggle (the "!") is on.
  readonly property var playerFields: {
    details.revision;
    return details.hasPlayer ? brg.map.playerFields() : [];
  }

  // ⚠️ Must match MapModel::playerFields' group names EXACTLY.
  readonly property var playerGroupOrder: ["Facing & movement", "Fine position",
                                           "What he can do here", "Battle", "Standing on",
                                           "Rewritten on load, or never read"]

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth          // never scroll sideways; the rows wrap instead
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // ⚠️ RESERVE THE SCROLLBAR'S LANE -- 16px. The bar is an overlay, so full-width content ends up
    // underneath it. Here that put the yellow "!" icons (right-anchored on each field's label row)
    // behind the scrollbar. See ui-patterns.md; it is a recurring gotcha and this is the fix.
    ColumnLayout {
      objectName: "detailsContent"   // the DEBUG harness full-length-shots the panel via this
      width: scroller.availableWidth - 22
      spacing: 8

      // ── Nothing selected: the MAP's own details ───────────────────────────────────────────
      //
      // The panel is never blank. Editing "what this map is" has one home, and this is it.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6
        visible: !details.hasSprite && !details.hasPlayer && !details.hasDoor && !details.hasSign
                 && !details.hasConnection

        Label {
          text: brg.map.mapName
          font.bold: true
          font.pixelSize: 15
          Layout.fillWidth: true
          elide: Text.ElideRight
        }

        Label {
          Layout.fillWidth: true
          text: qsTr("Nothing selected — click somebody, a warp or a sign to edit them.")
          wrapMode: Text.Wrap
          opacity: 0.6
          font.pixelSize: 11
        }

        MapDetailRow { label: qsTr("Map id");     value: String(brg.map.mapInd) }
        MapDetailRow { label: qsTr("Size");       value: qsTr("%1 × %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh) }
        MapDetailRow { label: qsTr("Tileset");    value: brg.map.tilesetName }
        MapDetailRow { label: qsTr("Palette");    value: brg.map.contrastName }
        MapDetailRow { label: qsTr("Player at");  value: qsTr("%1, %2").arg(brg.map.playerX).arg(brg.map.playerY) }
        MapDetailRow { label: qsTr("People");     value: qsTr("%1 of 15").arg(15 - brg.map.npcRoomLeft()) }
        MapDetailRow { label: qsTr("Warps");      value: qsTr("%1 of 32").arg(32 - brg.map.warpRoomLeft()) }
        MapDetailRow { label: qsTr("Signs");      value: qsTr("%1 of 16").arg(16 - brg.map.signRoomLeft()) }

        // ── The post-battle wild-encounter cooldown (wStatusFlags2 bit 0) ─────────────────────
        //
        // The one encounter flag briefed for this page (2026-07-15). It is DURABLE -- the console
        // keeps it across a Continue (verified: scripts/emu/probe_wild_encounter_cooldown.py), so it
        // wears no yellow "!": an edit sticks, and the game acts on it the moment the save loads.
        // See notes/reference/wild-encounter-cooldown.md.
        ColumnLayout {
          Layout.fillWidth: true
          Layout.topMargin: 8
          spacing: 4

          RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
              Layout.fillWidth: true
              text: qsTr("3-step wild encounter cooldown")
              font.pixelSize: 12
              wrapMode: Text.Wrap
            }

            MapSwitch {
              checked: brg.map.wildEncounterCooldown
              onToggled: brg.map.wildEncounterCooldown = !brg.map.wildEncounterCooldown
            }
          }

          Label {
            Layout.fillWidth: true
            text: qsTr("Gives 3 encounter-free steps when the save loads — the game's post-battle "
                       + "cooldown. Normally set automatically right after a battle, and it clears "
                       + "itself once you've walked those steps off.")
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
          }
        }

        // ══ MAP STATE — v1's "Map" page (the AreaMap leftover bytes) ═══════════════════════════
        //
        // ⚠️ notes/reference/area-map-state.md. Two durable levers (script step + run-on-load, always
        // on bike), one derived value kept in sync by default (the camera), and two reset-on-load
        // scratch fields behind the "Useless edits" toggle. Every value full-range, hack included.
        Rectangle { Layout.fillWidth: true; Layout.topMargin: 6; height: 1; color: brg.settings.dividerColor }

        ColumnLayout {
          id: areaState
          Layout.fillWidth: true
          Layout.topMargin: 4
          spacing: 6

          property bool rawScript: false   // the script "Something else…" disclosure

          Label {
            text: qsTr("Map state")
            font.bold: true
            font.pixelSize: 12
            Layout.fillWidth: true
          }

          // ── The PROGRESSION STATE — the researched stages of this map's story ────────────────
          //
          // Fed by the map-state blueprints (notes/reference/map-states.md): resting stages read
          // "1. First ambush armed", genuine branches "2a."/"2b.", and the transient cutscene
          // values are SHOWN too (leadership: "if it's a valid option it needs to be shown"),
          // flagged as mid-cutscene. Picking one applies the stage's WHOLE save block (script
          // byte + events + this map's missables + badges); ◀ ▶ roll one stage at a time.
          Label {
            Layout.fillWidth: true
            Layout.topMargin: 2
            visible: brg.map.hasStateBlueprint(-1)
            text: qsTr("Progression state")
            font.pixelSize: 11
            color: brg.settings.textColorMid
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: brg.map.hasStateBlueprint(-1)

            ComboBox {
              id: stateCombo
              objectName: "progressionStateCombo"   // the DEBUG harness scrolls/reads this
              Layout.fillWidth: true
              Layout.preferredHeight: 30
              font.pixelSize: 12
              textRole: "label"
              valueRole: "id"
              model: {
                details.revision;
                // No "Custom / not recognized" row (leadership, 2026-07-19): the model's
                // currentStateId() always determines a best stage from the dead-giveaway
                // flags and/or the current map script — the list is only real states
                // (researched stages + every raw step value, synthesized "s<value>").
                const states = brg.map.stateList(-1);
                let out = [];
                for (let i = 0; i < states.length; i++) {
                  const s = states[i];
                  if (s.kind === "step") {  // synthesized raw step ("s<value>")
                    out.push({ id: s.id, label: qsTr("Step %1 — %2").arg(s.script).arg(s.name),
                               desc: s.desc, kind: s.kind });
                    continue;
                  }
                  const tag = s.kind === "transient" ? qsTr(" (mid-cutscene)")
                            : s.derived ? qsTr(" (derived)") : "";
                  out.push({ id: s.id, label: s.id + ". " + s.name + tag,
                             desc: s.desc, kind: s.kind });
                }
                return out;
              }
              currentIndex: {
                details.revision;
                const cur = brg.map.currentStateId(-1);
                const l = model;
                for (let i = 0; i < l.length; i++)
                  if (l[i].id === cur) return i;
                return -1;
              }
              onActivated: {
                if (currentValue !== "" && currentValue !== undefined)
                  brg.map.applyState(currentValue, -1);
              }
              delegate: ItemDelegate {
                required property var modelData
                width: parent ? parent.width : 0
                contentItem: RowLayout {
                  spacing: 6
                  Text {
                    Layout.fillWidth: true
                    text: modelData.label
                    font.pixelSize: 12
                    font.italic: modelData.kind === "transient" || modelData.kind === "step"
                    color: brg.settings.textColorDark
                    elide: Text.ElideRight
                  }
                }
              }
            }

            Button {
              id: rollBackBtn
              Layout.preferredWidth: 30
              Layout.preferredHeight: 30
              text: "◀"
              font.pixelSize: 10
              onClicked: brg.map.rollBack(-1)
              MapToolTip {
                shown: rollBackBtn.hovered
                text: qsTr("Roll this map back one progression stage")
              }
            }
            Button {
              id: rollFwdBtn
              Layout.preferredWidth: 30
              Layout.preferredHeight: 30
              text: "▶"
              font.pixelSize: 10
              onClicked: brg.map.rollForward(-1)
              MapToolTip {
                shown: rollFwdBtn.hovered
                text: qsTr("Roll this map forward one progression stage")
              }
            }
          }

          // What the selected state MEANS — the stage's own story description.
          Label {
            Layout.fillWidth: true
            visible: brg.map.hasStateBlueprint(-1) && text !== ""
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: stateCombo.currentIndex >= 0 && stateCombo.model[stateCombo.currentIndex] !== undefined
                  ? stateCombo.model[stateCombo.currentIndex].desc : ""
          }

          // ── Current state step (the raw script byte) + run-on-load ───────────────────────────
          Label {
            Layout.fillWidth: true
            Layout.topMargin: 2
            text: qsTr("Current state step")
            font.pixelSize: 11
            color: brg.settings.textColorMid
          }

          // Descriptive picker when this map has named steps…
          ComboBox {
            id: curScriptCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            font.pixelSize: 12
            visible: brg.map.mapHasScriptList && !areaState.rawScript
            model: { details.revision; return brg.map.mapScriptList(); }
            textRole: "name"
            valueRole: "value"
            currentIndex: {
              details.revision;
              const l = model;
              for (let i = 0; i < l.length; i++)
                if (l[i].value === brg.map.mapScript) return i;
              return -1;
            }
            onActivated: brg.map.mapScript = currentValue

            delegate: ItemDelegate {
              required property var modelData
              width: parent ? parent.width : 0
              contentItem: RowLayout {
                spacing: 6
                Text {
                  Layout.fillWidth: true
                  text: modelData.name
                  font.pixelSize: 12
                  color: brg.settings.textColorDark
                  elide: Text.ElideRight
                }
                Text {
                  visible: modelData.hack === true
                  text: qsTr("raw")
                  font.pixelSize: 9
                  color: "#d55e00"
                }
              }
            }
          }

          // What the selected step MEANS -- the progression description (same words as the
          // Map Storage panel's per-map script dropdown; from maps.json scriptEntries desc).
          Label {
            Layout.fillWidth: true
            visible: curScriptCombo.visible && text !== ""
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
            text: {
              details.revision;
              const l = brg.map.mapScriptList();
              for (let i = 0; i < l.length; i++)
                if (l[i].value === brg.map.mapScript)
                  return l[i].desc !== undefined ? l[i].desc : "";
              return "";
            }
          }

          // …a raw index otherwise, or behind "Something else…".
          RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: !brg.map.mapHasScriptList || areaState.rawScript
            Label { text: qsTr("Step"); font.pixelSize: 10; opacity: 0.6 }
            SpinBox {
              Layout.fillWidth: true
              Layout.preferredHeight: 28
              font.pixelSize: 11
              editable: true
              from: 0
              to: 255
              value: brg.map.mapScript
              onValueModified: brg.map.mapScript = value
            }
          }

          // The "Something else…" link only when there IS a named list to step out of.
          Label {
            visible: brg.map.mapHasScriptList
            text: areaState.rawScript ? qsTr("Pick from the list") : qsTr("Something else…")
            font.pixelSize: 10
            color: brg.settings.accentColor
            MouseArea {
              anchors.fill: parent
              cursorShape: Qt.PointingHandCursor
              onClicked: areaState.rawScript = !areaState.rawScript
            }
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
              Layout.fillWidth: true
              text: qsTr("Run this state step on load")
              font.pixelSize: 12
              wrapMode: Text.Wrap
            }
            MapSwitch {
              checked: brg.map.runScriptOnLoad
              onToggled: brg.map.runScriptOnLoad = !brg.map.runScriptOnLoad
            }
          }
          Label {
            Layout.fillWidth: true
            text: qsTr("Runs the step above on the next map load instead of the map's default. On a "
                       + "scripted map the game consumes it on the first tick; on a quiet map it sticks.")
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
          }

          // ── Always on bike ───────────────────────────────────────────────────────────────────
          RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8
            Label {
              Layout.fillWidth: true
              text: qsTr("Always on bike (Cycling Road)")
              font.pixelSize: 12
              wrapMode: Text.Wrap
            }
            MapSwitch {
              checked: brg.map.alwaysOnBike
              onToggled: brg.map.alwaysOnBike = !brg.map.alwaysOnBike
            }
          }

          // ── Camera / view box (derived, synced by default) ───────────────────────────────────
          RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8
            Label {
              Layout.fillWidth: true
              text: brg.map.viewSynced ? qsTr("Camera — follows the player")
                                       : qsTr("Camera — set loose")
              font.pixelSize: 12
              wrapMode: Text.Wrap
            }
            MapSwitch {
              // On = broken loose. Flipping it toggles sync; re-attaching snaps the box to the player.
              checked: !brg.map.viewSynced
              onToggled: brg.map.setViewBreakSync(brg.map.viewSynced)
            }
          }
          // The raw pointer, only on the power path.
          ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            visible: !brg.map.viewSynced
            RowLayout {
              Layout.fillWidth: true
              spacing: 6
              Label { text: qsTr("Address"); font.pixelSize: 10; opacity: 0.6 }
              SpinBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                font.pixelSize: 11
                editable: true
                from: 0
                to: 65535
                value: brg.map.viewPtr
                onValueModified: brg.map.setViewPtr(value)
              }
            }
            Label {
              Layout.fillWidth: true
              text: qsTr("The game trusts this pointer and draws the screen from it — an off-map value "
                         + "shows garbage. You can also drag the view box around on the canvas.")
              wrapMode: Text.Wrap
              font.pixelSize: 10
              opacity: 0.55
            }
          }
          Label {
            Layout.fillWidth: true
            visible: brg.map.viewSynced
            text: qsTr("The view box tracks the player automatically. Break it loose to place it by "
                       + "hand.")
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
          }

          // ── Reset-on-load scratch, behind the "Useless edits" toggle ────────────────────────
          ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 4
            visible: brg.map.showScratch

            Rectangle {
              Layout.fillWidth: true
              radius: 6
              color: Qt.rgba(0, 0, 0, 0.03)
              border.width: 1
              border.color: brg.settings.dividerColor
              implicitHeight: asHdr.implicitHeight + 14
              RowLayout {
                id: asHdr
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6
                MapWarnIcon {
                  text: qsTr("The game works this value out again — or clears it — the moment it loads "
                             + "your save. Verified on a real cartridge.")
                }
                Label {
                  Layout.fillWidth: true
                  text: qsTr("Rewritten on load — the game resets these every time")
                  wrapMode: Text.Wrap
                  font.pixelSize: 10
                  opacity: 0.7
                }
              }
            }

            // Screen VRAM pointer → $9800
            RowLayout {
              Layout.fillWidth: true
              spacing: 6
              Label { Layout.preferredWidth: 110; text: qsTr("Screen VRAM ptr"); font.pixelSize: 10; opacity: 0.7; elide: Text.ElideRight }
              SpinBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                font.pixelSize: 10
                editable: true
                from: 0
                to: 65535
                value: brg.map.vramViewPtr
                onValueModified: brg.map.vramViewPtr = value
              }
            }
            Label { Layout.fillWidth: true; text: qsTr("Reset to $9800 on load."); font.pixelSize: 9; opacity: 0.5 }

            // Card-Key door X / Y → 0
            RowLayout {
              Layout.fillWidth: true
              spacing: 6
              Label { Layout.preferredWidth: 110; text: qsTr("Card-Key door X"); font.pixelSize: 10; opacity: 0.7; elide: Text.ElideRight }
              SpinBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                font.pixelSize: 10
                editable: true
                from: 0
                to: 255
                value: brg.map.cardKeyDoorX
                onValueModified: brg.map.cardKeyDoorX = value
              }
            }
            RowLayout {
              Layout.fillWidth: true
              spacing: 6
              Label { Layout.preferredWidth: 110; text: qsTr("Card-Key door Y"); font.pixelSize: 10; opacity: 0.7; elide: Text.ElideRight }
              SpinBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                font.pixelSize: 10
                editable: true
                from: 0
                to: 255
                value: brg.map.cardKeyDoorY
                onValueModified: brg.map.cardKeyDoorY = value
              }
            }
            Label { Layout.fillWidth: true; text: qsTr("Zeroed on load (Silph Co. door scratch)."); font.pixelSize: 9; opacity: 0.5 }
          }

          Label {
            Layout.fillWidth: true
            Layout.topMargin: 2
            visible: !brg.map.showScratch
            text: qsTr("Three more bytes here do nothing you can keep — the game resets them on load. "
                       + "Turn on “Useless edits” (the ! button) in the toolbar to see them.")
            wrapMode: Text.Wrap
            font.pixelSize: 10
            opacity: 0.55
          }
        }

        // The view pointer the GAME itself computed and left in the save. If an edit has made it
        // stale we say so plainly and offer the one-click fix -- we never quietly rewrite it.
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 6
          visible: !brg.map.headerMatches
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: fixCol.implicitHeight + 16

          ColumnLayout {
            id: fixCol
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("This map's header no longer matches what the game would compute.")
              wrapMode: Text.Wrap
              font.pixelSize: 11
            }

            Button {
              text: qsTr("Fix it")
              onClicked: brg.map.fixMapHeader()
            }
          }
        }

        // (⇄ WARP STATE used to be appended here, at the bottom of the map's own details. It is now
        //  its OWN PANEL, in the RIGHT dock -- which is where Twilight asked for it (2026-07-14: "I
        //  will place them in the right panel as warp state") and which is simply better: down here
        //  it sat below the fold, behind a scroll past six rows of map facts. The right dock is
        //  where the things you edit ABOUT THE MAP live; the Details panel is for what is SELECTED.
        //  @see WarpStatePanel.qml)
      }

      // ══ ⇄ A DOOR SELECTED ══════════════════════════════════════════════════════════════════
      //
      // ⚠️ **An edited door is genuinely LIVE.** `LoadMainData` sets BIT_NO_PREVIOUS_MAP on the saved
      // tileset byte, so the next `LoadMapHeader` bails out before it rebuilds the warp list from
      // ROM. Verified on the cartridge -- including a 4th door invented in a 3-door town.
      //
      // And the game puts the map's original doors back the moment the player leaves and walks in
      // again. That is the cartridge's behaviour, not our gap, and the panel SAYS it -- once the user
      // has actually made an edit it applies to. (Same rule as the cast: we track the EDIT, never a
      // diff against the ROM.) See notes/reference/warps.md §1.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasDoor

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Rectangle {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: 4
            color: "#66f0e442"
            border.width: 1
            border.color: "#f0e442"

            Text {
              anchors.centerIn: parent
              text: "⇄"
              font.pixelSize: 15
              color: "#212121"
            }
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              Layout.fillWidth: true
              text: qsTr("Warp %1").arg(details.door)
              font.bold: true
              font.pixelSize: 14
              elide: Text.ElideRight
            }

            Label {
              Layout.fillWidth: true
              text: details.doorData.destName || ""
              font.pixelSize: 10
              opacity: 0.6
              elide: Text.ElideRight
            }
          }
        }

        // Where it goes, resolved — the one thing about a door that matters, said in full.
        Label {
          Layout.fillWidth: true
          visible: (details.doorData.destLabel || "") !== ""
          text: details.doorData.destLabel || ""
          wrapMode: Text.Wrap
          font.pixelSize: 11
          color: brg.settings.textColorMid
        }

        // 🔫 It points at an arrival point the target map does not have. The console would copy four
        // arbitrary ROM bytes into the view pointer and the player's coordinates. Shown, explained,
        // never refused.
        Rectangle {
          Layout.fillWidth: true
          visible: details.hasDoor && !details.doorData.destValid
          radius: 6
          color: Qt.rgba(0.84, 0.37, 0, 0.12)
          border.width: 1
          border.color: "#d55e00"
          implicitHeight: badDest.implicitHeight + 14

          Label {
            id: badDest
            anchors.fill: parent
            anchors.margins: 7
            wrapMode: Text.Wrap
            font.pixelSize: 10
            text: qsTr("This warp leads to arrival point %1 of %2 — but that map only has %3.\n\n"
                       + "The game doesn't check. It will read whatever cartridge bytes happen to "
                       + "come after the list and drop the player somewhere undefined.")
                  .arg(details.doorData.destWarp || 0)
                  .arg(details.doorData.destName || "")
                  .arg(details.doorData.arrivalCount || 0)
          }
        }

        Button {
          Layout.fillWidth: true
          Layout.topMargin: 2
          Layout.preferredHeight: 28
          font.pixelSize: 11

          text: qsTr("Delete this warp")

          contentItem: Label {
            text: parent.text
            font: parent.font
            color: parent.hovered ? "#ffffff" : "#d55e00"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }

          background: Rectangle {
            radius: 4
            color: parent.hovered ? "#d55e00" : "transparent"
            border.width: 1
            border.color: "#d55e00"

            Behavior on color { ColorAnimation { duration: 90 } }
          }

          onClicked: {
            brg.map.removeWarp(details.door);
            if (details.canvas)
              details.canvas.selectedWarp = -1;
          }
        }

        Repeater {
          model: details.doorFields

          delegate: WarpField {
            required property var modelData
            Layout.fillWidth: true

            fieldData: modelData
            ind: details.door
          }
        }

        // ── The honest note ────────────────────────────────────────────────────────────────
        //
        // It appears ONLY once the user has actually changed a door -- the same rule as the cast, and
        // for the same reason: a notice that fires on every save anybody ever opens is noise, and
        // noise is a bug.
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 8
          visible: brg.map.warpsEdited()
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: liveNote.implicitHeight + 14

          Label {
            id: liveNote
            anchors.fill: parent
            anchors.margins: 7
            wrapMode: Text.Wrap
            font.pixelSize: 10
            text: qsTr("These warps are live — the game will use them when this save loads.\n\n"
                       + "It puts the map's original warps back as soon as the player walks out of "
                       + "this map and back in again. That's the game's own behaviour, not a limit "
                       + "of the editor.")
          }
        }
      }

      // ══ ▤ A SIGN SELECTED ══════════════════════════════════════════════════════════════════
      //
      // ⚠️ **An edited sign is genuinely LIVE**, on the same linchpin as a door (`.loadSignData` sits
      // inside `LoadMapHeader`, behind BIT_NO_PREVIOUS_MAP). The game puts the map's original signs
      // back the moment the player leaves and walks in again -- said in words below, once the user has
      // actually made an edit. See notes/reference/signs.md.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasSign

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Rectangle {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: 4
            color: "#66e69f00"
            border.width: 1
            border.color: "#e69f00"

            Text {
              anchors.centerIn: parent
              text: "▤"
              font.pixelSize: 15
              color: "#212121"
            }
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              Layout.fillWidth: true
              text: qsTr("Sign %1").arg(details.sign)
              font.bold: true
              font.pixelSize: 14
              elide: Text.ElideRight
            }

            Label {
              Layout.fillWidth: true
              text: qsTr("at %1, %2").arg(details.signData.x || 0).arg(details.signData.y || 0)
              font.pixelSize: 10
              opacity: 0.6
              elide: Text.ElideRight
            }
          }
        }

        // What it says, resolved — the whole point of a sign, shown in full (several lines allowed).
        Label {
          Layout.fillWidth: true
          visible: (details.signData.preview || "") !== "" && (details.signData.textValid === true)
          text: qsTr("“%1”").arg(details.signData.preview || "")
          wrapMode: Text.Wrap
          font.pixelSize: 11
          color: brg.settings.textColorMid
        }

        // 🔫 The text id points past this map's text table. The game reads whatever comes next.
        // Shown, explained, never refused.
        Rectangle {
          Layout.fillWidth: true
          visible: details.hasSign && (details.signData.textValid === false)
          radius: 6
          color: Qt.rgba(0.84, 0.37, 0, 0.12)
          border.width: 1
          border.color: "#d55e00"
          implicitHeight: badText.implicitHeight + 14

          Label {
            id: badText
            anchors.fill: parent
            anchors.margins: 7
            wrapMode: Text.Wrap
            font.pixelSize: 10
            text: qsTr("This sign's text id isn't one this map has — the game will read whatever text "
                       + "comes next in the cartridge.\n\nIt's still yours to set: pick from the map's "
                       + "text below, or keep the raw id.")
          }
        }

        Button {
          Layout.fillWidth: true
          Layout.topMargin: 2
          Layout.preferredHeight: 28
          font.pixelSize: 11

          text: qsTr("Delete this sign")

          contentItem: Label {
            text: parent.text
            font: parent.font
            color: parent.hovered ? "#ffffff" : "#d55e00"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }

          background: Rectangle {
            radius: 4
            color: parent.hovered ? "#d55e00" : "transparent"
            border.width: 1
            border.color: "#d55e00"

            Behavior on color { ColorAnimation { duration: 90 } }
          }

          onClicked: {
            brg.map.removeSign(details.sign);
            if (details.canvas)
              details.canvas.selectedSign = -1;
          }
        }

        Repeater {
          model: details.signFieldsData

          delegate: SignField {
            required property var modelData
            Layout.fillWidth: true

            fieldData: modelData
            ind: details.sign
          }
        }

        // ── The honest note (same rule, same words, as the doors) ─────────────────────────────
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 8
          visible: brg.map.signsEdited()
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: signLiveNote.implicitHeight + 14

          Label {
            id: signLiveNote
            anchors.fill: parent
            anchors.margins: 7
            wrapMode: Text.Wrap
            font.pixelSize: 10
            text: qsTr("These signs are live — the game will use them when this save loads.\n\n"
                       + "It puts the map's original signs back as soon as the player walks out of "
                       + "this map and back in again. That's the game's own behaviour, not a limit "
                       + "of the editor.")
          }
        }
      }

      // ══ 🔗 A CONNECTION SELECTED ═══════════════════════════════════════════════════════════
      //
      // A connection is neighbour + one signed OFFSET; the other nine bytes are macro-derived. So the
      // top of the panel is those two real inputs, and the raw nine live below, read-only while synced
      // and unlocked by the break-sync switch (the power path). See notes/reference/map-connections.md.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasConnection

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Rectangle {
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: 4
            color: "#66d55e00"
            border.width: 1
            border.color: "#d55e00"
            Text { anchors.centerIn: parent; text: "🔗"; font.pixelSize: 14 }
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Label {
              Layout.fillWidth: true
              text: qsTr("%1 connection").arg(details.connEdge.dirName || "")
              font.bold: true
              font.pixelSize: 14
              elide: Text.ElideRight
            }
            Label {
              Layout.fillWidth: true
              text: (details.connEdge.synced === false)
                    ? qsTr("Raw-edited — the offset no longer describes it")
                    : qsTr("to %1 · offset %2").arg(details.connEdge.toName || "").arg(details.connEdge.offset || 0)
              font.pixelSize: 10
              opacity: 0.6
              elide: Text.ElideRight
            }
          }
        }

        // ── Neighbour ──────────────────────────────────────────────────────────────────────
        Label { text: qsTr("Connects to"); font.pixelSize: 11; color: brg.settings.textColorMid }
        ComboBox {
          id: connMap
          Layout.fillWidth: true
          Layout.preferredHeight: 30
          font.pixelSize: 12
          model: details.hasConnection ? brg.map.connectionMapList(details.connection) : []
          textRole: "name"
          valueRole: "value"
          currentIndex: {
            details.revision;
            const list = model;
            for (let i = 0; i < list.length; i++)
              if (list[i].value === details.connEdge.toMap) return i;
            return -1;
          }
          onActivated: brg.map.setConnectionMap(details.connection, currentValue)

          // Default ★ first, then the maps that fit this edge, then the rest; size grey on the right.
          delegate: ItemDelegate {
            required property var modelData
            required property int index
            width: connMap.width
            height: (modelData.group !== "" ? 20 : 0) + 26
            highlighted: connMap.highlightedIndex === index

            contentItem: ColumnLayout {
              spacing: 0
              Text {
                visible: modelData.group !== ""
                Layout.fillWidth: true
                text: modelData.group
                font.pixelSize: 10; font.bold: true
                color: brg.settings.textColorMid
              }
              RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                  visible: modelData.isDefault === true
                  text: "★"; font.pixelSize: 11; color: "#e69f00"
                }
                Text {
                  Layout.fillWidth: true
                  text: modelData.name
                  font.pixelSize: 12
                  font.bold: modelData.isDefault === true
                  color: brg.settings.textColorDark
                  elide: Text.ElideRight
                }
                Text {
                  text: modelData.size
                  font.pixelSize: 10; font.family: "monospace"
                  color: brg.settings.textColorMid
                }
              }
            }
          }
        }

        // ── Offset (the one real knob) ───────────────────────────────────────────────────────
        RowLayout {
          Layout.fillWidth: true
          spacing: 6
          Label { text: qsTr("Offset"); font.pixelSize: 11; color: brg.settings.textColorMid }
          SpinBox {
            id: offsetSpin
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            font.pixelSize: 11
            editable: true
            from: details.connEdge.offsetMin !== undefined ? details.connEdge.offsetMin : -32
            to: details.connEdge.offsetMax !== undefined ? details.connEdge.offsetMax : 32
            value: details.connEdge.offset || 0
            onValueModified: brg.map.setConnectionOffset(details.connection, value)
          }
        }

        // The snap landmarks, as one-tap buttons.
        Flow {
          Layout.fillWidth: true
          spacing: 6
          Repeater {
            model: details.connEdge.snaps || []
            delegate: Button {
              required property var modelData
              height: 24
              font.pixelSize: 10
              text: modelData.name + " (" + modelData.offset + ")"
              onClicked: brg.map.setConnectionOffset(details.connection, modelData.offset)
            }
          }
        }

        // ── Attach to another edge (re-home; never a rotation) ───────────────────────────────
        Label {
          Layout.fillWidth: true
          Layout.topMargin: 4
          text: qsTr("Attached edge")
          font.pixelSize: 11
          color: brg.settings.textColorMid
        }
        RowLayout {
          Layout.fillWidth: true
          spacing: 4
          Repeater {
            model: [{ d: 0, n: qsTr("North") }, { d: 1, n: qsTr("South") },
                    { d: 2, n: qsTr("East") },  { d: 3, n: qsTr("West") }]
            delegate: Button {
              required property var modelData
              Layout.fillWidth: true
              Layout.preferredHeight: 26
              font.pixelSize: 10
              text: modelData.n
              // The current edge is highlighted; a free edge is a re-home target; an occupied one is off.
              enabled: modelData.d === details.connection
                       || !brg.map.connectionExists(modelData.d)
              highlighted: modelData.d === details.connection
              onClicked: {
                if (modelData.d === details.connection) return;
                brg.map.rehomeConnection(details.connection, modelData.d);
                if (details.canvas) details.canvas.selectedConnection = modelData.d;
              }
            }
          }
        }

        // ── The raw nine (read-only while synced; break sync to edit) ────────────────────────
        RowLayout {
          Layout.fillWidth: true
          Layout.topMargin: 6
          spacing: 6
          Label {
            Layout.fillWidth: true
            text: qsTr("Raw bytes")
            font.pixelSize: 11
            font.bold: true
            opacity: 0.55
          }
          Switch {
            text: qsTr("Break sync")
            font.pixelSize: 10
            checked: details.connRawEditable
            enabled: details.connEdge.synced !== false   // already desynced: always editable, switch moot
            onToggled: details.connBreakSync = checked
          }
        }

        Label {
          Layout.fillWidth: true
          text: details.connRawEditable
                ? qsTr("Editing these sets bytes directly — the offset above no longer describes the "
                       + "connection until you pick a neighbour or offset again.")
                : qsTr("These follow the offset above. Turn on “Break sync” to set them by hand.")
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
        }

        Repeater {
          model: details.connFieldsData
          delegate: RowLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: 6

            Label {
              Layout.preferredWidth: 92
              text: modelData.label
              font.pixelSize: 10
              opacity: 0.7
              elide: Text.ElideRight
            }
            SpinBox {
              Layout.fillWidth: true
              Layout.preferredHeight: 26
              font.pixelSize: 10
              editable: true
              enabled: details.connRawEditable
              from: modelData.min
              to: modelData.max
              value: modelData.value
              onValueModified: brg.map.setConnectionField(details.connection, modelData.key, value)
            }
          }
        }

        // ── Delete ───────────────────────────────────────────────────────────────────────────
        Button {
          Layout.fillWidth: true
          Layout.topMargin: 6
          Layout.preferredHeight: 28
          font.pixelSize: 11
          text: qsTr("Delete this connection")
          contentItem: Label {
            text: parent.text
            font: parent.font
            color: parent.hovered ? "#ffffff" : "#d55e00"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }
          background: Rectangle {
            radius: 4
            color: parent.hovered ? "#d55e00" : "transparent"
            border.width: 1
            border.color: "#d55e00"
            Behavior on color { ColorAnimation { duration: 90 } }
          }
          onClicked: {
            brg.map.removeConnection(details.connection);
            if (details.canvas) details.canvas.selectedConnection = -1;
          }
        }

        // ── The honest note (same rule as doors/signs) ──────────────────────────────────────
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 8
          visible: brg.map.connectionsEdited()
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: connLiveNote.implicitHeight + 14
          Label {
            id: connLiveNote
            anchors.fill: parent
            anchors.margins: 7
            wrapMode: Text.Wrap
            font.pixelSize: 10
            text: qsTr("These connections are live — the game will use them when this save loads.\n\n"
                       + "It puts the map's original connections back as soon as the player walks out "
                       + "of this map and back in again. That's the game's own behaviour, not a limit "
                       + "of the editor.")
          }
        }
      }

      // ── The PLAYER selected ──────────────────────────────────────────────────────────────
      //
      // He is slot 0, he is selectable and draggable like anybody else -- and he does not live in
      // the sprite table, so `npcFields` has nothing for him. His position is the one thing about
      // him that is genuinely a map fact, and the canvas already edits it by dragging; the rest of
      // him (name, badges, money, the lot) is the Trainer Card's job, not the map's.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasPlayer

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Image {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: "image://player/npc/1/0/" + brg.map.contrast
            smooth: false
            fillMode: Image.PreserveAspectFit
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              Layout.fillWidth: true
              text: qsTr("The player")
              font.bold: true
              font.pixelSize: 14
              elide: Text.ElideRight
            }

            Label {
              text: qsTr("Slot 0 — the game requires him")
              font.pixelSize: 10
              opacity: 0.6
            }
          }
        }

        Label {
          Layout.fillWidth: true
          Layout.topMargin: 2
          text: qsTr("Where he is standing")
          font.pixelSize: 11
          color: brg.settings.textColorMid
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 6

          Label { text: qsTr("X"); font.pixelSize: 10; opacity: 0.6 }

          SpinBox {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 28
            font.pixelSize: 11
            editable: true
            from: 0
            to: 255
            value: brg.map.playerX
            onValueModified: brg.map.movePlayer(value, brg.map.playerY)
          }

          Label { text: qsTr("Y"); font.pixelSize: 10; opacity: 0.6 }

          SpinBox {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 28
            font.pixelSize: 11
            editable: true
            from: 0
            to: 255
            value: brg.map.playerY
            onValueModified: brg.map.movePlayer(brg.map.playerX, value)
          }
        }

        // Moving him invalidates the view pointer the GAME computed. We say so and offer the fix --
        // we never quietly rewrite it. (The derived-byte doctrine; map-screen.md.)
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 6
          visible: !brg.map.headerMatches
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: pFixCol.implicitHeight + 16

          ColumnLayout {
            id: pFixCol
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("The map view pointer the game computed no longer matches where he is.")
              wrapMode: Text.Wrap
              font.pixelSize: 11
            }

            Button {
              text: qsTr("Fix it")
              onClicked: brg.map.fixMapHeader()
            }
          }
        }

        // ── Every other byte of his map state, grouped ──────────────────────────────────────
        //
        // ⚠️ Read notes/reference/player-state.md. Ten of these the game rewrites the instant it
        // loads the save, three it never reads -- all gathered in the last group, behind the
        // toolbar's "Useless edits" toggle (filtered in the MODEL, so no view can leak one). The
        // durable ones show here always. Everything full-range, hack values included, never refused.
        Repeater {
          model: details.playerGroupOrder

          delegate: ColumnLayout {
            id: playerGroup
            required property string modelData

            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 3

            readonly property var members: (details.playerFields || []).filter(function(f) {
              return f.group === playerGroup.modelData;
            })

            readonly property bool isRewriteGroup:
              playerGroup.modelData === "Rewritten on load, or never read"

            visible: playerGroup.members.length > 0

            // The durable groups get a plain heading.
            Label {
              visible: !playerGroup.isRewriteGroup
              text: playerGroup.modelData
              font.pixelSize: 11
              font.bold: true
              opacity: 0.55
              Layout.fillWidth: true
              Layout.topMargin: 4
            }

            // ⚠️ THE REWRITE GROUP IS A DIFFERENT KIND OF GROUP, so it says so rather than being a
            // plain heading -- exactly like the warp panel's "Fields that do nothing". Twilight,
            // 2026-07-14: *"which ones were regenerated or rewritten on save load with little
            // exclamation points grouped below and hidden behind a switch."*
            Rectangle {
              visible: playerGroup.isRewriteGroup
              Layout.fillWidth: true
              Layout.topMargin: 10
              radius: 6
              color: Qt.rgba(0, 0, 0, 0.03)
              border.width: 1
              border.color: brg.settings.dividerColor
              implicitHeight: rewriteHdr.implicitHeight + 16

              ColumnLayout {
                id: rewriteHdr
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Label {
                  Layout.fillWidth: true
                  text: qsTr("Rewritten on load, or never read")
                  font.pixelSize: 11
                  font.bold: true
                  opacity: 0.75
                }

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 6

                  MapWarnIcon {
                    text: qsTr("The game works this value out again the moment it loads your save. "
                               + "Verified on a real cartridge.")
                  }

                  Label {
                    Layout.fillWidth: true
                    text: qsTr("the game rewrites it every time it loads your save")
                    wrapMode: Text.Wrap
                    font.pixelSize: 10
                    opacity: 0.6
                  }
                }

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 6

                  Label {
                    Layout.preferredWidth: 14
                    text: "💀"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                  }

                  Label {
                    Layout.fillWidth: true
                    text: qsTr("it survives perfectly — and nothing in the game ever reads it")
                    wrapMode: Text.Wrap
                    font.pixelSize: 10
                    opacity: 0.6
                  }
                }
              }
            }

            Repeater {
              model: playerGroup.members

              delegate: PlayerField {
                required property var modelData
                Layout.fillWidth: true

                fieldData: modelData
              }
            }
          }
        }

        // The switch that reveals the rewrite/dead group lives in the TOOLBAR -- the same one the
        // sprite and warp panels use. Point at it rather than growing a second one.
        Label {
          Layout.fillWidth: true
          Layout.topMargin: 10
          visible: !brg.map.showScratch
          text: qsTr("Thirteen more of his bytes do nothing you can keep — the game either rewrites "
                     + "them when it loads your save, or never reads them. Turn on “Useless edits” (the ! button) "
                     + "in the toolbar to see them.")
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
        }

        Label {
          Layout.fillWidth: true
          Layout.topMargin: 8
          text: qsTr("His name, his badges and everything else about him live on the Trainer Card.")
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
        }
      }

      // ── A sprite selected ────────────────────────────────────────────────────────────────
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasSprite

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Image {
            width: 32
            height: 32
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: details.sprite.source || ""
            smooth: false
            fillMode: Image.PreserveAspectFit
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              text: details.sprite.name || ""
              font.bold: true
              font.pixelSize: 14
              Layout.fillWidth: true
              elide: Text.ElideRight
            }

            Label {
              text: qsTr("Slot %1").arg(details.slot)
              font.pixelSize: 10
              opacity: 0.6
            }
          }
        }

        // ⚠️ A DELETE BUTTON THAT SAYS DELETE.
        //
        // It was a "✕" ToolButton with a tooltip. Twilight: *"The x button deletes — it's not
        // self-explanatory, should be delete button."* A destructive action gets a word, not a glyph
        // you have to hover to identify.
        Button {
          Layout.fillWidth: true
          Layout.topMargin: 2
          Layout.preferredHeight: 28
          font.pixelSize: 11

          text: qsTr("Delete this character")

          contentItem: Label {
            text: parent.text
            font: parent.font
            color: parent.hovered ? "#ffffff" : "#d55e00"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }

          background: Rectangle {
            radius: 4
            color: parent.hovered ? "#d55e00" : "transparent"
            border.width: 1
            border.color: "#d55e00"

            Behavior on color { ColorAnimation { duration: 90 } }
          }

          onClicked: {
            brg.map.removeNpc(details.slot);
            if (details.canvas)
              details.canvas.selectedNpc = -1;
          }
        }

        // (A big yellow BLOCK saying "this map hasn't loaded this character's picture" sat here.
        //  REMOVED 2026-07-13 -- Twilight: *"don't have it also as a big yellow block on the details
        //  page."* And she is right: the picture picker two rows below already carries the yellow "!"
        //  on exactly that character, with the sentence in its tooltip. Saying it a second time, in a
        //  paragraph, in a coloured box, above the fields you came to edit, is the panel shouting.)

        // (A blue "this map's cast no longer matches the game's" notice sat here. REMOVED 2026-07-13
        // -- Twilight: "do not have notice on cast no longer matches". It fired on every edit, said
        // the same thing every time, and pushed the fields you were editing down the panel. The fact
        // it carried -- the game rebuilds a map's cast from ROM when you walk back in -- is still
        // true, and it is written down where a fact belongs: notes/reference/sprites.md, Part 6.)

        // ── Every byte, grouped ──────────────────────────────────────────────────────────────
        Repeater {
          model: details.groupOrder

          delegate: ColumnLayout {
            id: fieldGroup
            required property string modelData

            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 3

            readonly property var members: (details.fields || []).filter(function(f) {
              return f.group === fieldGroup.modelData;
            })

            visible: fieldGroup.members.length > 0

            Label {
              text: fieldGroup.modelData
              font.pixelSize: 11
              font.bold: true
              opacity: 0.55
              Layout.fillWidth: true
              Layout.topMargin: 4
            }

            Repeater {
              model: fieldGroup.members

              delegate: SpriteField {
                required property var modelData
                Layout.fillWidth: true

                fieldData: modelData
                slot: details.slot
                canvas: details.canvas   // so the picture picker can mute the ground while it is up
              }
            }
          }
        }
      }
    }
  }
}
