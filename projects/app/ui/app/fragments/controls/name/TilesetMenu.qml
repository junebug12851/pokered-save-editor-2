import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

Menu {
  title: "Change Tileset"

  MenuItem {
    text: "Treat as Outdoor?"
    checkable: true
    checked: brg.settings.previewOutdoor
    onTriggered: brg.settings.previewOutdoor = !brg.settings.previewOutdoor
  }

  MenuItem {
    text: "Cavern"
    enabled: brg.settings.previewTileset !== "Cavern"
    onTriggered: brg.settings.previewTileset = "Cavern"
  }

  MenuItem {
    text: "Cemetery"
    enabled: brg.settings.previewTileset !== "Cemetery"
    onTriggered: brg.settings.previewTileset = "Cemetery"
  }

  MenuItem {
    text: "Club"
    enabled: brg.settings.previewTileset !== "Club"
    onTriggered: brg.settings.previewTileset = "Club"
  }

  MenuItem {
    text: "Dojo"
    enabled: brg.settings.previewTileset !== "Dojo"
    onTriggered: brg.settings.previewTileset = "Dojo"
  }

  MenuItem {
    text: "Facility"
    enabled: brg.settings.previewTileset !== "Facility"
    onTriggered: brg.settings.previewTileset = "Facility"
  }

  MenuItem {
    text: "Forest"
    enabled: brg.settings.previewTileset !== "Forest"
    onTriggered: brg.settings.previewTileset = "Forest"
  }

  MenuItem {
    text: "Forest Gate"
    enabled: brg.settings.previewTileset !== "Forest Gate"
    onTriggered: brg.settings.previewTileset = "Forest Gate"
  }

  MenuItem {
    text: "Gate"
    enabled: brg.settings.previewTileset !== "Gate"
    onTriggered: brg.settings.previewTileset = "Gate"
  }

  MenuItem {
    text: "Gym"
    enabled: brg.settings.previewTileset !== "Gym"
    onTriggered: brg.settings.previewTileset = "Gym"
  }

  MenuItem {
    text: "House"
    enabled: brg.settings.previewTileset !== "House"
    onTriggered: brg.settings.previewTileset = "House"
  }

  MenuItem {
    text: "Interior"
    enabled: brg.settings.previewTileset !== "Interior"
    onTriggered: brg.settings.previewTileset = "Interior"
  }

  MenuItem {
    text: "Lab"
    enabled: brg.settings.previewTileset !== "Lab"
    onTriggered: brg.settings.previewTileset = "Lab"
  }

  MenuItem {
    text: "Lobby"
    enabled: brg.settings.previewTileset !== "Lobby"
    onTriggered: brg.settings.previewTileset = "Lobby"
  }

  MenuItem {
    text: "Mansion"
    enabled: brg.settings.previewTileset !== "Mansion"
    onTriggered: brg.settings.previewTileset = "Mansion"
  }

  MenuItem {
    text: "Mart"
    enabled: brg.settings.previewTileset !== "Mart"
    onTriggered: brg.settings.previewTileset = "Mart"
  }

  MenuItem {
    text: "Museum"
    enabled: brg.settings.previewTileset !== "Museum"
    onTriggered: brg.settings.previewTileset = "Museum"
  }

  MenuItem {
    text: "Overworld"
    enabled: brg.settings.previewTileset !== "Overworld"
    onTriggered: brg.settings.previewTileset = "Overworld"
  }

  MenuItem {
    text: "Plateau"
    enabled: brg.settings.previewTileset !== "Plateau"
    onTriggered: brg.settings.previewTileset = "Plateau"
  }

  MenuItem {
    text: "Pokecenter"
    enabled: brg.settings.previewTileset !== "Pokecenter"
    onTriggered: brg.settings.previewTileset = "Pokecenter"
  }

  MenuItem {
    text: "Reds House 1"
    enabled: brg.settings.previewTileset !== "Reds House 1"
    onTriggered: brg.settings.previewTileset = "Reds House 1"
  }

  MenuItem {
    text: "Reds House 2"
    enabled: brg.settings.previewTileset !== "Reds House 2"
    onTriggered: brg.settings.previewTileset = "Reds House 2"
  }

  MenuItem {
    text: "Ship"
    enabled: brg.settings.previewTileset !== "Ship"
    onTriggered: brg.settings.previewTileset = "Ship"
  }

  MenuItem {
    text: "Ship Port"
    enabled: brg.settings.previewTileset !== "Ship Port"
    onTriggered: brg.settings.previewTileset = "Ship Port"
  }

  MenuItem {
    text: "Underground"
    enabled: brg.settings.previewTileset !== "Underground"
    onTriggered: brg.settings.previewTileset = "Underground"
  }
}
