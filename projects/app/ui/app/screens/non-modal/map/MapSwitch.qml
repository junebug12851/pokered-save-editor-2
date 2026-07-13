/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/*
  MapSwitch.qml -- a small sliding on/off switch.

  Hand-rolled rather than a Material `Switch`: Qt 6.5+ gives its controls a large implicit height
  and a wide touch target, so a Material Switch in a 30px popup row is a 40px control fighting its
  layout (ui-patterns.md -> "Material controls fight small heights"). This is 34x18, flat, and in
  the app's own language -- the knob slides, the track fills with the accent.
*/
import QtQuick

Item {
  id: root

  property bool checked: false
  signal toggled()

  implicitWidth: 34
  implicitHeight: 18
  opacity: enabled ? 1.0 : 0.4

  Rectangle {
    id: track

    anchors.fill: parent
    radius: height / 2

    color: root.checked ? brg.settings.accentColor : "#dcdcdc"
    Behavior on color { ColorAnimation { duration: 110 } }

    Rectangle {
      id: knob

      width: parent.height - 4
      height: width
      radius: width / 2
      y: 2
      x: root.checked ? parent.width - width - 2 : 2

      color: "#ffffff"

      Behavior on x { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
    }
  }

  MouseArea {
    anchors.fill: parent
    enabled: root.enabled
    cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    onClicked: root.toggled()
  }
}
