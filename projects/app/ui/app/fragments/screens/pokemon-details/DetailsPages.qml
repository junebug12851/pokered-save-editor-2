import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

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

    font.capitalization: Font.Capitalize

    TabButton {
      text: "General"
    }
    TabButton {
      text: "DV/EV"
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
