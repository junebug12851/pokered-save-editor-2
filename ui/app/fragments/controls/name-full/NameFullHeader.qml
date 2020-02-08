import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../name"
import "../../general"
import "../../modal"

ToolBar {
  id: top

  signal toggleExample();
  signal reUpdateExample();
  signal preClose();

  property string str: ""
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  height: 75
  Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

  onStrChanged: {
    editor.str = top.str
  }

  Row {
    anchors.centerIn: parent
    spacing: 20

    NameFullEdit {
      id: editor

      onToggleExample: top.toggleExample();
      onReUpdateExample: top.reUpdateExample();

      chopLen: top.chopLen
      sizeMult: top.sizeMult
      isPersonName: top.isPersonName
      hasBox: top.hasBox

      str: top.str
      onStrChanged: top.str = str;
    }

    Button {
      text: "Is Outdoor"
      opacity: (brg.settings.previewOutdoor) ? 1.00 : 0.50
      onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;
      font.capitalization: Font.Capitalize
      font.pixelSize: 12
      flat: true
    }

    NameFullTileset {}
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
