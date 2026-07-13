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
  MapIdentityBar.qml -- WHAT IS LOADED. One job, and it never does anything else.

  The old info row carried the map name, the tileset, the size, the player, a hand-rolled contrast
  stepper AND three panel toggles -- five unrelated ideas in one strip, which is why the screen felt
  like a dev tool. Tool options went to the context bar; the panels went to the dock rail. What is
  left here is the answer to "what am I looking at", and that is all.

  It is also where the screen tells you when what you are looking at is not what it seems: a glitch
  / half-baked map id draws ANOTHER map's data, which is exactly what a Game Boy does with it. That
  gets said out loud, not passed off as a map of its own.
*/
import QtQuick
import QtQuick.Layouts

Rectangle {
  id: bar

  implicitHeight: 34
  color: "#f7f7f7"

  Rectangle {
    anchors.bottom: parent.bottom
    width: parent.width
    height: 1
    color: brg.settings.dividerColor
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 12
    anchors.rightMargin: 12
    spacing: 10

    Text {
      text: brg.map.valid ? brg.map.mapName : qsTr("No map")
      font.pixelSize: 13
      font.bold: true
      color: brg.settings.textColorDark
      elide: Text.ElideRight
      Layout.maximumWidth: 220
    }

    // A quiet chip: the fact, not a label saying what kind of fact it is.
    component Chip: Rectangle {
      property alias text: chipText.text
      implicitWidth: chipText.implicitWidth + 14
      implicitHeight: 20
      radius: 10
      color: "#ffffff"
      border.width: 1
      border.color: brg.settings.dividerColor

      Text {
        id: chipText
        anchors.centerIn: parent
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }

    Chip { text: brg.map.tilesetName }
    Chip { text: qsTr("%1 × %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh) }

    // The map id, plainly. It is the number the save actually holds, and someone editing a save
    // wants to see it.
    Chip { text: qsTr("id %1").arg(brg.map.mapInd) }

    Rectangle {
      visible: brg.map.isCopy
      implicitWidth: warnText.implicitWidth + 16
      implicitHeight: 20
      radius: 10
      color: Qt.rgba(brg.settings.errorColor.r, brg.settings.errorColor.g,
                     brg.settings.errorColor.b, 0.10)
      border.width: 1
      border.color: brg.settings.errorColor

      Text {
        id: warnText
        anchors.centerIn: parent
        text: qsTr("⚠ unfinished copy — the game draws %1's data here").arg(brg.map.copyOfName)
        font.pixelSize: 11
        color: brg.settings.errorColor
      }
    }

    Item { Layout.fillWidth: true }
  }
}
