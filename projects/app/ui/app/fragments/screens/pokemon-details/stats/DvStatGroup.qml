import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../../general"
import "../../../header"

// DvStatGroup.qml -- the DV (IV) slider group for the StatsTab.
//
// DV (Determinant Value) editor: HP is read-only (Gen 1 computes it from the
// other four DVs); Atk/Def/Spd/Spc are user-editable 0–15 sliders.
//
// Laid out with a 3-column GridLayout (label | slider | readout) so rows align
// and space evenly on their own — no hand-tuned/negative margins. Each readout
// binds to its slider's value, which is the single source of truth (it updates
// both on user drag and on the boxData.onDvChanged sync), so the display is
// always correct without referencing unrelated properties.
Rectangle {
  color: "transparent"
  implicitHeight: dvGrid.implicitHeight

  GridLayout {
    id: dvGrid
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.leftMargin: 5
    anchors.rightMargin: 5

    columns: 3
    columnSpacing: 10
    rowSpacing: 2

    // ---- HP (read-only, computed) ----
    Text {
      text: "HP"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: hpStatDvEdit
      enabled: false
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 15
      snapMode: Slider.SnapAlways
      stepSize: 1

      Material.accent: brg.settings.textColorDark

      Behavior on value { NumberAnimation { duration: 250 } }

      Component.onCompleted: value = boxData.hpDV
      Connections {
        target: boxData
        function onDvChanged() { hpStatDvEdit.value = boxData.hpDV; }
      }
    }
    Text {
      text: hpStatDvEdit.value.toFixed(0)
      font.pixelSize: 14
      Layout.preferredWidth: font.pixelSize * 2
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    // ---- Atk ----
    Text {
      text: "Atk"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: atkStatDvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 15
      snapMode: Slider.SnapAlways
      stepSize: 1

      onMoved: { boxData.dvSet(0, value); }
      Component.onCompleted: value = boxData.dvAt(0)
      Connections {
        target: boxData
        function onDvChanged() { atkStatDvEdit.value = boxData.dvAt(0); }
      }
    }
    Text {
      text: atkStatDvEdit.value.toFixed(0)
      font.pixelSize: 14
      Layout.preferredWidth: font.pixelSize * 2
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    // ---- Def ----
    Text {
      text: "Def"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: defStatDvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 15
      snapMode: Slider.SnapAlways
      stepSize: 1

      onMoved: { boxData.dvSet(1, value); }
      Component.onCompleted: value = boxData.dvAt(1)
      Connections {
        target: boxData
        function onDvChanged() { defStatDvEdit.value = boxData.dvAt(1); }
      }
    }
    Text {
      text: defStatDvEdit.value.toFixed(0)
      font.pixelSize: 14
      Layout.preferredWidth: font.pixelSize * 2
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    // ---- Spd ----
    Text {
      text: "Spd"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: spdStatDvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 15
      snapMode: Slider.SnapAlways
      stepSize: 1

      onMoved: { boxData.dvSet(2, value); }
      Component.onCompleted: value = boxData.dvAt(2)
      Connections {
        target: boxData
        function onDvChanged() { spdStatDvEdit.value = boxData.dvAt(2); }
      }
    }
    Text {
      text: spdStatDvEdit.value.toFixed(0)
      font.pixelSize: 14
      Layout.preferredWidth: font.pixelSize * 2
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }

    // ---- Sp ----
    Text {
      text: "Sp"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: spStatDvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 15
      snapMode: Slider.SnapAlways
      stepSize: 1

      onMoved: { boxData.dvSet(3, value); }
      Component.onCompleted: value = boxData.dvAt(3)
      Connections {
        target: boxData
        function onDvChanged() { spStatDvEdit.value = boxData.dvAt(3); }
      }
    }
    Text {
      text: spStatDvEdit.value.toFixed(0)
      font.pixelSize: 14
      Layout.preferredWidth: font.pixelSize * 2
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    }
  }
}
