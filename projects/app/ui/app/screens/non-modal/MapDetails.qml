import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top
  property int mapInd: 0

  GridView {
    id: view
    anchors.fill: parent
    property int cellSize: 100

    cellWidth: cellSize
    cellHeight: cellSize
    clip: true

    delegate: Button {
      property int cellSize: view.cellSize

      text: name
      width: cellSize
      height: cellSize

      icon.source: iconSrc
      icon.width: 45
      icon.height: 45
      icon.color: "transparent"

      font.pixelSize: 15
      font.capitalization: Font.Capitalize

      flat: true
      display: AbstractButton.TextUnderIcon
      padding: 20

      //onClicked: brg.router.changeScreen(page)
    }

    model: ListModel {
      id: iconsModel

      ListElement {
        name: "Loaded"
        iconSrc: "qrc:/assets/icons/fontawesome/map.svg"
      }

      ListElement {
        name: "General"
        iconSrc: "qrc:/assets/icons/fontawesome/cog.svg"
      }

      ListElement {
        name: "Events"
        iconSrc: "qrc:/assets/icons/fontawesome/globe-americas.svg"
      }

      ListElement {
        name: "Trades"
        iconSrc: "qrc:/assets/icons/fontawesome/hand-holding-heart.svg"
      }

      ListElement {
        name: "Missables"
        iconSrc: "qrc:/assets/icons/fontawesome/eye-slash.svg"
      }
    }

    ScrollBar.vertical: ScrollBar {}
  }

  Component.onCompleted: {
    // Daycare (Pokemon in Daycare)
    if(mapInd === 72)
      iconsModel.append({
                          name: "Pokemon",
                          iconSrc: "qrc:/assets/icons/fontawesome/baby-carriage.svg"
                        });

    // Vermilion Gym (Lt. Surge Switches)
    else if(mapInd === 92)
      iconsModel.append({
                          name: "Switches",
                          iconSrc: "qrc:/assets/icons/fontawesome/trash-restore.svg"
                        });

    // Cinnabar Gym (Quiz Answers Next Opp)
    else if(mapInd === 166)
      iconsModel.append({
                          name: "Quiz",
                          iconSrc: "qrc:/assets/icons/fontawesome/gamepad.svg"
                        });

    // Safari Zone Related Maps (Various Safari Zone Variables)
    else if(mapInd === 156 || // Entrance
            mapInd === 217 || // East
            mapInd === 218 || // North
            mapInd === 219 || // West
            mapInd === 220 || // Center
            mapInd === 221 || // Center Rest house
            mapInd === 222 || // Secret House
            mapInd === 223 || // West Rest house
            mapInd === 224 || // East Rest house
            mapInd === 225    // North Rest house
            )
      iconsModel.append({
                          name: "Park",
                          iconSrc: "qrc:/assets/icons/fontawesome/kiwi-bird.svg"
                        });
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    //onBtn1Clicked: brg.file.data.randomizeExpansion()
  }
}
