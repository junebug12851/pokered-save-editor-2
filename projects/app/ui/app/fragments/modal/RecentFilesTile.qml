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
      border.color: brg.settings.textColorMid
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
      spacing: -12

      x: 0
      y: 0
      width: parent.width
      height: parent.height
      model: brg.recentFilesModel

      delegate: Button {
        text: path
        width: parent.width
        enabled: isEnabled
        leftInset: -10
        rightInset: -10
        flat: true

        contentItem: Text {
          opacity: .75
          font.pixelSize: recentFilesBtn.hotKeySize * 0.85

          color: (parent.enabled)
                 ? brg.settings.textColorDark
                 : brg.settings.textColorMid

          text: parent.text
          horizontalAlignment: Text.AlignLeft
          verticalAlignment: Text.AlignVCenter
        }

        onClicked: recentFilesMenu.recentFileClick(fileIndex)
      }
    }
  }
}
