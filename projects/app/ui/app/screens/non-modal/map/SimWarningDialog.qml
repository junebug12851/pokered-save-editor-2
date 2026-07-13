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
 * "Letting the people walk MOVES the real sprite data."
 *
 * The one place in this app that admits to being destructive, and it only says it once -- but the
 * "don't show me this again" starts **unticked**, because a warning you have to opt back INTO is
 * not a warning.
 *
 * @see MapSim -- and the note there on why destructive was the right call.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
  id: dialog

  anchors.centerIn: Overlay.overlay
  modal: true
  width: 380

  title: qsTr("The people will actually move")

  standardButtons: Dialog.Ok | Dialog.Cancel

  onAccepted: {
    if (dontAsk.checked)
      brg.settings.mapSimWarned = true;
  }

  ColumnLayout {
    width: parent.width
    spacing: 12

    Label {
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      text: qsTr("This doesn't preview anything — it edits your save. Every step a character takes writes their new position into the sprite data, exactly as if you had dragged them there yourself.")
    }

    Label {
      Layout.fillWidth: true
      wrapMode: Text.Wrap
      font.pixelSize: 12
      opacity: 0.75
      text: qsTr("There's no undo for it, so if you want the cast where it is, save first. (The game rebuilds a map's original cast from the cartridge anyway, the moment the player walks back in.)")
    }

    CheckBox {
      id: dontAsk
      text: qsTr("Don't show me this again")
      checked: false      // ⚠️ UNTICKED. Never pre-tick a way out of a warning.
    }
  }
}
