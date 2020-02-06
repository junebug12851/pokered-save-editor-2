import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

TileButton {
  id: recentFilesBtn
  signal completed()

  onClicked: {
    recentFilesBtn.opacity = 0;
    recentFilesMenu.open();
  }

  Popup {
    id: recentFilesMenu
    x: 0
    y: 0
    width: parent.width
    height: parent.height
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    clip: true

    background: Rectangle {
      color: "transparent"
      border.color: brg.settings.primaryColorDark
      border.width: 1
    }

    onClosed: recentFilesBtn.opacity = 1;

    function recentFileClick(fileIndex) {
      if(fileIndex === -1)
        brg.file.clearRecentFiles()
      else
      {
        brg.file.openFileRecent(fileIndex)
        completed()
      }

      close();
    }

    ListView {
      id: recentFilesList
      anchors.fill: parent
      spacing: -15

      x: 0
      y: 0
      width: parent.width
      height: parent.height
      model: recentFilesModel

      delegate: Button {
        text: path
        width: parent.width
        enabled: isEnabled

//        background: Rectangle {
//          color: manualMouse.curColor
//        }

        contentItem: Text {
//          color: (parent.enabled)
//                 ? Qt.darker(Style.textColorPrimary, 2)
//                 : Style.textColorPrimary

          opacity: recentFilesBtn.titleOpacity
          font.pixelSize: recentFilesBtn.hotKeySize * 0.85

          text: parent.text
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
        }
      }
    }
  }
}
