import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../../general"
import "../../../header"

// EV (Stat Experience) editor: five 0–65535 sliders. Laid out with a 2-column
// GridLayout (label | slider) so rows align and space evenly without hand-tuned
// or negative margins. Each slider shows its exact value in a tooltip while
// dragged.
Rectangle {
  color: "transparent"
  implicitHeight: evGrid.implicitHeight

  GridLayout {
    id: evGrid
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.leftMargin: 5
    anchors.rightMargin: 10

    columns: 2
    columnSpacing: 10
    rowSpacing: 2

    // ---- HP ----
    Text {
      text: "HP"
      font.pixelSize: 14
      horizontalAlignment: Text.AlignRight
      Layout.preferredWidth: font.pixelSize * 3
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
    }
    Slider {
      id: hpStatEvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 0xFFFF
      snapMode: Slider.SnapAlways
      stepSize: 1

      ToolTip {
        parent: hpStatEvEdit.handle
        visible: hpStatEvEdit.pressed || hpStatEvEdit.hovered
        text: hpStatEvEdit.value.toFixed(0)
        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.hpExp = value; }
      Component.onCompleted: value = boxData.hpExp
      Connections {
        target: boxData
        function onHpExpChanged() { hpStatEvEdit.value = boxData.hpExp; }
      }
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
      id: atkStatEvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 0xFFFF
      snapMode: Slider.SnapAlways
      stepSize: 1

      ToolTip {
        parent: atkStatEvEdit.handle
        visible: atkStatEvEdit.pressed || atkStatEvEdit.hovered
        text: atkStatEvEdit.value.toFixed(0)
        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.atkExp = value; }
      Component.onCompleted: value = boxData.atkExp
      Connections {
        target: boxData
        function onAtkExpChanged() { atkStatEvEdit.value = boxData.atkExp; }
      }
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
      id: defStatEvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 0xFFFF
      snapMode: Slider.SnapAlways
      stepSize: 1

      ToolTip {
        parent: defStatEvEdit.handle
        visible: defStatEvEdit.pressed || defStatEvEdit.hovered
        text: defStatEvEdit.value.toFixed(0)
        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.defExp = value; }
      Component.onCompleted: value = boxData.defExp
      Connections {
        target: boxData
        function onDefExpChanged() { defStatEvEdit.value = boxData.defExp; }
      }
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
      id: spdStatEvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 0xFFFF
      snapMode: Slider.SnapAlways
      stepSize: 1

      ToolTip {
        parent: spdStatEvEdit.handle
        visible: spdStatEvEdit.pressed || spdStatEvEdit.hovered
        text: spdStatEvEdit.value.toFixed(0)
        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.spdExp = value; }
      Component.onCompleted: value = boxData.spdExp
      Connections {
        target: boxData
        function onSpdExpChanged() { spdStatEvEdit.value = boxData.spdExp; }
      }
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
      id: spStatEvEdit
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      from: 0; to: 0xFFFF
      snapMode: Slider.SnapAlways
      stepSize: 1

      ToolTip {
        parent: spStatEvEdit.handle
        visible: spStatEvEdit.pressed || spStatEvEdit.hovered
        text: spStatEvEdit.value.toFixed(0)
        Material.background: brg.settings.accentColor
        Material.foreground: brg.settings.textColorLight
        font.pixelSize: 14
        enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 70 } }
        exit: Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 70 } }
      }

      onMoved: { boxData.spExp = value; }
      Component.onCompleted: value = boxData.spExp
      Connections {
        target: boxData
        function onSpExpChanged() { spStatEvEdit.value = boxData.spExp; }
      }
    }
  }
}
