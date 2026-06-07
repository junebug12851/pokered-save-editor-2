// Home.qml -- the home screen (the app's main menu).
//
// A Page showing the grid of navigation icons (IconsView driven by
// HomeIconsModel) that lead to the editor screens, with a one-button footer:
// "Re-Roll", which fully randomizes the save (brg.file.data.randomizeExpansion()).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/header"
import "../../fragments/general"
import "../../fragments/screens/home"

Page {
  id: page

  // Icons
  IconsView {
    anchors.fill: parent
    model: HomeIconsModel {}
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: brg.file.data.randomizeExpansion()
  }
}
