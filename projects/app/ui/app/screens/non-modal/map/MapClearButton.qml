/*
  * Copyright 2026 Fairy Fox
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
 * A CLEAR button. It says "Clear".
 *
 * ⚠️ It used to be a lone `⊘` on the rail-button chassis, which is the same chassis every *tool* on
 * this screen uses -- so it read as one more mode you could switch into, and told you nothing about
 * what it would do. Project leadership: *"the clear button ... should actually look like a clear button."*
 *
 * So: a flat pill with the word in it. It is disabled -- not hidden -- when there is nothing to
 * clear, because a control that vanishes when it is not needed is a control you cannot find when it
 * is. A disabled one is still a label telling you what this corner of the panel does.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: btn

  /// Shown next to the ⊘. Kept to one word.
  property string label: qsTr("Clear")

  /// Small = the one that sits on a group's row. Normal = the one in the panel's title bar.
  property bool compact: false

  /// What it explains, on hover. ⚠️ Gated on the header's "?" toggle, like every other tooltip.
  property string tip: ""

  signal clicked()

  implicitWidth: pill.implicitWidth
  implicitHeight: btn.compact ? 18 : 22

  opacity: btn.enabled ? 1.0 : 0.35

  Rectangle {
    id: pill

    anchors.fill: parent
    implicitWidth: row.implicitWidth + (btn.compact ? 10 : 14)
    radius: height / 2

    color: !btn.enabled       ? "transparent"
         : hover.hovered      ? "#d55e00"
         : "transparent"

    border.width: 1
    border.color: !btn.enabled  ? brg.settings.dividerColor
                : hover.hovered ? "#d55e00"
                : "#b0000000"

    Behavior on color { ColorAnimation { duration: 90 } }

    Row {
      id: row
      anchors.centerIn: parent
      spacing: 3

      Text {
        anchors.verticalCenter: parent.verticalCenter
        text: "⊘"
        font.pixelSize: btn.compact ? 9 : 11
        color: hover.hovered && btn.enabled ? "#ffffff" : brg.settings.textColorMid
      }

      Text {
        anchors.verticalCenter: parent.verticalCenter
        text: btn.label
        font.pixelSize: btn.compact ? 9 : 11
        font.bold: !btn.compact
        color: hover.hovered && btn.enabled ? "#ffffff" : brg.settings.textColorMid
      }
    }

    HoverHandler {
      id: hover
      enabled: btn.enabled
      cursorShape: Qt.PointingHandCursor
    }

    TapHandler {
      enabled: btn.enabled
      // Consume it: the row underneath is a layer row, and clicking Clear must not also fold or
      // toggle the group it sits on.
      onTapped: (event) => { btn.clicked(); }
      gesturePolicy: TapHandler.ReleaseWithinBounds
    }
  }

  MapToolTip {
    shown: hover.hovered && btn.tip !== ""
    text: btn.tip
  }
}
