// SimulatedBar.qml -- the "Simulated" tileset controls, shown ABOVE the keyboard on the
// tile pages.
//
// These used to sit in the top bar, where they were three things floating over a name
// field they have nothing to do with. They belong next to the thing they affect: the
// picture tiles. Put here, with a sentence explaining WHY the tiles are only an
// approximation, the control explains itself instead of needing to be explained.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../name"

Item {
  id: bar

  implicitHeight: tray.implicitHeight

  // Same pill toggle as the rest of the screen's chrome -- properly padded, rounded,
  // accent-filled when on.
  component FlatToggle: Button {
    id: ftb
    property bool active: false

    flat: true
    font.capitalization: Font.Capitalize
    font.pixelSize: 11
    Material.elevation: 0

    topPadding: 5
    bottomPadding: 5
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
      radius: 4
      border.width: 1
      border.color: ftb.active ? brg.settings.accentColor : brg.settings.dividerColor
      color: ftb.active
             ? brg.settings.accentColor
             : (ftb.hovered ? Qt.lighter(brg.settings.dividerColor, 1.18) : "#ffffff")
    }

    contentItem: Text {
      text: ftb.text
      font: ftb.font
      color: ftb.active ? brg.settings.textColorLight : brg.settings.textColorDark
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
  }

  Rectangle {
    id: tray
    anchors.centerIn: parent

    implicitWidth: row.implicitWidth + 20
    implicitHeight: row.implicitHeight + 10

    radius: 6
    color: Qt.lighter(brg.settings.dividerColor, 1.34)
    border.width: 1
    border.color: brg.settings.dividerColor

    RowLayout {
      id: row
      anchors.centerIn: parent
      spacing: 9

      Label {
        text: qsTr("Simulated with")
        font.pixelSize: 11
        color: brg.settings.textColorMid
        Layout.alignment: Qt.AlignVCenter
      }

      NameFullTileset {
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: 122
      }

      FlatToggle {
        text: qsTr("Outdoor")
        active: brg.settings.previewOutdoor
        onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;

        MainToolTip { text: "Render these tiles as they'd look outdoors vs. indoors." }
      }
    }
  }
}
