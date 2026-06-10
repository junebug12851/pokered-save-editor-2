// PokemonPane.qml -- one PC storage half (used twice on the Pokemon screen).
//
// Wraps a PokemonBoxView below a header bar (check-all toggle, a SelectPokemonBox
// box-switcher, and a "set as current box" dot button). Bound to a
// PokemonStorageModel (model) and its PokemonBoxSelectModel (selectModel).
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

  // ---- Header bar: [check-all]  box-switcher (set-current), centered group ----
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

    // "Set as current box" dot, just to the right of the switcher (shown only
    // when this pane isn't already the current box).
    IconButtonSquare {
      anchors.left: boxSwitcher.right
      anchors.leftMargin: 4
      anchors.verticalCenter: boxSwitcher.verticalCenter

      visible: (brg.file.data.dataExpanded.storage.curBox != top.model.curBox) &&
               (top.model.curBox >= 0)

      icon.source: "qrc:/assets/icons/fontawesome/dot-circle.svg"
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
