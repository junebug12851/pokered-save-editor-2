// NameDisplayMenuNoTileset.qml -- a name-display menu without the tileset submenu.
//
// Same as NameDisplayMenu (Randomize Name / Toggle Example / Randomize Example /
// Close) but omits the Simulated Tileset submenu, for contexts where tileset
// selection doesn't apply.
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

  MenuItem {
    text: "Close"
    onTriggered: menu.close();
  }
}
