// AppWindow.qml -- the always-present app body (the bottom of App.qml's stack).
//
// A Page with the shared AppHeader on top and an inner StackView (appBody) for
// the non-modal pages (Home, TrainerCard, Bag, ...). Like App.qml it mirrors the
// C++ Router, but for the *non-modal* navigation signals. Modals are handled one
// level up in App.qml.
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Controls

import "../fragments/header"

// App body is the initial modal, it allows non-modal app navigation
Page {
  id: appPage
  objectName: "appWindow"

  Material.background: brg.settings.textColorLight  // themed background

  // DEBUG: open a party mon's details editor directly (mirrors PokemonBoxView's
  // openMonEditor -- push PokemonDetails with a boxData). Used by the --screen
  // pokemonDetails debug-launch flag so automation can reach the details editor
  // without clicking a cell. The flag that calls it is compiled out of release.
  function debugOpenPartyDetails(index) {
    var mon = brg.file.data.dataExpanded.player.pokemon.partyAt(index);
    if(!mon) return false;
    appBody.push("qrc:/ui/app/screens/non-modal/PokemonDetails.qml",
                 { boxData: mon, partyData: mon });
    brg.router.manualStackPush("pokemonDetails");
    return true;
  }

  // App-wide header
  header: AppHeader {}

  // Body -- the inner stack of non-modal pages
  StackView {
    anchors.fill: parent
    id: appBody
    initialItem: "qrc:/ui/app/screens/non-modal/Home.qml"  // first page shown

    // Mirror the C++ Router's non-modal navigation onto this inner stack
    Connections {
      target: brg.router

      function onGoHome() { appBody.pop(null); }            // back to Home
      function onOpenNonModal(url) { appBody.push(url); }   // push a page
      function onCloseNonModal(url) { appBody.pop(url); }   // pop a page
    }
  }
}
