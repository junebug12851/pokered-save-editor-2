// DetailsPages.qml -- the tabbed editor (left half of PokemonDetails).
//
// A TabBar (General / DV-EV / Moves) over a StackLayout of OverviewTab, StatsTab,
// and MovesTab, all bound to the mon being edited (boxData; partyData for the
// party view). This is the deep per-Pokemon editor; the right half is GlancePane.
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

    // Snug, content-width tabs (Material stretches them edge-to-edge by default,
    // which left huge gaps); padding gives a little breathing room without spreading.
    TabButton {
      text: qsTr("General")
      width: implicitContentWidth + 28
    }
    TabButton {
      text: qsTr("DV/EV")
      width: implicitContentWidth + 28
    }
    TabButton {
      text: qsTr("Moves")
      width: implicitContentWidth + 28
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
