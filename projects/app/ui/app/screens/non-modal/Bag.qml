// Bag.qml -- the items editor screen.
//
// Two ItemsPane side by side (a 50/50 RowLayout): the player's Bag (bound to
// player.items / brg.bagItemsModel) and the PC Storage box
// (storage.items / brg.pcItemsModel). Footer buttons: Re-Roll (randomize both
// boxes) and Sort (sort both).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/bag"

Page {
  id: page

  // Two equal panes via an auto layout — each fillWidth so they split the
  // screen 50/50 with no manual width math.
  RowLayout {
    anchors.fill: parent
    spacing: 0

    ItemsPane {
      id: bagPane
      Layout.fillWidth: true
      Layout.fillHeight: true

      title: "Bag"
      box: brg.file.data.dataExpanded.player.items
      model: brg.bagItemsModel
    }

    ItemsPane {
      id: storagePane
      Layout.fillWidth: true
      Layout.fillHeight: true

      title: "Storage"
      box: brg.file.data.dataExpanded.storage.items
      model: brg.pcItemsModel
    }
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn2 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      brg.file.data.dataExpanded.player.items.randomize()
      brg.file.data.dataExpanded.storage.items.randomize()
    }

    icon2.source: "qrc:/assets/icons/fontawesome/sort-amount-up.svg"
    text2: "Sort"
    onBtn2Clicked: {
      brg.file.data.dataExpanded.player.items.sort();
      brg.file.data.dataExpanded.storage.items.sort();
    }
  }
}
