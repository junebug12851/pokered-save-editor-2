import QtQuick 2.14
import QtQuick.Controls 2.14

import "../../../common/Style.js" as Style

Menu {
  signal changeStr(string val);
  signal toggleExample();
  signal reUpdateExample();

  property bool isPersonName: false
  property bool hasBox: false

  MenuItem {
    text: "Randomize Name";
    onTriggered: {
      if(isPersonName)
        changeStr(randomPlayerName.randomName());
      else
        changeStr(randomPokemonName.randomName());
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
