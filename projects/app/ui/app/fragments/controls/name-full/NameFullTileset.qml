// NameFullTileset.qml -- the full keyboard's borderless tileset picker combo.
//
// A flat, borderless ComboBox of named tilesets that sets
// brg.settings.previewTileset. This is the full-keyboard copy of
// SimulatedTilesetCombo -- keep its tileset list in sync with that one and
// TilesetMenu.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ComboBox {

  font.capitalization: Font.Capitalize
  font.pixelSize: 12
  flat: true
  width: font.pixelSize * 12

  // Borderless: clean look, no frame around the combo (Twilight's call s13n).
  background: Rectangle { color: "transparent"; border.width: 0 }

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
    "Pokécenter",
    "Reds House 1",
    "Reds House 2",
    "Ship",
    "Ship Port",
    "Underground"
  ]

  onActivated: brg.settings.previewTileset = currentValue;
  Component.onCompleted: currentIndex = brg.settings.previewTilesetIndex;
}

