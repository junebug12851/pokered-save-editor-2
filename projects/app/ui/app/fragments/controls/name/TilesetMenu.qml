// TilesetMenu.qml -- the "Simulated Tileset" submenu of named tilesets.
//
// A Menu with an outdoor/indoor toggle plus one checkable item per tileset; each
// sets brg.settings.previewTileset (the current one is disabled). This is the menu
// form of SimulatedTilesetCombo -- keep the two tileset lists in sync.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Menu {
  title: qsTr("Simulated Tileset")

  // Tri-state, like every other one of these: Indoor -> Cave -> Outdoor. (The label IS the
  // state.) See notes/reference/tiles.md.
  MenuItem {
    text: qsTr("Simulate: %1").arg(brg.settings.previewTilesetTypeName)
    onTriggered: brg.settings.cyclePreviewTilesetType()
  }

  MenuItem {
    text: qsTr("Cavern")
    enabled: brg.settings.previewTileset !== "Cavern"
    onTriggered: brg.settings.previewTileset = "Cavern"
  }

  MenuItem {
    text: qsTr("Cemetery")
    enabled: brg.settings.previewTileset !== "Cemetery"
    onTriggered: brg.settings.previewTileset = "Cemetery"
  }

  MenuItem {
    text: qsTr("Club")
    enabled: brg.settings.previewTileset !== "Club"
    onTriggered: brg.settings.previewTileset = "Club"
  }

  MenuItem {
    text: qsTr("Dojo")
    enabled: brg.settings.previewTileset !== "Dojo"
    onTriggered: brg.settings.previewTileset = "Dojo"
  }

  MenuItem {
    text: qsTr("Facility")
    enabled: brg.settings.previewTileset !== "Facility"
    onTriggered: brg.settings.previewTileset = "Facility"
  }

  MenuItem {
    text: qsTr("Forest")
    enabled: brg.settings.previewTileset !== "Forest"
    onTriggered: brg.settings.previewTileset = "Forest"
  }

  MenuItem {
    text: qsTr("Forest Gate")
    enabled: brg.settings.previewTileset !== "Forest Gate"
    onTriggered: brg.settings.previewTileset = "Forest Gate"
  }

  MenuItem {
    text: qsTr("Gate")
    enabled: brg.settings.previewTileset !== "Gate"
    onTriggered: brg.settings.previewTileset = "Gate"
  }

  MenuItem {
    text: qsTr("Gym")
    enabled: brg.settings.previewTileset !== "Gym"
    onTriggered: brg.settings.previewTileset = "Gym"
  }

  MenuItem {
    text: qsTr("House")
    enabled: brg.settings.previewTileset !== "House"
    onTriggered: brg.settings.previewTileset = "House"
  }

  MenuItem {
    text: qsTr("Interior")
    enabled: brg.settings.previewTileset !== "Interior"
    onTriggered: brg.settings.previewTileset = "Interior"
  }

  MenuItem {
    text: qsTr("Lab")
    enabled: brg.settings.previewTileset !== "Lab"
    onTriggered: brg.settings.previewTileset = "Lab"
  }

  MenuItem {
    text: qsTr("Lobby")
    enabled: brg.settings.previewTileset !== "Lobby"
    onTriggered: brg.settings.previewTileset = "Lobby"
  }

  MenuItem {
    text: qsTr("Mansion")
    enabled: brg.settings.previewTileset !== "Mansion"
    onTriggered: brg.settings.previewTileset = "Mansion"
  }

  MenuItem {
    text: qsTr("Mart")
    enabled: brg.settings.previewTileset !== "Mart"
    onTriggered: brg.settings.previewTileset = "Mart"
  }

  MenuItem {
    text: qsTr("Museum")
    enabled: brg.settings.previewTileset !== "Museum"
    onTriggered: brg.settings.previewTileset = "Museum"
  }

  MenuItem {
    text: qsTr("Overworld")
    enabled: brg.settings.previewTileset !== "Overworld"
    onTriggered: brg.settings.previewTileset = "Overworld"
  }

  MenuItem {
    text: qsTr("Plateau")
    enabled: brg.settings.previewTileset !== "Plateau"
    onTriggered: brg.settings.previewTileset = "Plateau"
  }

  MenuItem {
    text: qsTr("Pokécenter")
    enabled: brg.settings.previewTileset !== "Pokécenter"
    onTriggered: brg.settings.previewTileset = "Pokécenter"
  }

  MenuItem {
    text: qsTr("Reds House 1")
    enabled: brg.settings.previewTileset !== "Reds House 1"
    onTriggered: brg.settings.previewTileset = "Reds House 1"
  }

  MenuItem {
    text: qsTr("Reds House 2")
    enabled: brg.settings.previewTileset !== "Reds House 2"
    onTriggered: brg.settings.previewTileset = "Reds House 2"
  }

  MenuItem {
    text: qsTr("Ship")
    enabled: brg.settings.previewTileset !== "Ship"
    onTriggered: brg.settings.previewTileset = "Ship"
  }

  MenuItem {
    text: qsTr("Ship Port")
    enabled: brg.settings.previewTileset !== "Ship Port"
    onTriggered: brg.settings.previewTileset = "Ship Port"
  }

  MenuItem {
    text: qsTr("Underground")
    enabled: brg.settings.previewTileset !== "Underground"
    onTriggered: brg.settings.previewTileset = "Underground"
  }
}
