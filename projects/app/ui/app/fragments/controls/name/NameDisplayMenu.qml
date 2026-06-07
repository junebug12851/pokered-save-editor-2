// NameDisplayMenu.qml -- the right-click/overflow menu for a name display (with
// tileset submenu).
//
// A Menu offering Randomize Name (emits changeStr with a random player/Pokemon
// name), Toggle Example, Randomize Example (only when hasBox), the Simulated
// Tileset submenu (TilesetMenu), and Close. Compare NameDisplayMenuNoTileset,
// which drops the tileset submenu.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Menu {
  id: menu
  signal changeStr(string val);
  signal toggleExample();
  signal reUpdateExample();

  property bool isPersonName: false
  property bool hasBox: false

  MenuItem {
    text: "Randomize Name";
    onTriggered: {
      if(isPersonName)
        changeStr(brg.randomPlayerName.randomExample());
      else
        changeStr(brg.randomPokemonName.randomExample());
    }
  }

  MenuItem {
    text: "Toggle Example"
    onTriggered: toggleExample();
  }

  MenuItem {
    enabled: hasBox
    text: "Randomize Example"
    onTriggered: reUpdateExample();
  }

  TilesetMenu {
    title: "Simulated Tileset"
  }

  MenuItem {
    text: "Close"
    onTriggered: menu.close();
  }
}
