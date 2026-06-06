import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Effects

import App.PokemonStorageModel
import App.PokemonBoxSelectModel

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

    function onCloseNonModal() {
      theModel.onReset();
      pokemonDetailsListener.target = null;
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
      // Show the nickname; for an un-nicknamed mon (empty nickname) fall back to
      // the species name, matching the in-game display. (Most mons in a save have
      // no custom nickname, so without this the label is just blank.)
      var nick = (itemNickname === undefined || itemNickname === null) ? "" : itemNickname;
      if(nick === "")
        nick = (itemName === undefined || itemName === null) ? "" : itemName;

      if(nick.length > 10)
        return nick.substring(0, 7) + "...";
      else
        return nick;
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
      Material.elevation: 0
      z: 100

      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      // The Material Button's built-in icon+text label would NOT render the text
      // at this small fixed height (the accent pill showed but the name never
      // did). Draw the name with an explicit Text — same approach as the level
      // badge, which renders fine. The cell-wide MouseArea handles the click.
      contentItem: Item {
        Row {
          anchors.centerIn: parent
          spacing: 4

          Image {
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            sourceSize.width: 10
            sourceSize.height: 10
            source: "qrc:/assets/icons/fontawesome/pen.svg"
            fillMode: Image.PreserveAspectFit

            // Tint the monochrome SVG to match the light text. pen.svg is a
            // solid BLACK path; MultiEffect.colorization scales the tint by the
            // source's luminance, so a black source stays black. brightness:1.0
            // pushes it to white first, then colorization recolors it to
            // textColorLight. (Without brightness the pen renders dark.)
            layer.enabled: true
            layer.effect: MultiEffect {
              brightness: 1.0
              colorization: 1.0
              colorizationColor: brg.settings.textColorLight
            }
          }

          Text {
            anchors.verticalCenter: parent.verticalCenter
            text: editBtn.text
            color: brg.settings.textColorLight
            font: editBtn.font
          }
        }
      }

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

      // Clicking anywhere on a (non-empty) slot opens the editor, not just the
      // little hover button. Placeholder "+" slots keep their own add handler.
      onClicked: {
        if(itemIsPlaceholder)
          return;
        if(itemIsParty)
          openMonEditor(itemIsParty, view.theModel.getPartyMon(index));
        else
          openMonEditor(itemIsParty, view.theModel.getBoxMon(index));
      }
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
