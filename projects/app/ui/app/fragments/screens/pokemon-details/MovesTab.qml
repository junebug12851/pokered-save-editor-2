import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty
import App.PokemonMove

import "../../general"
import "../../header"
import "../../controls/selection"

Rectangle {
  id: top
  property PokemonBox boxData: null

  color: "transparent"

  ColumnLayout {
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right

    anchors.margins: 5

    PokemonMoveSel {
      Layout.fillWidth: true
      monMove: boxData.movesAt(0)
    }

    PokemonMoveSel {
      Layout.fillWidth: true
      monMove: boxData.movesAt(1)
    }

    PokemonMoveSel {
      Layout.fillWidth: true
      monMove: boxData.movesAt(2)
    }

    PokemonMoveSel {
      Layout.fillWidth: true
      monMove: boxData.movesAt(3)
    }
  }
}
