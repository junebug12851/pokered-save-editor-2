import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/modal"

Page {
  id: top

  property real saveHeight: 0.40
  property real revertHeight: 0.30
  property real scrubHeight: 1.00 - saveHeight - revertHeight
  property real saveAsHeight: 0.60
  property real saveCopyAsHeight: 1.00 - saveAsHeight
  property real divider: 0.65

  Image {
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/jelly-donuts.jpg"
    opacity: .35
  }

  TileButton {
    id: saveFileBtn
    title: "Save File"
    imgSrc: "qrc:/assets/icons/fontawesome/save.svg"
    hotKey: "Ctrl + S"
    sizeH: saveHeight
    divider: divider
    infoTxt: "Save your file so far"

    onClicked: {
      var res = brg.file.saveFile()

      // Close modal only if a file was selected
      if(res === true)
        brg.router.closeScreen();
    }
  }

  TileButton {
    title: "Revert File"
    imgSrc: "qrc:/assets/icons/fontawesome/undo.svg"
    hotKey: "Ctrl + Shift + O"
    anchors.top: saveFileBtn.bottom
    anchors.left: parent.left
    sizeH: revertHeight
    divider: divider
    infoTxt: "Destroy all unsaved changes"

    onClicked: {
      brg.file.reopenFile()
      brg.router.closeScreen();
    }
  }

  TileButton {
    title: "Scrub File"
    imgSrc: "qrc:/assets/icons/fontawesome/broom.svg"
    hotKey: "Ctrl + Shift + W"
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    sizeH: scrubHeight
    divider: divider
    infoTxt: "Scrub and sanitize the file clean, this means that " +
             "all the unused areas of the file are wiped."

    onClicked: {
      brg.file.wipeUnusedSpace()
      brg.router.closeScreen();
    }
  }

  TileButton {
    title: "Save File As..."
    imgSrc: "qrc:/assets/icons/fontawesome/file-export.svg"
    hotKey: "Ctrl + Shift + S"
    anchors.top: parent.top
    anchors.right: parent.right
    sizeH: saveAsHeight
    divider: divider
    infoTxt: "Save the file and unsaved changes to a different " +
             "file then switch to that file."

    onClicked: {
      var res = brg.file.saveFileAs()

      // Close modal only if a file was selected
      if(res === true)
        brg.router.closeScreen();
    }
  }

  TileButton {
    title: "Save Copy As..."
    imgSrc: "qrc:/assets/icons/fontawesome/copy.svg"
    hotKey: "Ctrl + Alt + S"
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    sizeH: saveCopyAsHeight
    divider: divider
    infoTxt: "Save the file and unsaved changes to a different " +
             "file but keep working on the current file."

    onClicked: {
      var res = brg.file.saveFileCopy()

      // Close modal only if a file was selected
      if(res === true)
        brg.router.closeScreen();
    }
  }

  ModalClose {
    onClicked: brg.router.closeScreen();
  }

  CreditWork {
    text: "\"Who wants some Jelly Donuts\" by Ry-Spirit (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/ry-spirit/art/Who-wants-some-Jelly-Donuts-373934999"
  }
}
