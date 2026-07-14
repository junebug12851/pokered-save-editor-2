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

/*
  LayersPanel.qml -- the layer tree. Three groups, every one of them collapsible, every layer in them
  toggleable, and the group's eye toggles the lot.

  This is what the old chip bar wanted to be. The differences that matter:

    * THE GUIDES AND THE GAME-VIEW BOXES ARE LAYERS NOW. The block grid, the map bounds, the PLAYER,
      the RED screen box and the ACCENT draw area used to be hard-coded rectangles with no switch at
      all -- you could not turn off the red box sitting over the map. Twilight asked for them as
      layers, and they are layers.
    * THE ROW IS THE LEGEND. Every row carries the exact ink the renderer paints in (it comes FROM
      the renderer -- MapEngine::layerColor), so a legend cannot drift out of sync with what is
      drawn. That is why the footer legend is gone.
    * A GROUP'S EYE IS TRI-STATE and one click always changes something: any child on -> the click
      turns the group off; none on -> it turns on every child that has something to show.
    * ALT-CLICK AN EYE = SOLO (Photoshop's gesture): that layer alone in its group, and alt-clicking
      it again puts back exactly what was on before. A solo is a look, not a destructive edit.
    * A LAYER WITH NOTHING TO SHOW SAYS SO. No grass on this map -> the Grass row is dimmed and says
      why, rather than switching on an empty overlay and leaving you wondering if it's broken.

  Everything here is a VIEW setting. Not one byte of the save is touched by any of it -- and
  tst_map_layers byte-diffs the whole save to prove it.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // The dock owns the frame and the title bar. A panel is its content.
  color: "transparent"

  /// A dirty counter. `anyOn()` / `groupAnyOn()` are METHODS, and a binding on a method never
  /// re-evaluates -- so the Clear buttons would light up once and then lie forever. Bumping this on
  /// the model's own signals is what gives those calls something to depend on. (This is THE recurring
  /// QML trap on this screen -- reference/qt-patterns.md.)
  property int revision: 0

  Connections {
    target: brg.mapLayers
    function onViewBitsChanged() { panel.revision++; }
  }

  Connections {
    target: brg.map
    function onOverlayChanged() { panel.revision++; }
  }

  readonly property bool anythingOn: {
    panel.revision;
    return brg.mapLayers.anyOn();
  }

  // ── The Clear, in the dock's title bar ──────────────────────────────────────────────────────
  //
  // Not down in a footer -- it was, and it ate two rows out of a list that only fits eight. The dock
  // instantiates this into the panel's own title, hard right (Twilight, 2026-07-13).
  property Component headerAction: Component {
    MapClearButton {
      enabled: panel.anythingOn
      tip: qsTr("Turn every layer off, in every group — back to just the map")
      onClicked: brg.mapLayers.clearAll()
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    spacing: 6

    // (A caption sat here -- "None of this is in the save." Removed 2026-07-13, Twilight: it is
    // inaccurate and confusing. It was true of the *switches*, but the things they draw are very
    // much in the save -- the player's coordinates, the grass tile, the connections, the edge of the
    // world. A caption that needs a paragraph of qualification should not be there at all; the rows
    // explain themselves on hover.)

    ListView {
      id: list

      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true

      model: brg.mapLayers
      spacing: 1

      ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

      delegate: Rectangle {
        id: row

        // Injected model roles.
        required property int index
        required property string layerName
        required property string layerDescription
        required property bool layerIsGroup
        required property bool layerVisible
        required property int layerGroupState
        required property bool layerApplies
        required property color layerColor
        required property bool layerExpanded
        required property string layerKey

        // Reserve the scrollbar's lane (the recurring gotcha -- ui-patterns.md).
        width: list.width - 16
        implicitHeight: layerIsGroup ? 26 : 23

        color: layerIsGroup ? "#00000000"
             : hover.hovered ? "#0d000000"
             : "transparent"

        HoverHandler { id: hover }

        // ⚠️ The indent, twice got wrong, so it is spelled out.
        //
        // A GROUP row is [chevron 20][spacing 6][eye] -- so its eye starts at x = 26.
        // A CHILD row has no chevron, so its indent has to clear that AND then step in:
        //
        //     26 (where the group's eye is) + 14 (one indent step) = 40.
        //
        // First cut used 14 -- the child's eye landed LEFT of its parent's and the tree read upside
        // down. Second cut used 26 -- the eyes lined up exactly, which is not a hierarchy either.
        // (Twilight caught both, by looking. "A manual screenshot would have detected this.")
        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: row.layerIsGroup ? 0 : 40
          spacing: 6

          // The group's fold. Clutter is a bug: a group you are not using takes one line.
          MapRailButton {
            visible: row.layerIsGroup
            size: 20
            glyph: row.layerExpanded ? "⌄" : "›"
            tip: row.layerExpanded ? qsTr("Fold this group away") : qsTr("Unfold this group")
            onClicked: brg.mapLayers.toggleExpanded(row.index)
          }

          // ── The eye ───────────────────────────────────────────────────────────────────────
          //
          // Filled = on. Hollow = off. A group shows a HALF-filled eye when only some of its
          // children are on, so the group row tells the truth even when it is folded shut.
          Rectangle {
            id: eye

            objectName: "layerEye_" + row.layerKey   // the DEBUG harness drives the layers by name

            implicitWidth: 18
            implicitHeight: 18
            radius: 4

            readonly property bool on: row.layerIsGroup ? (row.layerGroupState > 0) : row.layerVisible
            readonly property bool partial: row.layerIsGroup && row.layerGroupState === 1

            color: !row.layerApplies ? "transparent"
                 : on ? Qt.rgba(row.layerColor.r, row.layerColor.g, row.layerColor.b,
                                partial ? 0.35 : 0.85)
                 : "transparent"

            border.width: 1
            border.color: row.layerApplies ? row.layerColor : Qt.rgba(0, 0, 0, 0.15)
            opacity: row.layerApplies ? 1.0 : 0.45

            Behavior on color { ColorAnimation { duration: 90 } }

            // The "some" state, said with a shape as well as an alpha -- colour is never the only
            // signal (the overlay already gives every layer its own pattern; the rows keep faith
            // with that).
            Rectangle {
              visible: eye.partial
              anchors.centerIn: parent
              width: 8
              height: 2
              radius: 1
              color: row.layerColor
            }

            MouseArea {
              anchors.fill: parent
              cursorShape: Qt.PointingHandCursor
              acceptedButtons: Qt.LeftButton

              onClicked: (mouse) => {
                // Alt-click = solo. Photoshop's gesture, and worth having: with nine meaning layers
                // the question is usually "just show me the ledges".
                if (!row.layerIsGroup && (mouse.modifiers & Qt.AltModifier))
                  brg.mapLayers.solo(row.index);
                else
                  brg.mapLayers.toggle(row.index);
              }
            }
          }

          Text {
            Layout.fillWidth: true
            text: row.layerName
            font.pixelSize: row.layerIsGroup ? 12 : 11
            font.bold: row.layerIsGroup || (!row.layerIsGroup && row.layerVisible)
            color: !row.layerApplies ? brg.settings.textColorMid
                 : row.layerIsGroup ? brg.settings.textColorDark
                 : (row.layerVisible ? brg.settings.textColorDark : brg.settings.textColorMid)
            opacity: row.layerApplies ? 1.0 : 0.5
            elide: Text.ElideRight
          }

          // (A "none here" tag sat here on any layer this map has none of. Removed 2026-07-13,
          // Twilight: "extra clutter". The row is already dimmed and its hollow eye already says it,
          // and the tooltip spells it out -- three ways of saying one thing was two too many.)

          // ── Clear, per category ───────────────────────────────────────────────────────────
          //
          // Not the group's eye: the eye is a TOGGLE, and clicking it with nothing on switches the
          // whole group ON. A control that sometimes does the opposite of clearing is not a Clear.
          MapClearButton {
            visible: row.layerIsGroup
            compact: true
            enabled: {
              panel.revision;
              return brg.mapLayers.groupAnyOn(row.index);
            }
            Layout.rightMargin: 2
            tip: qsTr("Turn off everything in %1").arg(row.layerName)
            onClicked: brg.mapLayers.clearGroup(row.index)
          }
        }

        // The whole row explains itself. Half these words -- "counter" meaning a shop desk you can
        // talk across -- are things nobody should be expected to already know, and the app is where
        // that gets answered. ⚠️ Gated on the header's "?" toggle. (MapToolTip.qml)
        MapToolTip {
          shown: hover.hovered && row.layerDescription !== ""
          text: row.layerApplies
                ? row.layerDescription
                : qsTr("%1 — there is none of it on this map.").arg(row.layerName)
        }
      }
    }

    // ── The footer: the one dial ─────────────────────────────────────────────────────────────
    //
    // The Clear that used to sit down here has gone UP, into the dock's title bar (Twilight,
    // 2026-07-13) -- which is also where a panel-wide action belongs. What is left is the dial:
    // how hard the overlay is painted. Stacked annotation over four shades of grey genuinely needs
    // it, and it is shared rather than repeated nine times.
    RowLayout {
      Layout.fillWidth: true
      Layout.rightMargin: 16
      Layout.topMargin: 2
      spacing: 8
      // Bound to the NOTIFYING property, not to a method call: a plain method call is not a binding,
      // so this would appear once and then never update again. (The classic QML trap.)
      visible: brg.map.layers !== 0

      Text {
        text: qsTr("Overlay")
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }

      Slider {
        id: opacitySlider
        Layout.fillWidth: true
        Layout.minimumHeight: 0
        Layout.preferredHeight: 24
        from: 0.15
        to: 1.0
        value: brg.mapLayers.overlayOpacity
        onMoved: brg.mapLayers.overlayOpacity = value

        // MapToolTip, not the stock one. @see MapToolTip -- the stock ToolTip is dark-on-translucent
        // and unreadable, and it is the complaint that keeps coming back.
        MapToolTip {
          parent: opacitySlider.handle
          shown: opacitySlider.pressed || opacitySlider.hovered
          followGlobalSetting: false
          delay: 0
          text: Math.round(opacitySlider.value * 100) + "%"
        }
      }
    }
  }
}
