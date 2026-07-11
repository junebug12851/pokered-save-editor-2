// TrainerCard.qml -- the trainer-card editor screen.
//
// A SwipeView (non-interactive, single page for now) hosting CardFront, the
// trainer-card front face editor. Footer Re-Roll randomizes the player's basics
// (name, ID, money, badges, etc.) and the world playtime.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/trainer-card"

Page {
  id: top

  SwipeView {
    id: pageView
    anchors.centerIn: parent
    // Widened + made a touch taller (was 500x250) so the right-hand fields and the
    // Playtime group no longer crowd or overlap the trainer artwork on the left.
    width: 600
    height: 310
    clip: true
    interactive: false

    CardFront {}
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      // Most of the data on the screen
      brg.file.data.dataExpanded.player.basics.randomize();

      // Playtime
      brg.file.data.dataExpanded.world.other.randomizePlaytime();
    }
  }
}
