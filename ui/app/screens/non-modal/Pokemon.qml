import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
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
