import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonStorageModel 1.0
import App.PokemonBoxSelectModel 1.0

import "../../general"
import "../../controls/selection"

GridView {
  id: view
  property int cellSize: 75
  property PokemonStorageModel theModel: null

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true

  function hack_newAndRePositionViewEnd() {
    theModel.getCurBox().pokemonNew();
    positionViewAtEnd();
  }

  delegate: Rectangle {
    property bool isLast: index+1 < view.count ? false : true

    function getMonUrl() {

      if(itemDex === -1 || itemDex === undefined || itemDex === null)
        return "qrc:/assets/icons/fontawesome/question.svg";

      var num = (itemDex+1).toString().padStart(3, "0");

      var name = itemName.toLowerCase();
      if(name === "nidoran<f>")
        name = "nidoran-f";
      else if(name === "nidoran<m>")
        name = "nidoran-m";
      else if(name === "mr.mime")
        name = "mrmime";

      return "qrc:/assets/icons/mon-icons/" + num + "-" + name + ".svg";
    }

    CheckBox {
      id: selectBox
      hoverEnabled: true

      visible: !itemIsPlaceholder && (mouse.containsMouse || checked || hovered || editBtn.hovered)

      anchors.top: parent.top
      anchors.left: parent.left

      Component.onCompleted: (itemChecked !== undefined)
                             ? checked = itemChecked
                             : checked = false;

      onCheckedChanged: itemChecked = checked;

      z: 100

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight
    }

    Button {
      id: editBtn
      visible: !itemIsPlaceholder && (mouse.containsMouse || hovered || selectBox.hovered)
      hoverEnabled: true

      text: "Edit"
      font.capitalization: Font.Capitalize

      height: 20
      padding: 0
      topInset: 0
      rightInset: 0
      bottomInset: 0
      leftInset: 0
      display: AbstractButton.TextBesideIcon
      flat: true
      z: 100

      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right

      icon.source: "qrc:/assets/icons/fontawesome/pen.svg"
      icon.width: 10
      icon.height: 10

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight
    }

    MouseArea {
      id: mouse

      anchors.fill: parent
      hoverEnabled: true
    }

    Image {
      visible: !itemIsPlaceholder

      anchors.centerIn: parent
      width: parent.width - 15
      height: parent.height - 15

      sourceSize.height: height
      sourceSize.width: width

      source: getMonUrl()
      fillMode: Image.PreserveAspectFit
    }

    RoundButton {
      anchors.centerIn: parent
      visible: itemIsPlaceholder
      width: parent.width * 0.60
      height: parent.height * 0.60
      radius: 100
      display: AbstractButton.IconOnly
      padding: 0
      topInset: 0
      rightInset: 0
      bottomInset: 0
      leftInset: 0
      flat: true

      icon.source: "qrc:/assets/icons/fontawesome/plus.svg"

      Material.background: brg.settings.primaryColor
      Material.foreground: brg.settings.textColorLight

      onClicked: theModel.getCurBox().pokemonNew();
    }

    color: "transparent"

    width: view.cellSize
    height: view.cellSize
  }

  ScrollBar.vertical: ScrollBar {}
}
