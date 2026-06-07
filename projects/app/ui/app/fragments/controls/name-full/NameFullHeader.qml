// NameFullHeader.qml -- the full keyboard's top toolbar.
//
// Hosts the "Simulated" controls (Outdoor toggle + NameFullTileset combo), a
// Grid/Tileset view toggle (drives showTileset, which PagedPicker follows), the
// NameFullEdit name field, and a ModalClose that emits preClose() before closing
// (so the value commits exactly once). Carries the shared str/chopLen/sizeMult/
// isPersonName/hasBox state down to the editor.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../name"
import "../../general"
import "../../modal"

ToolBar {
  id: top

  signal preClose();

  property string str: ""
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  // Drives which page the left pane shows: false = character list, true = the
  // simulated tileset grid. Toggled by the "View" button below. FullKeyboard
  // binds its picker to this.
  property bool showTileset: false

  height: 124
  Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

  onStrChanged: editor.str = top.str

  // Flat, square toggle button: no Material elevation/shadow, no rounded
  // corners. Filled with the accent when `active`, outlined otherwise.
  component FlatToggle: Button {
    id: ftb
    property bool active: false

    flat: true
    font.capitalization: Font.Capitalize
    font.pixelSize: 12
    Material.elevation: 0

    topPadding: 9
    bottomPadding: 9
    leftPadding: 5
    rightPadding: 5

    background: Rectangle {
      radius: 0
      border.width: 1
      border.color: brg.settings.accentColor
      color: ftb.active
             ? brg.settings.accentColor
             : (ftb.hovered ? Qt.lighter(brg.settings.accentColor, 1.65) : "transparent")
    }

    contentItem: Text {
      text: ftb.text
      font: ftb.font
      color: ftb.active ? brg.settings.textColorLight : brg.settings.textColorDark
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
  }

  ColumnLayout {
    anchors.centerIn: parent
    spacing: 6

    // ---- "Simulated" control group: a plain text label, then two toggle
    //      buttons and the tileset combo. ----
    RowLayout {
      Layout.alignment: Qt.AlignHCenter
      spacing: 3

      // Plain caption — NOT a button.
      Label {
        text: "Simulated"
        font.bold: true
        font.pixelSize: 13
        color: brg.settings.textColorDark
        Layout.alignment: Qt.AlignVCenter
      }

      // Bold vertical pipe between the label and the controls (Twilight likes it).
      ToolSeparator {
        Layout.fillHeight: false
        Layout.preferredHeight: 24
        Layout.alignment: Qt.AlignVCenter
      }

      FlatToggle {
        text: "Outdoor"
        active: brg.settings.previewOutdoor
        onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;

        MainToolTip { text: "Render tiles as they'd look outdoors vs. indoors." }
      }

      NameFullTileset {
        Layout.alignment: Qt.AlignVCenter
        // In a RowLayout the combo's internal `width` is ignored, so without a
        // preferred width it collapses to its indicator and the tileset name
        // doesn't show. Keep it just wide enough for the names.
        Layout.preferredWidth: 132
      }

      // Label reflects the CURRENT view (it's the same simulated data, just shown
      // as the pill grid vs. the full tileset).
      FlatToggle {
        text: top.showTileset ? "Tileset" : "Grid"
        active: top.showTileset
        onClicked: top.showTileset = !top.showTileset;

        MainToolTip {
          text: "Switch between the character grid and the full simulated " +
                "tileset (click any tile to insert it)."
        }
      }
    }

    // ---- Name input: wide, centered, with Clear right beside it ----
    NameFullEdit {
      id: editor
      Layout.alignment: Qt.AlignHCenter

      chopLen: top.chopLen
      sizeMult: top.sizeMult
      isPersonName: top.isPersonName
      hasBox: top.hasBox

      str: top.str
      onStrChanged: top.str = str;
    }
  }

  ModalClose {
    onClicked: {
      top.preClose();
      brg.router.closeScreen();
    }
    anchors.topMargin: 3
    anchors.rightMargin: 3
    icon.width: 28
    icon.height: 28
  }
}

