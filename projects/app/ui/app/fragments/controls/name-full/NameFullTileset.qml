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
  id: combo

  /// Closed again -- the caller hands the keys back to the keyboard deck.
  signal menuClosed()

  /// Opened from the deck's Tab key (tile pages only). Taking focus is the point: with
  /// it, the combo's own key handling gives you Up/Down to walk the list and Enter to
  /// choose, so the picker is drivable without ever reaching for the mouse.
  function openMenu() {
    combo.forceActiveFocus();
    combo.popup.open();
  }

  // Tab again = "that one" -- it opened the menu, so it closes it the same way. (Enter
  // is handled by the ComboBox itself.)
  Keys.onPressed: (event) => {
    if(!combo.popup.visible)
      return;

    if(event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
      brg.settings.previewTileset = combo.currentValue;
      combo.popup.close();
      event.accepted = true;
    }
  }

  Connections {
    target: combo.popup
    function onClosed() { combo.menuClosed(); }
  }

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

