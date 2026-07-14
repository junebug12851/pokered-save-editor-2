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

/**
 * The yellow **!** — a NOTICE. Not an error, not a refusal: a fact about these bytes that you would
 * otherwise have to already know.
 *
 * It has exactly one job on this screen, and it is Twilight's (2026-07-13): mark the fields the game
 * **overwrites when it loads the save**. The sprite-set cache, the animation scratch, the derived
 * pointers. Those bytes are real, they are yours, and you may edit every one of them — the console
 * simply recomputes them before it ever reads them, and an editor that lets you carefully set a value
 * the game will discard without ever mentioning it is an editor that wasted your afternoon.
 *
 * ⚠️ Like the **?**, it is NOT gated on the tooltip toggle — the same exception, and the same reason
 * the characters panel's "!" was granted one: it is a notice you did not ask for and do need. Its
 * sibling **?** is for information you went looking for. Use the right one:
 *
 *   * **?** — "what IS this?"                          (@see MapInfoIcon)
 *   * **!** — "careful: this is not what it looks like." (here)
 */
import QtQuick
import QtQuick.Controls

Rectangle {
  id: warn

  /// What the notice says. Kept to a sentence.
  property string text: ""

  /// How wide the tooltip is allowed to get before it wraps.
  property int tipWidth: 220

  /// ⚠️ **The red one.** Amber says *"the game will overwrite this"* — a fact, no harm done. This says
  /// **"the console has no answer for this value"**: the warp destinations whose ROM lookup tables
  /// have no bounds check, so an illegal value makes a real Game Boy read arbitrary cartridge bytes
  /// as warp data. That is not a notice, it is a hazard, and red is what red is for.
  ///
  /// ⚠️ Use it sparingly. Twilight, 2026-07-13: *"you have red text everywhere, even to indicate
  /// information, which is bad."* She is right — red means *something is broken*. Here, something is.
  property bool warn: false

  implicitWidth: 14
  implicitHeight: 14
  radius: 7

  color: warn.warn ? (hover.hovered ? "#a33f00" : "#d55e00")
                   : (hover.hovered ? "#c79100" : "#ffd54f")
  border.width: 1
  border.color: warn.warn ? "#7a3600" : "#8a6d00"

  // ⚠️ Fill + align, never `anchors.centerIn` -- a "!" has no descender, so centring the Text's BOX
  // pushes the glyph into the bottom-left of the circle. (It shipped that way once; Twilight saw it.)
  Text {
    anchors.fill: parent
    text: "!"
    font.pixelSize: 10
    font.bold: true
    color: warn.warn ? "#ffffff" : "#3a2e00"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }

  HoverHandler {
    id: hover
    cursorShape: Qt.PointingHandCursor
  }

  ToolTip {
    visible: hover.hovered && warn.text !== ""
    delay: 150

    x: warn.width + 4
    y: -2

    background: Rectangle {
      color: "#212121"
      radius: 4
    }

    contentItem: Label {
      text: warn.text
      color: "#ffffff"
      font.pixelSize: 10
      wrapMode: Text.Wrap
      width: warn.tipWidth
    }
  }
}
