import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

DefTextEdit {
  id: framesEdit

  property alias menuBtnVisible: menuBtn.visible

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  width: 2 * font.pixelSize

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(txtDec === NaN)
      return;

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.frames = txtDec;
  }

  MainToolTip {
    text: "Frames played, 0-59"
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    onFramesChanged: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames
  }

  Component.onCompleted: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames;

  IconButtonSquare {
    id: menuBtn
    visible: false

    anchors.topMargin: -12

    anchors.top: parent.top
    anchors.left: parent.right

    icon.width: 7
    icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

    onClicked: playtimeMenu.open();

    Menu {
      id: playtimeMenu

      MenuItem {
        text: "Enabled";
        checkable: true
        checked: !brg.file.data.dataExpanded.world.other.playtime.clockMaxed
        onTriggered: brg.file.data.dataExpanded.world.other.playtime.clockMaxed = !brg.file.data.dataExpanded.world.other.playtime.clockMaxed;
      }

      MenuItem {
        text: "Paused";
        checkable: true
        checked: !brg.file.data.dataExpanded.area.general.countPlaytime
        onTriggered: brg.file.data.dataExpanded.area.general.countPlaytime = !brg.file.data.dataExpanded.area.general.countPlaytime;
      }

      MenuItem {
        text: "Randomize";
        onTriggered: brg.file.data.dataExpanded.world.other.randomizePlaytime();
      }

      MenuItem {
        text: "Clear";
        onTriggered: brg.file.data.dataExpanded.world.other.clearPlaytime();
      }

      MenuItem {
        text: "Close"
        onTriggered: playtimeMenu.close();
      }
    }
  }
}
