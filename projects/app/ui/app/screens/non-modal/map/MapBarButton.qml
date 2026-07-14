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
  MapBarButton.qml -- one compact ICON tool-button on the top bar, with a ▾ that says "I drop a menu".

  Twilight, 2026-07-14: *"'Pallet Town' is littered all over the top bar and we're cramped. Have tool
  buttons for Map, Warp, Contrast and Music — icon buttons, and prefer a dynamic reactive icon over a
  plain always-same one (a contrast icon showing the active contrast, not a generic one). Down arrows
  next to the buttons to indicate the dropdown."*

  So this is the shared trigger the four pickers use, and it is deliberately just a frame: the ICON is
  whatever the caller puts inside (the default child slot), which is what lets Contrast hand it a live
  four-shade swatch and Music a note that lights up while it plays. The button owns the chrome — the
  rounded frame, the hover/open wash, the ▾, the tooltip — and nothing else, so every one of the four
  reads as the same kind of thing.

  Usage:
      MapBarButton {
        open: picker.openState            // bind to the picker's own open flag
        tip: qsTr("...")
        onToggle: picker.openState = !picker.openState
        Text { text: "⇄"; font.pixelSize: 14 }   // the icon — any item(s)
      }
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  /// The simple case: a single glyph, centred for you. (Map = ⊞, Warp = ⇄.) For a REACTIVE icon —
  /// Contrast's live swatch, Music's note that lights up — leave this empty and nest the icon as a
  /// child instead (the default `content` slot), anchoring it `verticalCenter: parent.verticalCenter`.
  property string glyph: ""

  /// A colour for the glyph, so a glyph icon can react too (Music's ♪ goes accent while it plays).
  property color glyphColor: brg.settings.textColorDark

  /// The reactive-icon slot. Whatever the caller nests inside — a swatch Row, a note. Sized around.
  default property alias content: iconArea.data

  /// True while this button's menu is open — the caller binds it to the picker's own `openState`, so
  /// the button lights up while its dropdown is showing.
  property bool open: false

  /// The tooltip. White-on-dark MapToolTip, gated the same as everything else on this screen.
  property string tip: ""

  /// The frame's border colour. Defaults to the normal divider; the contrast button overrides it to
  /// yellow on a glitch value, so the icon signals its own state.
  property color accent: brg.settings.dividerColor

  /// Emitted on click. The caller opens/closes its popup here — the button does not own the popup,
  /// because the popup belongs to (and is positioned by) the picker.
  signal toggle()

  implicitWidth: frame.implicitWidth
  implicitHeight: 26

  Rectangle {
    id: frame
    anchors.fill: parent

    implicitWidth: rowL.implicitWidth + 16
    radius: 6

    color: root.open      ? "#e6e6e6"
         : hov.hovered    ? "#f0f0f0"
         : "#ffffff"

    border.width: 1
    border.color: root.accent

    Behavior on color { ColorAnimation { duration: 90 } }

    RowLayout {
      id: rowL
      anchors.centerIn: parent
      spacing: 5

      // The simple glyph, when one is given — centred, no fuss.
      Text {
        visible: root.glyph !== ""
        text: root.glyph
        font.pixelSize: 14
        color: root.glyphColor
        Layout.alignment: Qt.AlignVCenter
      }

      // The reactive-icon slot. A fixed height so every button lines up; width follows its child.
      Item {
        id: iconArea
        visible: root.glyph === ""
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: childrenRect.width
        implicitHeight: 16
      }

      // The ▾ — this is the whole "it drops a menu" affordance Twilight asked for.
      Text {
        text: "⌄"
        font.pixelSize: 10
        color: brg.settings.textColorMid
        opacity: 0.7
        Layout.alignment: Qt.AlignVCenter
      }
    }

    HoverHandler { id: hov; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: root.toggle() }

    MapToolTip {
      shown: hov.hovered && root.tip !== "" && !root.open
      text: root.tip
    }
  }
}
