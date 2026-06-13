// Rival.qml -- the rival editor screen.
//
// Edits the rival's name (NameDisplay, two-way bound to
// brg.file.data.dataExpanded.rival.name) and chosen starter (a flat ComboBox over
// brg.starterModel that sets rival.starter -- which determines the rival's team).
//
// Laid out like the Trainer Card (CardFront): a centered bordered card with a
// single shared fieldH height knob and a divider under the name, so the boxes stay
// compact and the rows line up regardless of the Qt 6 Material control height.
// The rival has only a couple of fields, so they stay in a simple vertical stack
// (name -> image -> starter) rather than the trainer card's two columns. The footer
// "Re-Roll" randomizes the rival. All bindings guard against a null rival (no save
// loaded yet).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  // The rival object, or null before a file is open. Always go through this
  // (never dereference the chain blind) so the screen is safe during load/reset.
  function rival() {
    return brg.file.data.dataExpanded
         ? brg.file.data.dataExpanded.rival
         : null;
  }

  // Centered card, matching the Trainer Card's grey box.
  Rectangle {
    id: card
    anchors.centerIn: parent
    width: 360
    height: 300

    border.color: brg.settings.textColorMid
    color: "transparent"

    // One height knob for every editable field on the card (same pattern as
    // CardFront): Qt 6 Material gives ComboBox/TextField a tall implicit height,
    // so pin an explicit shared height to keep the field compact. See
    // notes/reference/ui-patterns.md.
    property int fieldH: 28

    NameDisplay {
      id: rivalNameEdit

      anchors.top: parent.top
      anchors.topMargin: 33
      anchors.horizontalCenter: parent.horizontalCenter

      isPersonName: true
      isPlayerName: false

      onStrChanged: {
        let r = top.rival();
        if(r) r.name = str;
      }

      Connections {
        target: top.rival()
        function onNameChanged() {
          let r = top.rival();
          if(r) rivalNameEdit.str = r.name;
        }
      }

      Component.onCompleted: {
        let r = top.rival();
        if(r) rivalNameEdit.str = r.name;
      }
    }

    // Divider under the name (matches CardFront's spacer).
    Spacer {
      id: divider

      anchors.top: rivalNameEdit.bottom
      anchors.topMargin: 25
      anchors.left: parent.left

      width: parent.width
      height: 1
      border.color: brg.settings.dividerColor
    }

    Image {
      source: "qrc:/assets/icons/rival.png"

      anchors.top: divider.bottom
      anchors.topMargin: 15
      anchors.bottom: starterRow.top
      anchors.bottomMargin: 15
      anchors.horizontalCenter: parent.horizontalCenter

      width: card.width / 3
      fillMode: Image.PreserveAspectFit
    }

    // Starter label + combo, centered as a unit (no magic offset).
    Row {
      id: starterRow

      anchors.bottom: parent.bottom
      anchors.bottomMargin: 25
      anchors.horizontalCenter: parent.horizontalCenter

      spacing: 7

      Label {
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 14
        text: qsTr("Starter")
      }

      ComboBox {
        id: rivalStarterEdit
        height: card.fieldH
        width: font.pixelSize * 10 // Starter name is max of 10 chars

        textRole: "monName"
        valueRole: "monInd"

        font.capitalization: Font.Capitalize
        font.pixelSize: 14
        flat: true
        model: brg.starterModel

        // Borderless: clean look, no frame around the combo (Twilight's call s13n).
        background: Rectangle { color: "transparent"; border.width: 0 }

        onActivated: {
          let r = top.rival();
          if(r) r.starter = currentValue;
        }

        Component.onCompleted: {
          let r = top.rival();
          if(r) currentIndex = brg.starterModel.valToIndex(r.starter);
        }

        Connections {
          target: top.rival()
          function onStarterChanged() {
            let r = top.rival();
            if(r) rivalStarterEdit.currentIndex = brg.starterModel.valToIndex(r.starter);
          }
        }

        MainToolTip {
          text: qsTr("Set at start of game and determines your rivals team, namely which Pokemon he has growing with him.")
        }
      }
    }
  }

  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      let r = top.rival();
      if(r) r.randomize();
    }
  }
}
