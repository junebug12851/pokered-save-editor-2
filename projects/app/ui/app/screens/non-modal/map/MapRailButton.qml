/*
  * Copyright 2026 Fairy Fox
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
  MapRailButton.qml -- one square button on a rail (the tool rail, the dock rail, a bar).

  The app's flat language, in a square: nothing at rest, a soft wash on hover, the accent fill when
  it is the active one. Deliberately NOT a Material Button -- Qt 6.5+ gives those a large implicit
  height and width that fights any small control (ui-patterns.md -> "Material controls fight small
  heights"), and a rail of them would be a row of 40px pills.

  A glyph, not an icon file: this app's chrome is flat marks and chips, not a tray of tiny system
  icons. The tooltip is where the words go.
*/
import QtQuick
import QtQuick.Controls

Rectangle {
  id: btn

  property string glyph: ""
  property string tip: ""
  property bool active: false
  property bool enabledBtn: true
  /// A PRIMARY button is marked as special at rest -- the Map Storage (persistent storage /
  /// "World") icon. ⚠️ RESTYLED 2026-07-18 (leadership: *"needs to be a different background
  /// color than the one used to determine which menu or panel is selected/open"*): the old
  /// treatment filled it with the ACCENT at rest, which is exactly the open-panel look -- it
  /// permanently read as "this panel is open". Now: a light accent WASH + accent outline at
  /// rest, and only a genuinely open panel gets the solid accent fill.
  property bool primary: false
  property int size: 32
  /// A one-key shortcut, shown in the tooltip. The keys themselves are bound by the screen.
  property string shortcut: ""

  signal clicked()

  implicitWidth: size
  implicitHeight: size
  radius: 4

  color: !enabledBtn ? "transparent"
       : active      ? brg.settings.accentColor
       : ma.containsPress ? "#22000000"
       : primary     ? Qt.alpha(brg.settings.accentColor, ma.containsMouse ? 0.28 : 0.16)
       : ma.containsMouse ? "#14000000"
       : "transparent"

  // The primary mark: a quiet accent OUTLINE, never the open-panel fill.
  border.width: primary && !active ? 1 : 0
  border.color: Qt.alpha(brg.settings.accentColor, 0.55)

  opacity: enabledBtn ? 1.0 : 0.35

  Behavior on color { ColorAnimation { duration: 90 } }

  Text {
    anchors.centerIn: parent
    text: btn.glyph
    font.pixelSize: Math.round(btn.size * 0.5)
    color: btn.active ? brg.settings.textColorLight : brg.settings.textColorDark
  }

  MouseArea {
    id: ma
    anchors.fill: parent
    hoverEnabled: true
    enabled: btn.enabledBtn
    cursorShape: btn.enabledBtn ? Qt.PointingHandCursor : Qt.ArrowCursor
    onClicked: btn.clicked()
  }

  // Hover shows it. Not "hover, if you first found the ? toggle in the header" -- see MapToolTip.
  MapToolTip {
    shown: ma.containsMouse && btn.tip !== ""
    text: btn.shortcut === "" ? btn.tip : btn.tip + "  (" + btn.shortcut + ")"
  }
}
