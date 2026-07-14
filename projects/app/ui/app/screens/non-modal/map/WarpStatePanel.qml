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
 * ⇄ **WARP STATE** -- the twelve bytes AROUND the doors.
 *
 * Not the doors themselves (those are objects on the canvas, edited in the Details panel when you
 * click one). This is everything the save remembers about *warping*: where the last FLY was headed,
 * which hole you fell down, whether DIG is about to fire, and whether a door needs walking into.
 *
 * It belongs to the MAP, not to any one door -- which is why it is a **right-dock panel**, with the
 * other things you edit about the map itself, and not a section of the Details panel.
 * (Twilight, 2026-07-14: *"I will place them in the right panel as warp state or something better
 * named."* The first cut appended it to the bottom of the map's own details, where it sat below the
 * fold behind a scroll past six rows of map facts. She was right.)
 *
 * ## Every byte, named in English
 *
 * v1 shipped these as **"Scripted Warp"**, **"Non-Normal Warp"**, **"Special Warp"**, **"Skip
 * Joypad"** -- invented names, no explanations, and three of them wrong. Every field here carries the
 * name of what it actually does and a sentence saying what that means, because **the app is where
 * that gets answered**. Nobody should have to already know what `wDestinationWarpID` is.
 *
 * ## The three marks (@see WarpField.qml)
 *
 * - 🔫 **the two loaded guns** -- `Fly sends you to` and the `falling drops you onto` / `hole #` pair.
 *   Their ROM lookup tables have **no bounds check** (and the fly one has no terminator either), so an
 *   illegal value makes a real console read arbitrary cartridge bytes as warp data. The pickers offer
 *   the legal values; the full range is one click away, and **never refused**.
 * - ⚠️ **wiped on load** and 💀 **never read** -- **two different facts**, gathered at the bottom
 *   behind the toolbar's "Reloaded values" switch, each wearing its own mark.
 *
 * Everything here is verified against the cartridge. @see notes/reference/warps.md
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: panel
  objectName: "mapWarpState"

  /// Handed over by MapDock (a Loader's content cannot see the ids around it).
  property var canvas: null

  /// The panel's "?" -- MapDock puts it in the title bar. The one tooltip icon this panel gets.
  readonly property string panelInfo: qsTr(
    "Everything the save remembers about warping that isn't a door itself: where the last FLY was "
    + "headed, which hole you fell down, where DIG will drop you.\n\n"
    + "These belong to the map you're standing on, not to any one door — which is why they're here "
    + "and not in a door's own panel. Click a door on the map to edit that.\n\n"
    + "A red ! means the value is one no real Game Boy has an answer for. It's still yours to set — "
    + "nothing here is refused, and nothing is rewritten behind your back.")

  /// Harness-visible: how many rows the panel thinks it has. A panel that is empty when it shouldn't
  /// be is otherwise very hard to tell apart from one that is correct.
  readonly property int diagFieldCount: (panel.fields || []).length

  // ⚠️ BINDINGS, not an imperative refresh(). The panel is built by a Loader before `canvas` is handed
  // over, so anything that runs once at completion runs too early and never again. `revision` is the
  // dependency a binding on a C++ *method* is otherwise missing. (DetailsPanel learned this the hard
  // way -- it came up permanently blank while every C++ test passed.)
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { panel.revision++; }

    // A door moving does NOT emit changed() -- that would re-render the whole map image. The warp
    // state has to listen to its own signal or it goes stale.
    function onWarpsChanged() { panel.revision++; }
  }

  readonly property var fields: {
    panel.revision;   // a dependency, deliberately
    return brg.map.warpStateFields();
  }

  /// ⚠️ These strings must match MapModel::warpStateFields' group names EXACTLY -- they are what the
  /// rows are filtered by.
  readonly property var groupOrder: ["Where the special warps go",
                                     "What kind of warp is happening",
                                     "Fields that do nothing"]

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // ⚠️ RESERVE THE SCROLLBAR'S LANE -- 16px, always. The bar is an OVERLAY, so full-width content
    // ends up underneath it, and the first thing to disappear is the right-anchored "!" icon on each
    // field's label row -- which is exactly the thing this panel exists to show you.
    // (ui-patterns.md, rule 2. It is the recurring gotcha on this project.)
    ColumnLayout {
      width: scroller.availableWidth - 16
      spacing: 6

      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6

        Label {
          Layout.fillWidth: true
          text: qsTr("What happens when you warp")
          wrapMode: Text.Wrap
          opacity: 0.6
          font.pixelSize: 11
        }

        Repeater {
          model: panel.groupOrder

          delegate: ColumnLayout {
            id: group
            required property string modelData

            Layout.fillWidth: true
            spacing: 3

            readonly property var members: (panel.fields || []).filter(function(f) {
              return f.group === group.modelData;
            })

            readonly property bool isNothingGroup: group.modelData === "Fields that do nothing"

            visible: group.members.length > 0

            // The two real groups get a plain heading.
            Label {
              visible: !group.isNothingGroup
              text: group.modelData
              font.pixelSize: 11
              font.bold: true
              opacity: 0.55
              Layout.fillWidth: true
              Layout.topMargin: 8
            }

            // ⚠️ THE "DOES NOTHING" GROUP IS A DIFFERENT KIND OF GROUP, so it says so rather than
            // being a heading like the other two.
            //
            // Twilight, 2026-07-14: *"it would also be wonderful to know which ones were regenerated
            // or rewritten on save load with little exclamation points grouped below and hidden
            // behind a switch."* That is exactly this. The MODEL filters them out unless the
            // toolbar's "Reloaded values" switch is on -- so no view can leak one, and a test proves
            // they are gone (tst_warps: deadAndWipedFields_areAbsentUntilYouAskForThem).
            //
            // And ⚠️ WIPED and 💀 DEAD are **two different facts** -- one the console erases on every
            // load, one it keeps perfectly and never reads. Merging them into a single grey "unused"
            // would be a lie of convenience, and this project doesn't tell those.
            Rectangle {
              visible: group.isNothingGroup
              Layout.fillWidth: true
              Layout.topMargin: 10
              radius: 6
              color: Qt.rgba(0, 0, 0, 0.03)
              border.width: 1
              border.color: brg.settings.dividerColor
              implicitHeight: nothingCol.implicitHeight + 16

              ColumnLayout {
                id: nothingCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Label {
                  Layout.fillWidth: true
                  text: qsTr("Fields that do nothing")
                  font.pixelSize: 11
                  font.bold: true
                  opacity: 0.75
                }

                Label {
                  Layout.fillWidth: true
                  text: qsTr("Two different kinds of nothing — and they are not the same thing:")
                  wrapMode: Text.Wrap
                  font.pixelSize: 10
                  opacity: 0.6
                }

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 6

                  MapWarnIcon {
                    text: qsTr("The game zeroes this byte every time it loads a save. Verified on a "
                               + "real cartridge.")
                  }

                  Label {
                    Layout.fillWidth: true
                    text: qsTr("the game wipes it every time it loads your save")
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
              model: group.members

              delegate: WarpField {
                required property var modelData
                Layout.fillWidth: true

                fieldData: modelData
                ind: -1        // the MAP's state, not any one door's
              }
            }
          }
        }

        // The switch that reveals them lives in the TOOLBAR, and it is the same switch the sprite
        // panel uses. Point at it rather than growing a second one -- two switches doing one job is
        // exactly the clutter this screen was rebuilt to get rid of.
        Label {
          Layout.fillWidth: true
          Layout.topMargin: 10
          visible: !brg.map.showScratch
          text: qsTr("Four more fields here do nothing at all — the game either wipes them when it "
                     + "loads your save, or never reads them. Turn on “Reloaded values” in the "
                     + "toolbar to see them.")
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
        }

        Item { Layout.preferredHeight: 12 }   // breathing room at the bottom of the scroll
      }
    }
  }
}
