import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// SimulatedTilesetCombo.qml --
// The app-wide "simulated tileset" picker (drives brg.settings.previewTileset).
// Lives in general so both the full keyboard header and the quick-edit popup can
// reuse it. (Was name-full/NameFullTileset; moved here to avoid a general ->
// name-full import cycle.)
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
