// Pokemon.qml -- the Pokemon storage-box editor screen.
//
// Two PokemonPane side by side, each over one of the two PC storage halves
// (brg.pokemonStorageModel1/2 with the matching pokemonBoxSelectModel1/2). The
// footer has Re-Roll (randomizes both the storage boxes and the party, seeded by
// player.basics) and a "Boxes Setup" toggle that flips storage.boxesFormatted
// (its icon reflects the current formatted state). Detailed per-mon editing
// happens on PokemonDetails.qml, navigated to from a pane.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/pokemon"

Page {

  PokemonPane {
    id: pane1

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: Math.trunc(parent.width * 0.50)

    model: brg.pokemonStorageModel1
    selectModel: brg.pokemonBoxSelectModel1
  }

  PokemonPane {
    id: pane2

    anchors.left: pane1.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.right: parent.right

    model: brg.pokemonStorageModel2
    selectModel: brg.pokemonBoxSelectModel2
  }

  footer: AppFooterBtn2 {

    // Randomize Pokemon
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      brg.file.data.dataExpanded.storage.randomizePokemon(brg.file.data.dataExpanded.player.basics);
      brg.file.data.dataExpanded.player.pokemon.randomize(brg.file.data.dataExpanded.player.basics);
    }

    icon2.source: (brg.file.data.dataExpanded.storage.boxesFormatted)
                  ? "qrc:/assets/icons/fontawesome/check-circle.svg"
                  : "qrc:/assets/icons/fontawesome/times-circle.svg"

    text2: "Boxes Setup"
    onBtn2Clicked: brg.file.data.dataExpanded.storage.boxesFormatted = !brg.file.data.dataExpanded.storage.boxesFormatted;
  }
}
