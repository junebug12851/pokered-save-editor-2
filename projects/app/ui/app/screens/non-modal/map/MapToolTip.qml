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
  MapToolTip.qml -- the map screen's tooltip. Small, dark, close to the thing it explains.

  Three things were wrong with what this replaces (Twilight, 2026-07-13):

    * IT DIDN'T SHOW. A Material ToolTip parks itself away from its parent and the app's shared
      MainToolTip is gated on the header's "?" toggle (`brg.settings.infoBtnPressed`) -- so half the
      map screen's tooltips only appeared if you had turned global tooltips on first. A tooltip that
      needs a preference set is a tooltip nobody reads.
    * IT SAT TOO FAR AWAY, so it read as a floating label rather than as an explanation of the button
      under your cursor.
    * IT LOOKED BAD -- default Material chrome, thin text, poor contrast.

  So: hover shows it, always. It sits 6px under the control, centred on it. Dark, rounded, 11px, and
  wide enough for a real sentence.
*/
import QtQuick
import QtQuick.Controls

ToolTip {
  id: tip

  /// How wide the text may run before it wraps. A sentence, not a novel.
  property int maxWidth: 240

  delay: 350
  timeout: -1

  // Right under the control, centred on it -- so it explains THIS button and not the general area.
  x: (parent ? (parent.width - width) / 2 : 0)
  y: (parent ? parent.height + 6 : 0)

  padding: 7

  contentItem: Text {
    // `tip.text`, NOT `parent.text` -- a contentItem's parent is not the ToolTip, and QML only tells
    // you at RUNTIME, as "Unable to assign [undefined] to QString". (qt-patterns.md)
    text: tip.text
    font.pixelSize: 11
    color: "#ffffff"
    wrapMode: Text.WordWrap
    width: Math.min(implicitWidth, tip.maxWidth)
    lineHeight: 1.15
  }

  background: Rectangle {
    color: "#f2212121"
    radius: 5
    border.width: 1
    border.color: "#33ffffff"
  }

  enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 90 } }
  exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90 } }
}
