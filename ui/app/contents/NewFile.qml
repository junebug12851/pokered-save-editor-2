import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {
  id: top
  property real blankHeight: 0.40
  property real loadHeight: 0.60
  property real randomHeight: 1.00 - blankHeight
  property real recentHeight: 1.00 - loadHeight

  color: Style.primaryColorLight

  Image {
    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/pokewalk.jpg"
    opacity: .35
  }

  TileButton {
    title: "New Blank File"
    imgSrc: "qrc:/assets/fontawesome-icons/file.svg"
    hotKey: "Ctrl + N"
    sizeH: blankHeight
    infoTxt: "Create a new blank file with everything empty."

    onClicked: {
      file.newFile()
      exitAnim.start()
    }
  }

  TileButton {
    title: "New Random File"
    imgSrc: "qrc:/assets/fontawesome-icons/dice.svg"
    hotKey: "Ctrl + Shift + R"
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    sizeH: randomHeight
    infoTxt: "Create a new file with everything fully\n" +
             "randomized ready to play."

    onClicked: {
      file.data.randomizeExpansion()
      exitAnim.start()
    }
  }

  TileButton {
    title: "Load an Existing File..."
    imgSrc: "qrc:/assets/fontawesome-icons/folder-open.svg"
    hotKey: "Ctrl + O"
    anchors.top: parent.top
    anchors.right: parent.right
    sizeH: loadHeight
    infoTxt: "Switch to a different file"

    onClicked: {
      var res = file.openFile()

      // Close modal only if a file was selected
      if(res === true)
        exitAnim.start()
    }
  }

  RecentFilesTile {
    title: "Load a Recent File..."
    imgSrc: "qrc:/assets/fontawesome-icons/file-import.svg"
    hotKey: "Ctrl + Shift + [0-4]"
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    sizeH: recentHeight
    infoTxt: "Switch to one of your recently used files"

    onCompleted: exitAnim.start()
  }

  ModalClose {
    onClicked: exitAnim.start()
  }

  CreditWork {
    text: "\"Poke Walk Kanto\" by Ry-Spirit (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/ry-spirit/art/Poke-Walk-Kanto-591588328"
  }

  // Entrace animation
  NumberAnimation on x {
    from: (root.startingUp === false) ? -1000 : 0;
    to: 0;
    duration: (root.startingUp === false) ? 250 : 0
  }

  NumberAnimation on opacity {
    from: (root.startingUp === false) ? 0 : 1;
    to: 1;
    duration: (root.startingUp === false) ? 100 : 0
  }

  // Exit animation
  ParallelAnimation {
    id: exitAnim
    NumberAnimation {target: top; property: "x"; to: -1000; duration: 250}
    NumberAnimation {target: top; property: "opacity"; to: 0; duration: 250}

    onFinished: root.changeScreen("home")
  }
}
