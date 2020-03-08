import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0
import App.PokemonMove 1.0

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
