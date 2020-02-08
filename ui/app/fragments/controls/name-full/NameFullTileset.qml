import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ComboBox {

  font.capitalization: Font.Capitalize
  font.pixelSize: 12
  flat: true
  width: font.pixelSize * 12

  model: [
    "Cavern",
    "Cemetery",
    "Club",
    "Dojo",
    "Facility",
    "Forest",
    "Forest Gate",
    "Gate",
    "Gym",
    "House",
    "Interior",
    "Lab",
    "Lobby",
    "Mansion",
    "Mart",
    "Museum",
    "Overworld",
    "Plateau",
    "Pokecenter",
    "Reds House 1",
    "Reds House 2",
    "Ship",
    "Ship Port",
    "Underground"
  ]

  onActivated: brg.settings.previewTileset = currentValue;
  Component.onCompleted: currentIndex = brg.settings.previewTilesetIndex;
}
