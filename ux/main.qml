import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

//import "../src.js/data/pokemonDB.js" as PokemonDB
import com.gmail.junehanabi.pse.gamedata 1.0

Rectangle {
  // Because Dark Theme has quite an ugly Brighter Shade which ruins
  // The look and feel and can make things more difficult to read/see
  Material.accent: Material.color(Material.Pink, Material.Shade500)

  color: Material.background

  //Navigation {}

  Text {
    id: s
    text: JSON.parse(GameData.json("fly"))[0].name;
  }
}
