import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  ListView {
    id: mapsView
    clip: true
    model: brg.mapSelectModel
    anchors.fill: parent

    ScrollBar.vertical: ScrollBar {}

    delegate: ColumnLayout {

      property bool isLast: index+1 < mapsView.count ? false : true

      spacing: 0
      width: parent.width

      function convertName() {
        var pass1 = mapName.substring(4);
        return pass1.substring(0, pass1.length - 4);
      }

      Text {
        visible: mapInd < 0
        text: mapName === "phony"
              ? convertName()
              : convertName()
        font.pixelSize: 20
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        topPadding: 30
      }

      Button {
        visible: mapInd >= 0 && mapInd < 0xFF

        implicitWidth: parent.width / 2
        Layout.alignment: Qt.AlignHCenter
        text: mapName
        font.capitalization: Font.Capitalize
      }

      Text {
        visible: isLast
        bottomPadding: 75
      }
    }
  }

  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      // Most of the data on the screen
      brg.file.data.dataExpanded.area.randomize();

      // Playtime
      brg.file.data.dataExpanded.world.randomize();
    }
  }
}
