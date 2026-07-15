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
  // the model unless the toolbar's "Reloaded values" switch is on.
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
      width: scroller.availableWidth - 16
      spacing: 8

      // ── Nothing selected: the MAP's own details ───────────────────────────────────────────
      //
      // The panel is never blank. Editing "what this map is" has one home, and this is it.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6
        visible: !details.hasSprite && !details.hasPlayer && !details.hasDoor && !details.hasSign

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
        // toolbar's "Reloaded values" switch (filtered in the MODEL, so no view can leak one). The
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
                     + "them when it loads your save, or never reads them. Turn on “Reloaded values” "
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
