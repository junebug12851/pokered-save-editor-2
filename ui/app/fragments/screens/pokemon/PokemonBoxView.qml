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
  property int cellSize: 100
  property PokemonStorageModel theModel: null

  cellWidth: cellSize
  cellHeight: cellSize
  clip: true

  function hack_newAndRePositionViewEnd() {
    theModel.getCurBox().pokemonNew();
    positionViewAtEnd();
  }

  // We manually open outside of the router the pokemon details page beacuse
  // we want to pass parameters to it
  function openMonEditor(isParty, monData) {

    // Will fix this in a minute
    if(isParty)
      appBody.push("qrc:/ui/app/screens/non-modal/PokemonDetails.qml", {
                     boxData: monData,
                     partyData: monData
                   });
    else
      appBody.push("qrc:/ui/app/screens/non-modal/PokemonDetails.qml", {
                     boxData: monData
                   });

    // We then tell the router of what we've done
    brg.router.manualStackPush("pokemonDetails");

    // And then we open incomming signals so that we can receive input
    // This is tricky, we first enable listening for a page close event from the
    // router and then we enable listening from the details page. When the
    // details page closes we want both to shut off. This is how it's done
    pokemonDetailsListenerShutOff.target = brg.router;
    pokemonDetailsListener.target = appRoot.currentItem;
  }

  // Incomming signals from full-keyboard
  Connections {
    id: pokemonDetailsListener

    // Initially set to no incomming signals
    target: null
    ignoreUnknownSignals: true
  }

  // Here we shut-off connections
  Connections {
    id: pokemonDetailsListenerShutOff

    target: null
    ignoreUnknownSignals: true

    onCloseNonModal: {
      theModel.onReset();
      pokemonDetailsListener.target = null
      pokemonDetailsListenerShutOff.target = null;
    }
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

      var shiny = (itemIsShiny)
          ? "-shiny"
          : ""

      return "qrc:/assets/icons/mon-icons/" + num + "-" + name + shiny + ".svg";
    }

    function getMonNickname() {
      if(itemNickname === undefined || itemNickname === null)
        return "";

      if(itemNickname.length > 10)
        return itemNickname.substring(0, 7) + "..."
      else
        return itemNickname;
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

      text: getMonNickname()
      font.capitalization: Font.MixedCase

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

      onClicked: {
        if(itemIsParty)
          openMonEditor(itemIsParty, view.theModel.getPartyMon(index));
        else
          openMonEditor(itemIsParty, view.theModel.getBoxMon(index));
      }
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

    Rectangle {
      visible: !itemIsPlaceholder

      anchors.top: parent.top
      anchors.topMargin: 15
      anchors.right: parent.right

      radius: 20
      color: brg.settings.accentColor
      width: 12 * 3
      height: 15

      Text {
        anchors.centerIn: parent

        color: brg.settings.textColorLight
        font.pixelSize: 12
        text: "L" + itemLevel
      }
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

      onClicked: hack_newAndRePositionViewEnd()
    }

    color: "transparent"

    width: view.cellSize
    height: view.cellSize
  }

  ScrollBar.vertical: ScrollBar {}
}
