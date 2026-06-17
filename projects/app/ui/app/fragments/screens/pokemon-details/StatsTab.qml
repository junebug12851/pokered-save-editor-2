import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../general"
import "../../header"
import "./stats"

// StatsTab.qml -- the "DV/EV" tab of the Pokemon details editor.
//
// Bound to boxData. A DV/EV toggle switches between DvStatGroup and EvStatGroup;
// the toggle's overflow menu offers max/re-roll/reset for the active stat kind.
// Below is the Future Shiny row -- note the checkbox uses onToggled (not
// onCheckedChanged) so the shiny DV rewrite fires only on a real click, never on
// the programmatic sync. Leave that inline note.
//
// DV/EV tab. Laid out top-down in a ColumnLayout: toggle row, the (content-
// sized) stat group, then the Future-Shiny row — with a flexible spacer at the
// bottom so all slack collects there instead of as odd gaps between sections.
Rectangle {
  property PokemonBox boxData: null
  property PokemonBox partyData: null

  color: "transparent"

  ColumnLayout {
    anchors.fill: parent
    anchors.leftMargin: 5
    anchors.rightMargin: 5
    spacing: 4

    // ---- DV/EV toggle (centered) + its options ⋮ (beside it, doesn't shift
    //      the toggle's centering) ----
    Item {
      Layout.fillWidth: true
      Layout.topMargin: 12
      Layout.preferredHeight: statsTglBtn.height

      Button {
        id: statsTglBtn
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("DV")
        width: font.pixelSize * 5
        flat: true

        onClicked: {
          if(text == "DV")
            text = "EV";
          else
            text = "DV";
        }

        MainToolTip {
          text: (statsTglBtn.text == "DV")
                ? "These are set on capture and never change in-game, they help affect the stats making each Pokémon unique. HP is a 5th DV that's calculated from the other DV."
                : "These are zero on capture and increase after every battle until maxed out. They slowly increase based on each opponent defeated and help make each Pokémon unique."
        }
      }

      IconButtonSquare {
        anchors.left: statsTglBtn.right
        anchors.verticalCenter: statsTglBtn.verticalCenter
        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
        icon.color: brg.settings.textColorDark

        onClicked: (statsTglBtn.text === "DV")
                   ? statsTglBtnMenuDV.open()
                   : statsTglBtnMenuEV.open()

        Menu {
          id: statsTglBtnMenuDV
          MenuItem { text: "Max DVs";  onTriggered: { boxData.maxDVs(); } enabled: !boxData.isMaxDVs }
          MenuItem { text: "Re-Roll DVs"; onTriggered: boxData.reRollDVs() }
          MenuItem { text: "Reset DVs"; onTriggered: { boxData.resetDVs(); } enabled: !boxData.isMinDVs }
          MenuSeparator {}
          MenuItem { text: "Close" }
        }

        Menu {
          id: statsTglBtnMenuEV
          MenuItem { text: "Max EVs"; onTriggered: { boxData.maxEVs(); } enabled: !boxData.isMaxEVs }
          MenuItem { text: "Re-Roll EVs"; onTriggered: boxData.reRollEVs() }
          MenuItem { text: "Reset EVs"; onTriggered: { boxData.resetEVs(); } enabled: !boxData.isMinEvs }
          MenuSeparator {}
          MenuItem { text: "Close" }
        }
      }
    }

    // ---- The stat sliders (content-sized; only the visible one takes space) ----
    DvStatGroup {
      visible: statsTglBtn.text == "DV"
      Layout.fillWidth: true
    }

    EvStatGroup {
      visible: statsTglBtn.text == "EV"
      Layout.fillWidth: true
    }

    // ---- Future Shiny (clean row: shaded label | checkbox | … | ⋮) ----
    RowLayout {
      id: futureShinyRow
      Layout.fillWidth: true
      Layout.preferredHeight: 30
      Layout.maximumHeight: 30
      spacing: 8

      Rectangle {
        Layout.preferredWidth: 110
        Layout.fillHeight: true
        color: Qt.lighter(brg.settings.textColorMid, 1.75)
        HeaderText { text: "Future Shiny" }
      }

      CheckBox {
        id: futureShinyEdit
        Layout.alignment: Qt.AlignVCenter
        padding: 0
        Layout.preferredHeight: 26
        Layout.minimumHeight: 0

        // onToggled (not onCheckedChanged) so this fires ONLY on a real user
        // click — never on the programmatic `checked = boxData.isShiny` sync.
        // makeShiny() rewrites DV bytes; we must not trigger it from just opening.
        onToggled: {
          if(checked)
            boxData.makeShiny();
          else
            boxData.unmakeShiny();
        }

        Component.onCompleted: checked = boxData.isShiny;

        Connections {
          target: boxData
          function onDvChanged() { futureShinyEdit.checked = boxData.isShiny; }
        }

        MainToolTip {
          text: qsTr("Shiny didn't exist in Gen 1, but Gen 2 provided a way to determine a shiny from Gen 1 data. This way has been kept up with ever since.")
        }
      }

      Item { Layout.fillWidth: true }

      IconButtonSquare {
        Layout.alignment: Qt.AlignVCenter
        icon.width: 7
        icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
        icon.color: brg.settings.textColorDark

        onClicked: futureShinyMenu.open();

        Menu {
          id: futureShinyMenu
          MenuItem {
            text: "Re-Roll Shiny";
            onTriggered: { boxData.rollShiny(); }
          }
          MenuItem {
            text: "Re-Roll Non-Shiny";
            onTriggered: { boxData.rollNonShiny(); }
          }
          MenuSeparator { }
          MenuItem {
            text: qsTr("Convert to Shiny")
            enabled: !boxData.isShiny
            onTriggered: { boxData.makeShiny(); }
          }
          MenuItem {
            text: qsTr("Convert to Non-Shiny")
            enabled: boxData.isShiny
            onTriggered: { boxData.unmakeShiny(); }
          }
          MenuSeparator { }
          MenuItem { text: "Close" }
        }
      }
    }

    // Absorb leftover height so the form stays top-aligned (no odd mid gaps).
    Item { Layout.fillWidth: true; Layout.fillHeight: true }
  }
}
