import QtQuick 2.14
import QtQuick.Controls 2.14

import "../../../common/Style.js" as Style

Menu {
  title: "Change Tileset"

  MenuItem {
    text: "Treat as Outdoor?"
    checkable: true
    checked: root.previewOutdoor
    onTriggered: root.previewOutdoor = !root.previewOutdoor
  }

  MenuItem {
    text: "Cavern"
    enabled: root.previewTileset !== "Cavern"
    onTriggered: root.previewTileset = "Cavern"
  }

  MenuItem {
    text: "Cemetery"
    enabled: root.previewTileset !== "Cemetery"
    onTriggered: root.previewTileset = "Cemetery"
  }

  MenuItem {
    text: "Club"
    enabled: root.previewTileset !== "Club"
    onTriggered: root.previewTileset = "Club"
  }

  MenuItem {
    text: "Dojo"
    enabled: root.previewTileset !== "Dojo"
    onTriggered: root.previewTileset = "Dojo"
  }

  MenuItem {
    text: "Facility"
    enabled: root.previewTileset !== "Facility"
    onTriggered: root.previewTileset = "Facility"
  }

  MenuItem {
    text: "Forest"
    enabled: root.previewTileset !== "Forest"
    onTriggered: root.previewTileset = "Forest"
  }

  MenuItem {
    text: "Forest Gate"
    enabled: root.previewTileset !== "Forest Gate"
    onTriggered: root.previewTileset = "Forest Gate"
  }

  MenuItem {
    text: "Gate"
    enabled: root.previewTileset !== "Gate"
    onTriggered: root.previewTileset = "Gate"
  }

  MenuItem {
    text: "Gym"
    enabled: root.previewTileset !== "Gym"
    onTriggered: root.previewTileset = "Gym"
  }

  MenuItem {
    text: "House"
    enabled: root.previewTileset !== "House"
    onTriggered: root.previewTileset = "House"
  }

  MenuItem {
    text: "Interior"
    enabled: root.previewTileset !== "Interior"
    onTriggered: root.previewTileset = "Interior"
  }

  MenuItem {
    text: "Lab"
    enabled: root.previewTileset !== "Lab"
    onTriggered: root.previewTileset = "Lab"
  }

  MenuItem {
    text: "Lobby"
    enabled: root.previewTileset !== "Lobby"
    onTriggered: root.previewTileset = "Lobby"
  }

  MenuItem {
    text: "Mansion"
    enabled: root.previewTileset !== "Mansion"
    onTriggered: root.previewTileset = "Mansion"
  }

  MenuItem {
    text: "Mart"
    enabled: root.previewTileset !== "Mart"
    onTriggered: root.previewTileset = "Mart"
  }

  MenuItem {
    text: "Museum"
    enabled: root.previewTileset !== "Museum"
    onTriggered: root.previewTileset = "Museum"
  }

  MenuItem {
    text: "Overworld"
    enabled: root.previewTileset !== "Overworld"
    onTriggered: root.previewTileset = "Overworld"
  }

  MenuItem {
    text: "Plateau"
    enabled: root.previewTileset !== "Plateau"
    onTriggered: root.previewTileset = "Plateau"
  }

  MenuItem {
    text: "Pokecenter"
    enabled: root.previewTileset !== "Pokecenter"
    onTriggered: root.previewTileset = "Pokecenter"
  }

  MenuItem {
    text: "Reds House 1"
    enabled: root.previewTileset !== "Reds House 1"
    onTriggered: root.previewTileset = "Reds House 1"
  }

  MenuItem {
    text: "Reds House 2"
    enabled: root.previewTileset !== "Reds House 2"
    onTriggered: root.previewTileset = "Reds House 2"
  }

  MenuItem {
    text: "Ship"
    enabled: root.previewTileset !== "Ship"
    onTriggered: root.previewTileset = "Ship"
  }

  MenuItem {
    text: "Underground"
    enabled: root.previewTileset !== "Underground"
    onTriggered: root.previewTileset = "Underground"
  }
}
