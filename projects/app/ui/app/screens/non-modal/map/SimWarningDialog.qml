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
 * ⚠️ It has to **look like an alert**. The first version was a plain title and two paragraphs and it
 * read like a blog post (Twilight, 2026-07-13) -- which is exactly how a destructive action gets
 * clicked through without being read. So: a warning stripe, a big ⚠, a short bold line that says the
 * one thing that matters, and **dark, high-contrast text** rather than the polite grey the rest of
 * the app uses.
 *
 * The "don't show me this again" starts **unticked**, because a warning you have to opt back INTO is
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
  width: 400
  padding: 0

  // No stock title bar -- the header below IS the title, and it carries the warning colour.
  title: ""

  background: Rectangle {
    color: "#ffffff"
    radius: 8
    border.width: 1
    border.color: "#e0e0e0"
  }

  // ⚠️ NOT in `onAccepted`. A handler declared where a component is USED overrides the one declared
  // inside it -- so MapIdentityBar's `onAccepted: brg.mapSim.playing = true` would have silently
  // replaced this, and "don't show me this again" would never have been remembered. The button does
  // it instead, where nothing can shadow it.

  contentItem: ColumnLayout {
    spacing: 0

    // ── The stripe. You are meant to notice this. ─────────────────────────────────────────
    Rectangle {
      Layout.fillWidth: true
      implicitHeight: header.implicitHeight + 24
      color: "#fff3e0"          // warm, not alarming red -- this is destructive, not dangerous
      radius: 8

      // Square off the bottom corners so it meets the body cleanly.
      Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 10
        color: parent.color
      }

      Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 2
        color: "#ef6c00"
      }

      RowLayout {
        id: header
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
          text: "⚠"
          font.pixelSize: 30
          color: "#ef6c00"
          Layout.alignment: Qt.AlignTop
        }

        Label {
          Layout.fillWidth: true
          text: qsTr("This will change your save")
          font.pixelSize: 16
          font.bold: true
          color: "#5a3200"
          wrapMode: Text.Wrap
        }
      }
    }

    // ── The body. Dark text. This is the bit that must actually be read. ──────────────────
    ColumnLayout {
      Layout.fillWidth: true
      Layout.margins: 16
      spacing: 10

      Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        font.pixelSize: 13
        color: "#212121"
        text: qsTr("Letting the people walk doesn't preview anything — it edits the save. Every step a character takes writes their new position into the sprite data, exactly as if you had dragged them there yourself.")
      }

      Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        font.pixelSize: 13
        color: "#212121"
        font.bold: true
        text: qsTr("There is no undo. If you want the cast where it is, save first.")
      }

      CheckBox {
        id: dontAsk
        text: qsTr("Don't show me this again")
        checked: false      // ⚠️ UNTICKED. Never pre-tick a way out of a warning.

        contentItem: Label {
          text: dontAsk.text
          font.pixelSize: 12
          color: "#424242"
          leftPadding: dontAsk.indicator.width + 6
          verticalAlignment: Text.AlignVCenter
        }
      }
    }

    Rectangle {
      Layout.fillWidth: true
      implicitHeight: 1
      color: "#e0e0e0"
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.margins: 12
      spacing: 8

      Item { Layout.fillWidth: true }

      Button {
        text: qsTr("Cancel")
        flat: true
        onClicked: dialog.reject()
      }

      Button {
        text: qsTr("Let them walk")
        highlighted: true
        onClicked: {
          if (dontAsk.checked)
            brg.settings.mapSimWarned = true;

          dialog.accept();
        }
      }
    }
  }
}
