// PokemonPane.qml -- one PC storage half (used twice on the Pokemon screen).
//
// Wraps a PokemonBoxView below a header bar (check-all toggle, a SelectPokemonBox
// box-switcher, and an "Active" toggle showing/setting whether this pane's box is
// the game's current/active box). Bound to a PokemonStorageModel (model) and its
// PokemonBoxSelectModel (selectModel).
//
// The old footer of bulk actions (move to top/up/down/bottom, transfer, release)
// was removed once drag & drop landed -- reordering and cross-pane moves are now
// direct drag gestures (PokemonBoxView), and delete moved to a per-mon
// hover/checked button on the cell. See notes/reference/ui-patterns.md.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonStorageModel
import App.PokemonBoxSelectModel

import "../../header"
import "../../general"
import "../../controls/selection"
import "../../screens/home"

Rectangle {
  id: top
  property PokemonStorageModel model: null
  property PokemonBoxSelectModel selectModel: null

  // ---- Header bar: [check-all]  box-switcher + Active toggle, centered group ----
  Rectangle {
    id: boxHeader
    anchors.left: parent.left
    anchors.top: parent.top
    anchors.right: parent.right
    height: 45

    Material.foreground: brg.settings.textColorLight
    Material.background: brg.settings.accentColor
    color: brg.settings.accentColor

    // Check-all parked at the left, nudged right so its icon centers over the
    // row checkboxes below (matches ItemsPane's header).
    IconButtonSquare {
      anchors.left: parent.left
      anchors.leftMargin: 24
      anchors.verticalCenter: parent.verticalCenter
      icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
      onClicked: model.checkedToggleAll()
    }

    // Box switcher centered in the bar.
    SelectPokemonBox {
      id: boxSwitcher
      anchors.centerIn: parent
      model: selectModel
      onActivated: top.model.switchBox(currentValue - 1);
      Component.onCompleted: currentIndex = top.model.curBox + 1;
    }

    // "Active" toggle, just to the right of the switcher. Shows whether the box
    // this pane displays is the game's current/active storage box: On (filled)
    // when it is, Off (outlined) when it isn't. Clicking an Off toggle makes
    // this box active; once On it is non-clickable (a save always has exactly
    // one active box, so you can only switch which box is active from elsewhere,
    // never turn the active box off). Hidden on the Party pane, which has no
    // active-box concept. Bar-appropriate colors (light on the accent header).
    FlatToggle {
      id: activeToggle
      anchors.left: boxSwitcher.right
      anchors.leftMargin: 10
      anchors.verticalCenter: boxSwitcher.verticalCenter

      text: qsTr("Active")
      visible: top.model.curBox >= 0
      active: brg.file.data.dataExpanded.storage.curBox === top.model.curBox
      enabled: !active

      toggleColor: brg.settings.textColorLight
      activeTextColor: brg.settings.accentColor
      inactiveTextColor: brg.settings.textColorLight
      hoverColor: Qt.rgba(1, 1, 1, 0.18)

      onClicked: brg.file.data.dataExpanded.storage.curBox = top.model.curBox;
    }
  }

  PokemonBoxView {
    id: pokemonView

    model: top.model

    anchors.top: boxHeader.bottom
    anchors.left: parent.left
    anchors.leftMargin: 15
    anchors.right: parent.right
    anchors.rightMargin: 15
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 15

    theModel: top.model
  }
}
