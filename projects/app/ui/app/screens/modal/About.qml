// About.qml -- the "About / Credits" modal.
//
// A ListView over brg.creditsModel rendering each credit entry's fields when
// present (section header, creditTo, mandated attribution, urlTo, license, note).
// ModalClose dismisses via the router; CreditWork attributes the wallpaper art.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/modal"

Page {
  id: top

  Image {
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent
    source: "qrc:/assets/wallpaper/starters.jpg"
    opacity: .35
  }

  ListView {
      id: creditsView
      clip: true
      model: brg.creditsModel
      anchors.fill: parent

      ScrollBar.vertical: ScrollBar {}

      delegate: ColumnLayout {

        property bool isLast: index+1 < creditsView.count ? false : true

        spacing: 0
        // parent can be briefly null during model reset in Qt 6 (see qt6-patterns.md)
        width: parent ? parent.width : 0

        Text {
          visible: section !== ""
          text: section
          font.pixelSize: 20
          Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
          topPadding: 30
        }

        Text {
          visible: creditTo !== ""
          text: creditTo
          font.pixelSize: 15
          Layout.alignment: Qt.AlignVCenter
          topPadding: 30
          leftPadding: 15
        }

        Text {
          visible: mandated !== ""
          text: "\" " + mandated + " \""
          font.pixelSize: 13
          Layout.alignment: Qt.AlignVCenter
          leftPadding: 35
          topPadding: 10
        }

        Text {
          visible: urlTo !== ""
          text: urlTo
          font.pixelSize: 13
          font.italic: true
          Layout.alignment: Qt.AlignVCenter
          color: Qt.lighter(brg.settings.textColorDark, 1.25)
          leftPadding: 35
          topPadding: 10
        }

        Text {
          visible: license !== ""
          text: license
          font.pixelSize: 13
          font.italic: true
          Layout.alignment: Qt.AlignVCenter
          color: Qt.lighter(brg.settings.textColorDark, 1.25)
          leftPadding: 35
          topPadding: 10
        }

        Text {
          visible: note !== ""
          text: note
          font.pixelSize: 14
          Layout.alignment: Qt.AlignVCenter
          color: Qt.lighter(brg.settings.textColorDark, 1.25)
          leftPadding: 35
          topPadding: 10
        }

        Text {
          visible: isLast
          bottomPadding: 75
        }
      }
    }

    ModalClose {
      onClicked: brg.router.closeScreen();
      anchors.topMargin: 12
      anchors.rightMargin: 12
    }

    CreditWork {
      text: "\"Basic Pokemons Colors\" by yoshiyaki (CC-BY-NC-ND 3.0)\n" +
            "https://www.deviantart.com/yoshiyaki/art/Basic-Pokemons-Colors-574585879"
    }
}
