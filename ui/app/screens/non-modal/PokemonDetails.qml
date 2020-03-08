import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

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
    width: parent.width * .5
  }

  GlancePane {
    boxData: top.boxData
    partyData: top.partyData

    anchors.top: parent.top
    anchors.left: detailsPages.right
    anchors.bottom: parent.bottom
    anchors.right: parent.right
  }

  footer: AppFooterBtn3 {
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

    icon3.source: "qrc:/assets/icons/fontawesome/wrench.svg"
    text3: "Toolkit"
    onBtn3Clicked: {
      toolkitMenu.open();
    }

    Menu {
      id: toolkitMenu
      parent: footer.btn3
      MenuItem { text: "Max Out"; onTriggered: boxData.maxOut(); }
      MenuItem { text: "Correct Data"; onTriggered: boxData.update(true, true, true, true);}
      MenuItem { text: "Reset"; onTriggered: boxData.resetPokemon(); }
      MenuItem { text: "Evolve"; onTriggered: boxData.evolve(); enabled: boxData.hasEvolution }
      MenuItem { text: "De-Evolve"; onTriggered: boxData.deEvolve(); enabled: boxData.hasDeEvolution }
      MenuSeparator { }
      MenuItem { text: "Close" }
    }
  }
}
