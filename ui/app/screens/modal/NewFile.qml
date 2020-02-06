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

  ModalClose {
    onClicked: brg.router.closeScreen();
  }
}
