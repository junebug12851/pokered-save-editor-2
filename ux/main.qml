import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import "../src.js/data/pokemonDB.js" as PokemonDB

Rectangle {
  // Because Dark Theme has quite an ugly Brighter Shade which ruins
  // The look and feel and can make things more difficult to read/see
  Material.accent: Material.color(Material.Pink, Material.Shade500)

  color: Material.background

  Text {
    text: PokemonDB.pokemonDB.pokemon["abra"].type1.name;
  }
}
