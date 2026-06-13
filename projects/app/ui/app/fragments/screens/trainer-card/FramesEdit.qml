// FramesEdit.qml -- the "frames" field of the playtime clock (+ the clock's menu).
//
// A 2-digit DefTextEdit bound to playtime.frames (clamped 0-59), dimmed when the
// clock is maxed. The last PlaytimeEdit sub-field; it also hosts the playtime
// overflow menu (exposed via menuBtnVisible): Enabled (clockMaxed toggle), Paused
// (area.general.countPlaytime), Randomize, Clear, Close.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
  width: 2 * font.pixelSize + leftPadding + rightPadding

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(isNaN(txtDec))
      return;

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.frames = txtDec;
  }

  MainToolTip {
    text: qsTr("Frames played, 0-59")
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onFramesChanged() { framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames.toString(); }
  }

  Component.onCompleted: framesEdit.text = brg.file.data.dataExpanded.world.other.playtime.frames.toString();

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
        text: qsTr("Close")
        onTriggered: playtimeMenu.close();
      }
    }
  }
}