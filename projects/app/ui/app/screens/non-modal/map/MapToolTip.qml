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

  ⚠️ **IT ONLY SHOWS WHEN THE HEADER'S "?" IS ON** (`brg.settings.infoBtnPressed`).

  This file used to say the opposite, in as many words, and it was WRONG (Twilight, 2026-07-13):
  tooltips popping up on every hover, everywhere, is exactly the interrupting clutter this app is
  built to avoid. The "?" toggle is the user's switch and it is not ours to override. If you want
  tooltips, turn them on.

  There is **exactly one exception in the whole map screen**, granted explicitly: the **!** on a
  character the map has not loaded (CharacterCell.qml). That is not a hint about a button -- it is a
  notice about something that will go wrong, and it has to be readable the moment you point at it.

  What was genuinely wrong with the stock tooltip, and is fixed here: it sat too far from the control
  (so it read as a floating label rather than an explanation of the thing under the cursor), and its
  contrast was poor. This one sits 6px under the control, centred on it -- dark, rounded, 11px, wide
  enough for a real sentence.
*/
import QtQuick
import QtQuick.Controls

ToolTip {
  id: tip

  /// How wide the text may run before it wraps. A sentence, not a novel.
  property int maxWidth: 240

  /// Set false ONLY for a notice the user must see whether or not they asked for hints. There is one
  /// of those in the whole screen, and it is the character-not-loaded "!".
  property bool followGlobalSetting: true

  /// ⚠️ Callers set THIS, not `visible` -- so the "?" gate cannot be forgotten at a call site.
  property bool shown: false

  visible: tip.shown && (!tip.followGlobalSetting || brg.settings.infoBtnPressed)

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
