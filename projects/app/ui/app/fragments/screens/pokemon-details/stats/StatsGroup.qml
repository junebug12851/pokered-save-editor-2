import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../../general"
import "../../../header"

// StatsGroup.qml -- the read-only computed-stats grid (GlancePane, valid mons).
//
// Read-only computed stats (HP/Atk/Def/Spd/Sp) shown as a clean label | value
// grid. Two columns align on their own — no per-row anchor offsets.
Rectangle {
  color: "transparent"

  // Expose the grid's content size so the GlancePane can size the stats column and
  // anchor the sprite to its right (without this the rect is 0-wide and the sprite
  // overlaps the stats — visible once the pane is narrowed).
  implicitWidth: statsGrid.implicitWidth
  implicitHeight: statsGrid.implicitHeight

  GridLayout {
    id: statsGrid
    anchors.top: parent.top
    anchors.left: parent.left

    columns: 2
    columnSpacing: 5
    rowSpacing: 3

    Text {
      text: qsTr("HP")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Text {
      text: (boxData.isValidBool) ? boxData.hpStat : "???"
      font.pixelSize: 14
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    Text {
      text: qsTr("Atk")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Text {
      text: (boxData.isValidBool) ? boxData.atkStat : "???"
      font.pixelSize: 14
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    Text {
      text: qsTr("Def")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Text {
      text: (boxData.isValidBool) ? boxData.defStat : "???"
      font.pixelSize: 14
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    Text {
      text: qsTr("Spd")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Text {
      text: (boxData.isValidBool) ? boxData.spdStat : "???"
      font.pixelSize: 14
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    Text {
      text: qsTr("Sp")
      font.pixelSize: 14
      font.bold: true
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Text {
      text: (boxData.isValidBool) ? boxData.spStat : "???"
      font.pixelSize: 14
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }
  }
}
