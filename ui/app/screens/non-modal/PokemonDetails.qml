import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/pokemon"

Page {
  property PokemonBox boxData: null
  property PokemonBox partyData: null

  Text {
    anchors.centerIn: parent
    text: boxData.nickname
  }
}
