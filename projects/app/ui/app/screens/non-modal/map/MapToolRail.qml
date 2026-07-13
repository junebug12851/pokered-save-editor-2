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
  MapToolRail.qml -- the tools, down the left edge (Photoshop / Aseprite / Tiled all put them here,
  and they are all right: the tool is the thing your hand reaches for first).

  What each tool DOES lives in the context bar above the canvas -- that is the whole point of the
  split (Aseprite: "the context bar shows specific options for the active tool"). The rail says
  WHICH; the bar says HOW.

  Each tool carries a single-key shortcut, shown on the tooltip, bound by the screen.
*/
import QtQuick
import QtQuick.Layouts

Rectangle {
  id: rail

  /// The active tool's id.
  property string tool: "select"

  readonly property var tools: [
    { id: "select",  glyph: "↖", key: "V", tip: qsTr("Select & Move — click to select, drag to move") },
    { id: "pan",     glyph: "✥", key: "H", tip: qsTr("Pan — drag the map around (or hold Space with any tool)") },
    { id: "zoom",    glyph: "⌕", key: "Z", tip: qsTr("Zoom — click to zoom in, Alt-click to zoom out") }
  ]

  implicitWidth: 44
  color: "#f2f2f2"

  Rectangle {
    anchors.right: parent.right
    width: 1
    height: parent.height
    color: "#22000000"
  }

  ColumnLayout {
    anchors.top: parent.top
    anchors.topMargin: 6
    anchors.horizontalCenter: parent.horizontalCenter
    spacing: 4

    Repeater {
      model: rail.tools

      MapRailButton {
        required property var modelData

        objectName: "toolBtn_" + modelData.id   // the DEBUG harness picks tools through these
        glyph: modelData.glyph
        tip: modelData.tip
        shortcut: modelData.key
        active: rail.tool === modelData.id

        onClicked: rail.tool = modelData.id
      }
    }
  }
}
