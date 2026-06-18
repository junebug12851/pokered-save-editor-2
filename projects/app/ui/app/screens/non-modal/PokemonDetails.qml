// PokemonDetails.qml -- the single-Pokemon deep editor screen.
//
// Edits one Pokemon (boxData, a PokemonBox passed in by the caller; partyData is
// the party-only sibling view). Left half is DetailsPages (the tabbed editors:
// species, moves, stats, etc.); right half is GlancePane (a live summary). On load
// it points brg.moveSelectModel at this mon's box. Footer: Re-Roll (whole mon) +
// Heal. Destructive actions gate behind one shared ConfirmPopup, reached via the
// `confirmAsk(title, body, action, label)` function threaded down to the children.
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

  // One shared confirmation modal for every destructive action in this editor. The
  // function is threaded down to DetailsPages/MovesTab/PokemonMoveSel so a single
  // popup serves them all.
  ConfirmPopup { id: confirm }
  property var confirmAsk: function(title, body, action, label) { confirm.ask(title, body, action, label); }

  DetailsPages {
    id: detailsPages

    boxData: top.boxData
    partyData: top.partyData
    confirmAsk: top.confirmAsk

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
      top.confirmAsk(qsTr("Re-Roll this Pokémon?"),
        qsTr("This replaces the WHOLE Pokémon with a fresh random one — species, moves, stats, everything. It can't be undone."),
        function() { boxData.randomize(brg.file.data.dataExpanded.player.basics); },
        qsTr("Re-Roll"));
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
