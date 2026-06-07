// RecentFilesTile.qml -- the "recent files" tile on the NewFile modal.
//
// A TileButton that, when clicked, hides itself and opens an in-place Popup
// listing brg.recentFilesModel. Picking an entry calls brg.file.openFileRecent()
// and emits completed() (the modal closes); the special -1 index clears the recent
// list. Closing the popup restores the tile's opacity.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
    width: parent ? parent.width : 0
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
        // Only dismiss the New File modal if the file actually loaded. On failure
        // openFileRecent() returns false and raises the file-error screen on top;
        // leaving this modal in place means closing the error returns the user here.
        if(brg.file.openFileRecent(fileIndex))
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
      width: parent ? parent.width : 0
      height: parent.height
      model: brg.recentFilesModel

      delegate: Button {
        text: path
        width: parent ? parent.width : 0
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
