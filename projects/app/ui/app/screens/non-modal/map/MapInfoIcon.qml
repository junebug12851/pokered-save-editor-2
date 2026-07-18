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
 * The **?** — "what is this panel, and what is the one thing worth knowing about it?"
 *
 * ⚠️ **The one sanctioned exception to the tooltip-toggle rule**, and the rule for it is Twilight's,
 * verbatim (2026-07-13):
 *
 *   > *"a question mark on title for info non notice stuff that can be moused over for tooltips. I
 *   > guess since its an icon its an exception to the tooltip button rule — just one of these in the
 *   > title. **Don't litter them.** The tooltip icon is important."*
 *
 * ⚠️ AMENDED (leadership, 2026-07-18): *"dont be afraid to use tooltip icons for longer
 * descriptions just dont confuse that with tooltips being allowed to be long"* — so a "?" may now
 * sit on a SECTION header (and, sparingly, on a row that genuinely has a long form), replacing the
 * inline paragraphs that made the storage panel "way too many words". The original spirit stands:
 * don't litter them, and the TOOLTIP itself stays as short as it can be.
 *
 * The original rule: **ONE per panel, in the title bar.** It exists because the alternative is
 * what these panels used to do — three paragraphs of explanation stacked above the controls, which
 * is a wall of text you have to read past every single time to reach the thing you came for. The
 * words are still there. They are just behind a mark you can choose to hover.
 *
 * It is NOT gated on the header's tooltip toggle: you asked for it by pointing at a question mark.
 *
 * @see MapWarnIcon -- its sibling, for a NOTICE (a fact you did not ask for and need anyway).
 */
import QtQuick
import QtQuick.Controls

Rectangle {
  id: info

  /// The words that used to be a paragraph.
  property string text: ""

  /// How wide the tooltip is allowed to get before it wraps.
  property int tipWidth: 240

  implicitWidth: 16
  implicitHeight: 16
  radius: 8

  color: hover.hovered ? "#56b4e9" : "transparent"
  border.width: 1
  border.color: hover.hovered ? "#56b4e9" : brg.settings.textColorMid
  opacity: hover.hovered ? 1.0 : 0.55

  Behavior on color { ColorAnimation { duration: 90 } }

  // ⚠️ Fill + align, NOT `anchors.centerIn`: a Text's box carries the font's ascent and descent, so
  // centring the BOX shoves the glyph down and left. (Exactly the bug that put the characters
  // panel's "!" in the corner of its circle.)
  Text {
    anchors.fill: parent
    text: "?"
    font.pixelSize: 11
    font.bold: true
    color: hover.hovered ? "#ffffff" : brg.settings.textColorMid
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }

  HoverHandler {
    id: hover
    cursorShape: Qt.PointingHandCursor
  }

  ToolTip {
    visible: hover.hovered && info.text !== ""
    delay: 150

    // ON the mark, not across the screen from it.
    x: -width + info.width
    y: info.height + 4

    // OPAQUE, light-on-dark. The stock tooltip is dark text on a translucent panel, which over a
    // pale panel is genuinely hard to read (Twilight, 2026-07-13).
    background: Rectangle {
      color: "#212121"
      radius: 4
    }

    contentItem: Label {
      text: info.text
      color: "#ffffff"
      font.pixelSize: 11
      wrapMode: Text.Wrap
      width: info.tipWidth
    }
  }
}
