import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/modal"

Page {
  id: top

  property real blankHeight: 0.40
  property real loadHeight: 0.60
  property real randomHeight: 1.00 - blankHeight
  property real recentHeight: 1.00 - loadHeight

  Image {
    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/pokewalk.jpg"
    opacity: .35
  }

  TileButton {
    title: "New Blank File"
    imgSrc: "qrc:/assets/icons/fontawesome/file.svg"
    hotKey: "Ctrl + N"
    sizeH: blankHeight
    infoTxt: "Create a new blank file with everything empty."
    iconSize: 30

    onClicked: {
      brg.file.newFile()
      brg.router.closeScreen();
    }
  }

  TileButton {
    title: "New Random File"
    imgSrc: "qrc:/assets/icons/fontawesome/dice.svg"
    hotKey: "Ctrl + Shift + R"
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    sizeH: randomHeight
    infoTxt: "Create a new file with everything fully " +
             "randomized ready to play."

    onClicked: {
      brg.file.data.randomizeExpansion()
      brg.router.closeScreen();
    }
  }

  TileButton {
    title: "Load an Existing File..."
    imgSrc: "qrc:/assets/icons/fontawesome/folder-open.svg"
    hotKey: "Ctrl + O"
    anchors.top: parent.top
    anchors.right: parent.right
    sizeH: loadHeight
    infoTxt: "Switch to a different file"

    onClicked: {
      var res = brg.file.openFile()

      // Close modal only if a file was selected
      if(res === true)
        brg.router.closeScreen();
    }
  }

  RecentFilesTile {
    title: "Load a Recent File..."
    imgSrc: "qrc:/assets/icons/fontawesome/file-import.svg"
    hotKey: "Ctrl + Shift + [0-4]"
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    sizeH: recentHeight
    infoTxt: "Switch to one of your recently used files"

    onCompleted: brg.router.closeScreen();
  }

  ModalClose {
    onClicked: brg.router.closeScreen();
  }

  CreditWork {
    text: "\"Poke Walk Kanto\" by Ry-Spirit (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/ry-spirit/art/Poke-Walk-Kanto-591588328"
  }
}
