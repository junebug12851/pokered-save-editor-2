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
        a