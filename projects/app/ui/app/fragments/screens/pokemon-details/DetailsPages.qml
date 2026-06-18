// DetailsPages.qml -- the tabbed editor (left half of PokemonDetails).
//
// A TabBar (General / DV-EV / Moves) over a StackLayout of OverviewTab, StatsTab,
// and MovesTab, all bound to the mon being edited (boxData; partyData for the
// party view). The header (accent) bar also carries the mon-wide action groups to
// the RIGHT of the tabs (these replaced the old footer Toolkit button): a Data
// group [Reset | Max Out | Correct Data] and an Evolution control [<- fish ->].
// This is the deep per-Pokemon editor; the right half is GlancePane.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../general"
import "../../header"

Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null
  property PokemonBox partyData: null

  // A flat icon button styled for the ACCENT header bar (light icon + light hover
  // wash + light divider), grouped into a connected bordered group like the tabs'
  // other controls. `first` omits the left divider.
  component HdrBtn: Button {
    id: hb
    property bool first: false
    property string tip: ""
    flat: true
    display: AbstractButton.IconOnly
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    padding: 7
    icon.color: brg.settings.textColorLight
    Layout.fillHeight: true
    Layout.minimumHeight: 0
    background: Rectangle {
      color: hb.down ? Qt.rgba(1, 1, 1, 0.28)
             : hb.hovered ? Qt.rgba(1, 1, 1, 0.16)
             : "transparent"
      Rectangle {
        visible: !hb.first
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(1, 1, 1, 0.30)
      }
    }
    MainToolTip { text: hb.tip }
  }

  TabBar {
    id: bar

    Material.background: brg.settings.accentColor
    Material.foreground: brg.settings.textColorLight
    Material.accent: brg.settings.textColorLight

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: 45

    font.capitalization: Font.Capitalize

    // Snug, content-width tabs (Material stretches them edge-to-edge by default,
    // which left huge gaps); padding gives a little breathing room without spreading.
    TabButton {
      text: qsTr("General")
      width: implicitContentWidth + 28
    }
    TabButton {
      text: qsTr("DV/EV")
      width: implicitContentWidth + 28
    }
    TabButton {
      text: qsTr("Moves")
      width: implicitContentWidth + 28
    }
  }

  // Mon-wide action groups, right-aligned on the accent header bar (over the empty
  // space past the tabs): Data [Reset | Max Out | Correct Data] and Evolution
  // [de-evolve <- | fish | -> evolve].
  RowLayout {
    anchors.right: bar.right
    anchors.rightMargin: 10
    anchors.verticalCenter: bar.verticalCenter
    height: 32
    z: 2
    spacing: 10

    // Data: Reset · Max Out · Correct Data.
    Rectangle {
      Layout.fillHeight: true
      implicitWidth: dataGrp.implicitWidth
      radius: 4; color: "transparent"
      border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.35)
      clip: true
      RowLayout {
        id: dataGrp
        anchors.fill: parent
        spacing: 0
        HdrBtn {
          first: true
          Layout.preferredWidth: 36
          icon.width: 18; icon.height: 18
          icon.source: "qrc:/assets/icons/fontawesome/down-left-and-up-right-to-center.svg"
          onClicked: if(top.boxData) top.boxData.resetPokemon();
          tip: qsTr("Reset this Pokémon back to a fresh level-5 (all moves + data).")
        }
        HdrBtn {
          Layout.preferredWidth: 36
          icon.width: 18; icon.height: 18
          icon.source: "qrc:/assets/icons/fontawesome/up-right-and-down-left-from-center.svg"
          onClicked: if(top.boxData) top.boxData.maxOut();
          tip: qsTr("Max out all stats, HP, move PP & PP-Ups, etc.")
        }
        HdrBtn {
          Layout.preferredWidth: 36
          icon.width: 20; icon.height: 18
          icon.source: "qrc:/assets/icons/fontawesome/file-circle-check.svg"
          onClicked: {
            if(top.boxData) {
              // (Kept from the old Toolkit: update()'s 5th bool is dropped by a QML
              // arg glitch, so call the moves correction explicitly.)
              top.boxData.update(true, true, true, true);
              top.boxData.correctMoves();
              top.boxData.cleanupMoves();
            }
          }
          tip: qsTr("Make all stats, HP, moves, types, etc. game-accurate.")
        }
      }
    }

    // Evolution: de-evolve <- [fish] -> evolve.
    Rectangle {
      Layout.fillHeight: true
      implicitWidth: evoGrp.implicitWidth
      radius: 4; color: "transparent"
      border.width: 1; border.color: Qt.rgba(1, 1, 1, 0.35)
      clip: true
      RowLayout {
        id: evoGrp
        anchors.fill: parent
        spacing: 0
        HdrBtn {
          first: true
          Layout.preferredWidth: 32
          icon.width: 15; icon.height: 18
          icon.source: "qrc:/assets/icons/fontawesome/arrow-left.svg"
          enabled: top.boxData ? top.boxData.hasDeEvolution : false
          onClicked: if(top.boxData) top.boxData.deEvolve();
          tip: qsTr("De-evolve this Pokémon.")
        }
        // Static "fish" centrepiece (a non-interactive icon so it tints light like
        // the buttons -- a Button.icon recolours reliably; an Image would stay dark).
        Item {
          Layout.preferredWidth: 40
          Layout.fillHeight: true
          Rectangle {
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            width: 1
            color: Qt.rgba(1, 1, 1, 0.30)
          }
          Button {
            anchors.centerIn: parent
            flat: true
            display: AbstractButton.IconOnly
            hoverEnabled: false   // decorative: no hover, and no onClicked
            padding: 0
            topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
            icon.color: brg.settings.textColorLight
            icon.width: 22; icon.height: 18
            icon.source: "qrc:/assets/icons/fontawesome/fish.svg"
            background: Item {}
          }
        }
        HdrBtn {
          Layout.preferredWidth: 32
          icon.width: 15; icon.height: 18
          icon.source: "qrc:/assets/icons/fontawesome/arrow-right.svg"
          enabled: top.boxData ? top.boxData.hasEvolution : false
          onClicked: if(top.boxData) top.boxData.evolve();
          tip: qsTr("Evolve this Pokémon.")
        }
      }
    }
  }

  StackLayout {
    anchors.top: bar.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom

    currentIndex: bar.currentIndex

    OverviewTab {
      boxData: top.boxData
    }

    StatsTab {
      boxData: top.boxData
      partyData: top.partyData
    }

    MovesTab {
      boxData: top.boxData
    }
  }
}
