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

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    spacing: 6

    // One line, not two. At the default 750×480 window the panel has ~200px for the list, and a
    // two-line blurb costs two layer rows -- which is exactly the kind of thing the mandatory
    // screenshot review is for. (It wrapped to two lines on the first pass.)
    Text {
      Layout.fillWidth: true
      // Short enough to FIT the 170px panel. The longer version elided to "How you look at the map
      // — …", which is a sentence that has given up. (Screenshot review, 2026-07-13.)
      text: qsTr("None of this is in the save.")
      font.pixelSize: 11
      font.italic: true
      color: brg.settings.textColorMid
      elide: Text.ElideRight
    }

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

        // ⚠️ The indent, and why it is 26.
        //
        // A group row starts with its fold chevron (a 20px button) and only THEN its eye. A child
        // row has no chevron -- so with a naive 14px indent the child's eye landed LEFT of its
        // parent's, and the tree read upside down. (Twilight caught it: "the sub elements are
        // indented left past the root element, which is confusing and unintuitive -- a manual
        // screenshot would have detected this." She is right on both counts.)
        //
        // 26 = the chevron (20) + the row spacing (6), so a child's eye sits exactly under its
        // parent's eye, one step in. Change the chevron's size and change this with it.
        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: row.layerIsGroup ? 0 : 26
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

          // "none on this map" -- said on the row, not by leaving you to work it out.
          Text {
            visible: !row.layerIsGroup && !row.layerApplies
            text: qsTr("none here")
            font.pixelSize: 10
            font.italic: true
            color: brg.settings.textColorMid
            opacity: 0.6
            Layout.rightMargin: 2
          }
        }

        // The whole row explains itself. Half these words -- "counter" meaning a shop desk you can
        // talk across -- are things nobody should be expected to already know, and the app is where
        // that gets answered. Hover shows it; no preference to find first. (MapToolTip.qml)
        MapToolTip {
          visible: hover.hovered && row.layerDescription !== ""
          text: row.layerApplies
                ? row.layerDescription
                : qsTr("%1 — there is none of it on this map.").arg(row.layerName)
        }
      }
    }

    // ── The footer: the one dial, and the way back ───────────────────────────────────────────
    //
    // ONE row, not two. The overlay-strength slider and the clear button used to be stacked, and
    // between them they ate two layer rows out of a list that only has room for eight at the
    // default window size. (Caught by the screenshot review -- which is why it is mandatory.)
    //
    // The dial: how hard the meaning overlay is painted. Stacked annotation over four shades of
    // grey genuinely needs it, and it is shared rather than repeated nine times.
    RowLayout {
      Layout.fillWidth: true
      Layout.rightMargin: 16
      Layout.topMargin: 2
      spacing: 8
      // Bound to the two NOTIFYING properties, not to anyOn(): a plain method call is not a
      // binding, so this would appear once and then never update again. (The classic QML trap.)
      visible: brg.mapLayers.viewBits !== 0 || brg.map.layers !== 0

      Text {
        visible: brg.map.layers !== 0
        text: qsTr("Overlay")
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }

      Slider {
        id: opacitySlider
        visible: brg.map.layers !== 0
        Layout.fillWidth: true
        Layout.minimumHeight: 0
        Layout.preferredHeight: 24
        from: 0.15
        to: 1.0
        value: brg.mapLayers.overlayOpacity
        onMoved: brg.mapLayers.overlayOpacity = value

        ToolTip {
          parent: opacitySlider.handle
          visible: opacitySlider.pressed || opacitySlider.hovered
          text: Math.round(opacitySlider.value * 100) + "%"
          enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 70 } }
          exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 70 } }
        }
      }

      Item { Layout.fillWidth: brg.map.layers === 0 }

      MapRailButton {
        size: 24
        glyph: "⊘"
        tip: qsTr("Turn every layer off — back to just the map")
        onClicked: brg.mapLayers.clearAll()
      }
    }
  }
}
