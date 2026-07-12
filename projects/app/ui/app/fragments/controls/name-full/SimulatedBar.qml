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

  /// The picker closed -- hand the keys back to the keyboard.
  signal picked()

  /// Driven by the deck's Tab key: Tab opens the tileset picker (then Up/Down to walk it
  /// and Tab or Enter to choose), Ctrl+Tab flips Outdoor.
  function openPicker() {
    tileset.openMenu();
  }

  function flipOutdoor() {
    brg.settings.previewOutdoor = !brg.settings.previewOutdoor;
  }

  // Same pill toggle as the rest of the screen's chrome -- properly padded, rounded,
  // accent-filled when on.
  component FlatToggle: Button {
    id: ftb
    property bool active: false

    flat: true
    font.capitalization: Font.Capitalize
    font.pixelSize: 11
    Material.elevation: 0

    // Taller, and sized like a control rather than a word with a box drawn round it.
    topPadding: 7
    bottomPadding: 7
    leftPadding: 14
    rightPadding: 14
    implicitHeight: 28

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
        id: tileset
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: 122

        onMenuClosed: bar.picked();
      }

      FlatToggle {
        text: qsTr("Outdoor")
        active: brg.settings.previewOutdoor
        onClicked: bar.flipOutdoor();

        MainToolTip {
          text: "Render these tiles as they'd look outdoors vs. indoors. (Ctrl+Tab)"
        }
      }
    }
  }
}
