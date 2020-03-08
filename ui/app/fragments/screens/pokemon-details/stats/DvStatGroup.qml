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
    id: hpStatDvTxt

    anchors.top: parent.top
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "HP"
  }

  Slider {
    id: hpStatDvEdit
    enabled: false

    anchors.top: parent.top
    anchors.topMargin: -15
    anchors.left: hpStatDvTxt.right
    anchors.leftMargin: 10
    anchors.right: hpStatDvReadout.left
    anchors.rightMargin: 10

    from: 0
    to: 15
    snapMode: Slider.SnapAlways
    stepSize: 1

    Material.accent: brg.settings.textColorDark

    Behavior on value {
      NumberAnimation { duration: 250 }
    }

    Component.onCompleted: value = boxData.hpDV;

    Connections {
      target: boxData
      onDvChanged: hpStatDvEdit.value = boxData.hpDV;
    }
  }

  Text {
    id: hpStatDvReadout

    width: font.pixelSize * 2

    anchors.top: parent.top
    anchors.right: parent.right

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (hpStatDvEdit.value === "phony")
          ? boxData.hpDV
          : boxData.hpDV
  }

  Text {
    id: atkStatDvTxt

    anchors.top: hpStatDvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Atk"
  }

  Slider {
    id: atkStatDvEdit

    anchors.top: hpStatDvEdit.bottom
    anchors.topMargin: -16
    anchors.left: atkStatDvTxt.right
    anchors.leftMargin: 10
    anchors.right: atkStatDvReadout.left
    anchors.rightMargin: 10

    from: 0
    to: 15
    snapMode: Slider.SnapAlways
    stepSize: 1

    onMoved: boxData.dvSet(0, value);
    Component.onCompleted: value = boxData.dvAt(0);

    Connections {
      target: boxData
      onDvChanged: atkStatDvEdit.value = boxData.dvAt(0);
    }
  }

  Text {
    id: atkStatDvReadout

    width: font.pixelSize * 2

    anchors.top: hpStatDvReadout.bottom
    anchors.topMargin: 15
    anchors.right: parent.right

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (boxData.hpDV === "phony")
          ? boxData.dvAt(0)
          : boxData.dvAt(0)
  }

  Text {
    id: defStatDvTxt

    anchors.top: atkStatDvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Def"
  }

  Slider {
    id: defStatDvEdit

    anchors.top: atkStatDvEdit.bottom
    anchors.topMargin: -16
    anchors.left: defStatDvTxt.right
    anchors.leftMargin: 10
    anchors.right: defStatDvReadout.left
    anchors.rightMargin: 10

    from: 0
    to: 15
    snapMode: Slider.SnapAlways
    stepSize: 1

    onMoved: boxData.dvSet(1, value);
    Component.onCompleted: value = boxData.dvAt(1);

    Connections {
      target: boxData
      onDvChanged: defStatDvEdit.value = boxData.dvAt(1);
    }
  }

  Text {
    id: defStatDvReadout

    width: font.pixelSize * 2

    anchors.top: atkStatDvReadout.bottom
    anchors.topMargin: 15
    anchors.right: parent.right

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (boxData.hpDV === "phony")
          ? boxData.dvAt(1)
          : boxData.dvAt(1)
  }

  Text {
    id: spdStatDvTxt

    anchors.top: defStatDvTxt.bottom
    anchors.topMargin: 16
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Spd"
  }

  Slider {
    id: spdStatDvEdit

    anchors.top: defStatDvEdit.bottom
    anchors.topMargin: -15
    anchors.left: spdStatDvTxt.right
    anchors.leftMargin: 10
    anchors.right: spdStatDvReadout.left
    anchors.rightMargin: 10

    from: 0
    to: 15
    snapMode: Slider.SnapAlways
    stepSize: 1

    onMoved: boxData.dvSet(2, value);
    Component.onCompleted: value = boxData.dvAt(2);

    Connections {
      target: boxData
      onDvChanged: spdStatDvEdit.value = boxData.dvAt(2);
    }
  }

  Text {
    id: spdStatDvReadout

    width: font.pixelSize * 2

    anchors.top: defStatDvReadout.bottom
    anchors.topMargin: 15
    anchors.right: parent.right

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (boxData.hpDV === "phony")
          ? boxData.dvAt(2)
          : boxData.dvAt(2)
  }

  Text {
    id: spStatDvTxt

    anchors.top: spdStatDvTxt.bottom
    anchors.topMargin: 15
    anchors.left: parent.left

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignRight

    font.pixelSize: 14
    width: font.pixelSize * 3

    text: "Sp"
  }

  Slider {
    id: spStatDvEdit

    anchors.top: spdStatDvEdit.bottom
    anchors.topMargin: -16
    anchors.left: spStatDvTxt.right
    anchors.leftMargin: 10
    anchors.right: spStatDvReadout.left
    anchors.rightMargin: 10

    from: 0
    to: 15
    snapMode: Slider.SnapAlways
    stepSize: 1

    onMoved: boxData.dvSet(3, value);
    Component.onCompleted: value = boxData.dvAt(3);

    Connections {
      target: boxData
      onDvChanged: spStatDvEdit.value = boxData.dvAt(3);
    }
  }

  Text {
    id: spStatDvReadout

    width: font.pixelSize * 2

    anchors.top: spdStatDvReadout.bottom
    anchors.topMargin: 15
    anchors.right: parent.right

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft

    font.pixelSize: 14

    text: (boxData.hpDV === "phony")
          ? boxData.dvAt(3)
          : boxData.dvAt(3)
  }
}
