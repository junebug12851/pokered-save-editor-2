import QtQuick 2.14
import QtCharts 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import App.PokemonBox 1.0
import App.PokemonParty 1.0

import "../../../general"
import "../../../header"

Rectangle {
  color: "transparent"

  Text {
    id: hpStatEvTxt

    anchors.top: parent.top
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "HP"
  }

  Slider {
    id: hpStatEvEdit

    anchors.top: parent.top
    anchors.topMargin: -15
    anchors.left: hpStatEvTxt.right
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10

    from: 0
    to: 0xFFFF
    snapMode: Slider.SnapAlways
    stepSize: 1

    ToolTip {
      parent: hpStatEvEdit.handle
      visible: hpStatEvEdit.pressed
      text: hpStatEvEdit.value.toFixed(0)

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      font.pixelSize: 14
    }

    onMoved: boxData.hpExp = value;
    Component.onCompleted: value = boxData.hpExp;

    Connections {
      target: boxData
      onHpExpChanged: hpStatEvEdit.value = boxData.hpExp;
    }
  }

  Text {
    id: atkStatEvTxt

    anchors.top: hpStatEvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Atk"
  }

  Slider {
    id: atkStatEvEdit

    anchors.top: hpStatEvEdit.bottom
    anchors.topMargin: -16
    anchors.left: atkStatEvTxt.right
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10

    from: 0
    to: 0xFFFF
    snapMode: Slider.SnapAlways
    stepSize: 1

    ToolTip {
      parent: atkStatEvEdit.handle
      visible: atkStatEvEdit.pressed
      text: atkStatEvEdit.value.toFixed(0)

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      font.pixelSize: 14
    }

    onMoved: boxData.atkExp = value;
    Component.onCompleted: value = boxData.atkExp;

    Connections {
      target: boxData
      onAtkExpChanged: atkStatEvEdit.value = boxData.atkExp;
    }
  }

  Text {
    id: defStatEvTxt

    anchors.top: atkStatEvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Def"
  }

  Slider {
    id: defStatEvEdit

    anchors.top: atkStatEvEdit.bottom
    anchors.topMargin: -16
    anchors.left: defStatEvTxt.right
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10

    from: 0
    to: 0xFFFF
    snapMode: Slider.SnapAlways
    stepSize: 1

    ToolTip {
      parent: defStatEvEdit.handle
      visible: defStatEvEdit.pressed
      text: defStatEvEdit.value.toFixed(0)

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      font.pixelSize: 14
    }

    onMoved: boxData.defExp = value;
    Component.onCompleted: value = boxData.defExp;

    Connections {
      target: boxData
      onDefExpChanged: defStatEvEdit.value = boxData.defExp;
    }
  }

  Text {
    id: spdStatEvTxt

    anchors.top: defStatEvTxt.bottom
    anchors.topMargin: 16
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Spd"
  }

  Slider {
    id: spdStatEvEdit

    anchors.top: defStatEvEdit.bottom
    anchors.topMargin: -15
    anchors.left: spdStatEvTxt.right
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10

    from: 0
    to: 0xFFFF
    snapMode: Slider.SnapAlways
    stepSize: 1

    ToolTip {
      parent: spdStatEvEdit.handle
      visible: spdStatEvEdit.pressed
      text: spdStatEvEdit.value.toFixed(0)

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      font.pixelSize: 14
    }

    onMoved: boxData.spdExp = value;
    Component.onCompleted: value = boxData.spdExp;

    Connections {
      target: boxData
      onSpdExpChanged: spdStatEvEdit.value = boxData.spdExp;
    }
  }

  Text {
    id: spStatEvTxt

    anchors.top: spdStatEvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Sp"
  }

  Slider {
    id: spStatEvEdit

    anchors.top: spdStatEvEdit.bottom
    anchors.topMargin: -16
    anchors.left: spStatEvTxt.right
    anchors.leftMargin: 10
    anchors.right: parent.right
    anchors.rightMargin: 10

    from: 0
    to: 0xFFFF
    snapMode: Slider.SnapAlways
    stepSize: 1

    ToolTip {
      parent: spStatEvEdit.handle
      visible: spStatEvEdit.pressed
      text: spStatEvEdit.value.toFixed(0)

      Material.background: brg.settings.accentColor
      Material.foreground: brg.settings.textColorLight

      font.pixelSize: 14
    }

    onMoved: boxData.spExp = value;
    Component.onCompleted: value = boxData.spExp;

    Connections {
      target: boxData
      onSpExpChanged: spStatEvEdit.value = boxData.spExp;
    }
  }
}
