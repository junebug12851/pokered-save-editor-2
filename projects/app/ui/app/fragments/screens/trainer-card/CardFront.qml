// CardFront.qml -- the front face of the trainer card (the TrainerCard screen
// body).
//
// Lays out all the trainer-card editors: player name, ID, money, coins, starter,
// playtime, the Red artwork, and the gym-badge grid (ListBadges). The shared
// fieldH knob pins every Material field to one height so labels line up (see
// ui-patterns.md). Fields are anchored below one another (not at fixed offsets) so
// rows never overlap regardless of control height.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Rectangle {
  id: cardFront
  border.color: brg.settings.textColorMid
  color: "transparent"

  // One height knob for every editable field on the card. Qt 6 Material gives
  // TextField/ComboBox a tall implicit height; setting an explicit, shared height
  // (the documented "explicit height knob" pattern — see ui-patterns.md) shrinks
  // the boxes and keeps every row the same height so labels line up with text.
  property int fieldH: 28

  PlayerNameEdit {
    id: playerNameEdit

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.leftMargin: 15
    anchors.topMargin: 33
  }

  PlayerIdEdit {
    id: playerIdEdit
    width: starterEdit.width
    height: cardFront.fieldH

    anchors.bottom: spacer.top
    anchors.bottomMargin: 13
    anchors.right: parent.right
    // This is the anchor the whole right-hand column right-aligns to (Money, Coins,
    // Starter, Playtime all sit at playerIdEdit.left / starterEdit.right). The
    // margin leaves room to the right for the fields' [dice | trash] action groups
    // so they stay inside the card border.
    anchors.rightMargin: 92
  }

  Spacer {
    id: spacer

    anchors.top: playerNameEdit.bottom
    anchors.topMargin: 20
    anchors.left: parent.left

    width: parent.width
    height: 1
    border.color: brg.settings.dividerColor
  }

  MoneyEdit {
    id: moneyEdit

    // Start the right-hand column a little higher than before to make vertical
    // room for the Enabled/Paused toggle row added above the playtime clock.
    anchors.top: spacer.top
    anchors.topMargin: 10
    anchors.left: playerIdEdit.left

    width: starterEdit.width
    height: cardFront.fieldH
  }

  CoinsEdit {
    id: coinsEdit

    // Anchor BELOW the previous field (not a fixed offset from its top) so the
    // rows never overlap regardless of the Material TextField's height.
    anchors.top: moneyEdit.bottom
    anchors.topMargin: 4
    anchors.left: moneyEdit.left

    width: starterEdit.width
    height: cardFront.fieldH
  }

  StarterEdit {
    id: starterEdit

    anchors.top: coinsEdit.bottom
    anchors.topMargin: 4

    anchors.left: coinsEdit.left
    width: font.pixelSize * 10 // Starter name is max of 10 chars
    height: cardFront.fieldH
  }

  // Enabled / Paused toggle pair, sitting just above the playtime clock and
  // right-aligned to it.
  PlaytimeToggles {
    id: playtimeToggles

    anchors.top: starterEdit.bottom
    anchors.topMargin: 8
    anchors.right: starterEdit.right
  }

  PlaytimeEdit {
    id: playtimeEdit
    fieldH: cardFront.fieldH

    anchors.top: playtimeToggles.bottom
    anchors.topMargin: 5
    anchors.right: starterEdit.right
    anchors.rightMargin: 0
  }

  Image {
    source: "qrc:/assets/icons/trainer.png"

    anchors.left: parent.left
    anchors.leftMargin: 75

    anchors.top: spacer.bottom
    anchors.topMargin: 10

    anchors.bottom: parent.bottom
    anchors.bottomMargin: 10

    width: parent.width / 3

    fillMode: Image.PreserveAspectFit
  }

  ListBadges {
    id: badges

    anchors.left: parent.left
    anchors.leftMargin: 10

    anchors.top: spacer.bottom
    anchors.topMargin: 20

    cellSize: 35
    width: cellSize * 2
    height: cellSize * 4
  }
}
