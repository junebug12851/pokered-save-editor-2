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
      MenuItem { text: "Max Out"; onTriggered: boxData.maxOut();
        MainToolTip {
          text: "Max out all stats, health, move pp & pp-up, etc..."
        }
      }
      MenuItem { text: "Correct Data"; onTriggered: {
          // Another QML glitch, I expanded update() to 5 bools but despite
          // calling it with 5 it only calls it with 4. I've tried everything
          // so I'm giving up and calling the 2nd function explicitly
          boxData.update(true, true, true, true);
          boxData.correctMoves();
          boxData.cleanupMoves();
        }

        MainToolTip {
          text: "Make all all stats, health, moves, types etc... Game Accurate"
        }
      }
      MenuItem { text: "Reset"; onTriggered: boxData.resetPokemon();
        MainToolTip {
          text: "Reset this Pokemon back to a level 5 Pokemon including all moves and other data."
        }
      }
      MenuItem { text: "Evolve"; onTriggered: boxData.evolve(); enabled: boxData.hasEvolution
        MainToolTip {
          text: "Evolve this Pokemon"
        }
      }
      MenuItem { text: "De-Evolve"; onTriggered: boxData.deEvolve(); enabled: boxData.hasDeEvolution
        MainToolTip {
          text: "De-Evolve this Pokemon"
        }
      }
      MenuSeparator { }
      MenuItem { text: "Close" }
    }
  }
}
