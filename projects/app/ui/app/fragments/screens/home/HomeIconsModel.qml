// HomeIconsModel.qml -- the Home screen's menu grid model.
//
// A ListModel of {name, iconSrc, page, disabled} entries for the Home IconsView.
// `page` is the router screen key (e.g. "trainerCard", "pokedex", "bag"); entries
// with an empty page ("Options", "Hall of Fame", "Event Pokémon") are placeholders
// for not-yet-implemented screens. `disabled: true` greys the tile out and makes it
// non-clickable (see IconDelegate.qml) — used for not-yet-available screens.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ListModel {
  ListElement {
    name: "Trainer Card"
    iconSrc: "qrc:/assets/icons/poke-go/hat.svg"
    page: "trainerCard"
    disabled: false
  }
  ListElement {
    name: "Pokédex"
    iconSrc: "qrc:/assets/icons/poke-go/pokedex.svg"
    page: "pokedex"
    disabled: false
  }
  ListElement {
    name: "Items"
    iconSrc: "qrc:/assets/icons/poke-go/backpack.svg"
    page: "bag"
    disabled: false
  }
  ListElement {
    name: "Pokémon"
    iconSrc: "qrc:/assets/icons/poke-go/pikachu-2.svg"
    page: "pokemon"
    disabled: false
  }
  ListElement {
    name: "Rival"
    iconSrc: "qrc:/assets/icons/poke-go/player.svg"
    page: "rival"
    disabled: false
  }
  ListElement {
    name: "Maps"
    iconSrc: "qrc:/assets/icons/poke-go/map.svg"
    page: "maps"
    disabled: true
  }
  ListElement {
    name: "Options"
    iconSrc: "qrc:/assets/icons/poke-go/gear.svg"
    page: ""
    disabled: true
  }
  ListElement {
    name: "Hall of Fame"
    iconSrc: "qrc:/assets/icons/poke-go/crown.svg"
    page: ""
    disabled: true
  }
  ListElement {
    name: "Event Pokémon"
    iconSrc: "qrc:/assets/icons/poke-go/mew.svg"
    page: ""
    disabled: true
  }
  ListElement {
    name: "Market"
    iconSrc: "qrc:/assets/icons/poke-go/pokeballs.svg"
    page: "pokemart"
    disabled: false
  }
}
