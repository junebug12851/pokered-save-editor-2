import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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
        changeStr(brg.randomPlayerName.randomName());
      else
        changeStr(brg.randomPokemonName.randomName());
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
    title: "Change Tileset"
  }

  MenuItem {
    text: "Close"
    onTriggered: menu.close();
  }
}
