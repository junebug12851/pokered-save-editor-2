import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../general"
import "../../header"

Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null
  property PokemonBox partyData: null

  TabBar {
    id: bar

    Material.background: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    Material.accent: brg.settings.textColorLight

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 45

    TabButton {
      text: "Overview"
    }
    TabButton {
      text: "Stats"
    }
    TabButton {
      text: "Moves"
    }
  }

  StackLayout {
    anchors.top: bar.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom

    currentIndex: bar.currentIndex

    OverviewTab {
      boxData: top.boxData
    }

    StatsTab {
      boxData: top.boxData
      partyData: top.partyData
    }

    MovesTab {
      boxData: top.boxData
    }
  }
}
