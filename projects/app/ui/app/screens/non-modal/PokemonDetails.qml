// PokemonDetails.qml -- the single-Pokemon deep editor screen.
//
// Edits one Pokemon (boxData, a PokemonBox passed in by the caller; partyData is
// the party-only sibling view). Left half is DetailsPages (the tabbed editors:
// species, moves, stats, etc.); right half is GlancePane (a live summary). On load
// it points brg.moveSelectModel at this mon's box. Footer: Re-Roll (randomize this
// mon), Heal, and a Toolkit menu (Max Out / Correct Data / Reset / Evolve /
// De-Evolve). The two // comments on btn2.enabled and update() document known QML
// quirks -- leave them; they are real workarounds, not dead code.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/pokemon-details"

Page {
  id: top

  property PokemonBox boxData: null
  property PokemonBox partyData: null

  Component.onCompleted: brg.moveSelectModel.monFromBox(boxData);

  DetailsPages {
    id: detailsPages

    boxData: top.boxData
    partyData: top.partyData

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    // Editor takes ~58%; the GlancePane sprite/stats get a wider ~42%.
    width: parent.width * .58
  }

  GlancePane {
    boxData: top.boxData
    partyData: top.partyData

    anchors.top: parent.top
    anchors.left: detailsPages.right
    anchors.bottom: parent.bottom
    anchors.right: parent.right
  }

  // Footer: Re-Roll + Heal. The old Toolkit menu (Max Out / Correct Data / Reset /
  // Evolve / De-Evolve) moved to the Moves tab's actions section.
  footer: AppFooterBtn2 {
    id: footer

    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      boxData.randomize(brg.file.data.dataExpanded.player.basics)
    }

    // Not going to use because of a glitch where once you click it and the pokemon
    // are healed it becomes disabled and stuck in the mouse-down look
    //btn2.enabled: boxData.isHealed
    icon2.source: "qrc:/assets/icons/fontawesome/magic.svg"
    text2: "Heal"
    onBtn2Clicked: {
      boxData.heal();
    }
  }
}
