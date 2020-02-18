import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/bag"

Page {
  id: page

  ItemsPane {
    id: bagPane

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: Math.trunc(parent.width * 0.50)

    title: "Bag"
    box: brg.file.data.dataExpanded.player.items
    model: brg.bagItemsModel
  }

  ItemsPane {
    id: storagePane

    anchors.left: bagPane.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.right: parent.right

    title: "Storage"
    box: brg.file.data.dataExpanded.storage.items
    model: brg.pcItemsModel
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      brg.file.data.dataExpanded.player.items.randomize()
      brg.file.data.dataExpanded.storage.items.randomize()
    }
  }
}
