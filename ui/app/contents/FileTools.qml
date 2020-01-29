import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {
  property real saveHeight: 0.40
  property real revertHeight: 0.30
  property real scrubHeight: 1.00 - saveHeight - revertHeight
  property real saveAsHeight: 0.60
  property real saveCopyAsHeight: 1.00 - saveAsHeight
  property real divider: 0.65

  color: Style.primaryColorLight

  Image {
    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/jelly-donuts.jpg"
    opacity: .35
  }

  TileButton {
    id: saveFileBtn
    title: "Save File"
    imgSrc: "qrc:/assets/fontawesome-icons/save.svg"
    hotKey: "Ctrl + S"
    sizeH: saveHeight
    divider: divider
    infoTxt: "Save your file so far"

    onClicked: {
      var res = file.saveFile()

      // Close modal only if a file was selected
      if(res === true)
        root.changeScreen("home")
    }
  }

  TileButton {
    title: "Revert File"
    imgSrc: "qrc:/assets/fontawesome-icons/undo.svg"
    hotKey: "Ctrl + Shift + O"
    anchors.top: saveFileBtn.bottom
    anchors.left: parent.left
    sizeH: revertHeight
    divider: divider
    infoTxt: "Destroy all unsaved changes"

    onClicked: {
      file.reopenFile()
      root.changeScreen("home")
    }
  }

  TileButton {
    title: "Scrub File"
    imgSrc: "qrc:/assets/fontawesome-icons/broom.svg"
    hotKey: "Ctrl + Shift + W"
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    sizeH: scrubHeight
    divider: divider
    infoTxt: "Scrub and sanitize the file clean, this means that\n" +
             "all the unused areas of the file are wiped."

    onClicked: {
      file.wipeUnusedSpace()
      root.changeScreen("home")
    }
  }

  TileButton {
    title: "Save File As..."
    imgSrc: "qrc:/assets/fontawesome-icons/file-export.svg"
    hotKey: "Ctrl + Shift + S"
    anchors.top: parent.top
    anchors.right: parent.right
    sizeH: saveAsHeight
    divider: divider
    infoTxt: "Save the file and unsaved changes to a different\n" +
             "file then switch to the new file."

    onClicked: {
      var res = file.saveFileAs()

      // Close modal only if a file was selected
      if(res === true)
        root.changeScreen("home")
    }
  }

  TileButton {
    title: "Save Copy As..."
    imgSrc: "qrc:/assets/fontawesome-icons/copy.svg"
    hotKey: "Ctrl + Alt + S"
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    sizeH: saveCopyAsHeight
    divider: divider
    infoTxt: "Save the file and unsaved changes to a different\n" +
             "file but keep working on the current file."

    onClicked: {
      var res = file.saveFileCopy()

      // Close modal only if a file was selected
      if(res === true)
        root.changeScreen("home")
    }
  }

  ModalClose {
    onClicked: root.changeScreen("home")
  }

  CreditWork {
    text: "\"Who wants some Jelly Donuts\" by Ry-Spirit (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/ry-spirit/art/Who-wants-some-Jelly-Donuts-373934999"
  }
}
