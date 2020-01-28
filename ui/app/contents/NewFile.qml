import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {
  property real blankHeight: 0.40
  property real loadHeight: 0.70
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
  }

  TileButton {
    title: "New Random File"
    imgSrc: "qrc:/assets/fontawesome-icons/dice.svg"
    hotKey: "Ctrl + Shift + R"
    anchors.bottom: parent.bottom
    anchors.left: parent.left
    sizeH: randomHeight
  }

  TileButton {
    title: "Load an Existing File..."
    imgSrc: "qrc:/assets/fontawesome-icons/folder-open.svg"
    hotKey: "Ctrl + O"
    anchors.top: parent.top
    anchors.right: parent.right
    sizeH: loadHeight
  }

  TileButton {
    title: "Load a Recent File..."
    imgSrc: "qrc:/assets/fontawesome-icons/file-import.svg"
    hotKey: "Ctrl + Shift + [0-4]"
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    sizeH: recentHeight
  }

  ModalClose {
    onClicked: root.changeScreen("home")
  }

  CreditWork {
    text: "\"Poke Walk Kanto\" by Ry-Spirit (CC-BY-NC-ND 3.0)\n" +
          "https://www.deviantart.com/ry-spirit/art/Poke-Walk-Kanto-591588328"
  }
}
