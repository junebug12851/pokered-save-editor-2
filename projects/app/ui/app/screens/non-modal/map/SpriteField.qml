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
 * ONE field of a sprite, in the Details panel. The field says what KIND it is; this file draws the
 * control that kind deserves.
 *
 * ⚠️ **Rewritten 2026-07-13.** The old version drew every byte the same way — a number box, with the
 * combo (if any) squeezed in beside it and a paragraph underneath. Twilight:
 *
 *   > *"the fields are all just raw values, exactly what I said not to do... I don't know what most
 *   > of those numbers mean on sprite details, it's cryptic as crap... **Don't show boxes if it's
 *   > unnecessary** — some of these raw values have a combo box next to them and I bet that combo
 *   > box value is going to determine if the textbox raw value is even needed to be there or not."*
 *
 * So the rules now are:
 *
 *  1. **The raw byte box only appears when the combo cannot say it.** If the value is one the game
 *     names, the combo IS the control and there is nothing to type — so there is no box. The box
 *     appears the instant the value is one no option covers, which is exactly when you need it.
 *     Nothing is ever refused and nothing is ever silently corrected: the full byte range is always
 *     one click away, behind "Something else…".
 *  2. **The label is above the control**, always. A 190px panel cannot seat a label beside a combo —
 *     the combo ends up 90px wide with its text cut off, which is what happened.
 *  3. **A duration is drawn as a duration**, not as a number with a sentence explaining the number.
 *  4. **Scratch bytes wear a yellow "!"** — the console recomputes them when it loads the save.
 *
 * @see MapModel::npcFields -- the schema, and which kind each byte gets.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  id: field

  required property var fieldData
  required property int slot

  /// The map canvas -- handed down so the picture picker can stop the ground taking clicks while it
  /// is open. @see MapCanvas.popupsOpen
  property var canvas: null

  spacing: 2

  readonly property string kind: fieldData.kind || "byte"
  readonly property var options: fieldData.options || []
  readonly property int value: fieldData.value || 0

  /// Is the current value one the game has a name for? (An `enum` whose value nothing names is a
  /// HACK value -- shown, flagged, editable, never refused.)
  readonly property bool known: {
    if (field.kind !== "enum")
      return true;

    for (let i = 0; i < field.options.length; i++)
      if (field.options[i].value === field.value)
        return true;

    return false;
  }

  /// Does the picked option itself carry the hack flag? (A glitch item; a trainer class the game
  /// never uses. Real bytes, offered, and said out loud.)
  readonly property bool pickedHack: {
    for (let i = 0; i < field.options.length; i++)
      if (field.options[i].value === field.value)
        return field.options[i].hack === true;
    return false;
  }

  /// The user asked for the raw box.
  ///
  /// ⚠️ **STICKY. It stays open until they close it**, even if they happen to type a value the combo
  /// has a name for. Twilight: *"it's bad UX for [the raw box] to close just because a legitimate
  /// value was entered."* -- and she is right: typing a number and having the box you were typing in
  /// disappear from under you is the control fighting you.
  property bool rawOpen: false

  /// The raw box is up when it has been opened -- by the user, or by a value that needs it.
  readonly property bool rawShown: field.kind === "enum" && field.rawOpen

  // It OPENS itself for a value the combo cannot say (you need it, so it is there). But it never
  // CLOSES itself: dismissing it is always the user's move. That is the difference between a control
  // that helps and a control that fights you.
  Component.onCompleted: if (!field.known) field.rawOpen = true;
  onKnownChanged: if (!field.known) field.rawOpen = true;

  function commit(v) { brg.map.setNpcField(field.slot, field.fieldData.key, v); }

  // ── The label ──────────────────────────────────────────────────────────────────────────────
  RowLayout {
    Layout.fillWidth: true
    Layout.topMargin: 2
    spacing: 4

    Label {
      Layout.fillWidth: true
      text: field.fieldData.label
      font.pixelSize: 11
      color: brg.settings.textColorMid
      wrapMode: Text.Wrap

      HoverHandler { id: labelHover }

      // ⚠️ Gated on the header's tooltip toggle, like every other tooltip on this screen. The two
      // exceptions are icons you point at deliberately: MapInfoIcon and MapWarnIcon.
      MapToolTip {
        shown: labelHover.hovered && (field.fieldData.blurb || "") !== ""
        text: field.fieldData.blurb || ""
      }
    }

    // The game works this byte out again the next time it loads the save.
    MapWarnIcon {
      visible: field.fieldData.scratch === true
      text: qsTr("The game reloads this when it loads your save — it works it out from the map and "
                 + "the player. You can set it, and it won't survive Continue.")
    }
  }

  // ── picture: a grid of the actual artwork ──────────────────────────────────────────────────
  PicturePick {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "picture"

    canvas: field.canvas
    picture: field.value
    onPicked: (p) => field.commit(p)
  }

  // ── coords / pixels: X and Y are ONE fact, so they are ONE control ─────────────────────────
  RowLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "coords" || field.kind === "pixels"
    spacing: 6

    // Packed: low byte X, high byte Y. @see MapModel::setNpcField.
    readonly property int px: field.value & 0xFF
    readonly property int py: (field.value >> 8) & 0xFF

    Label {
      text: field.kind === "coords" ? qsTr("X") : qsTr("←→")
      font.pixelSize: 10
      opacity: 0.6
    }

    SpinBox {
      id: xBox
      Layout.fillWidth: true
      Layout.minimumWidth: 0
      Layout.preferredHeight: 28
      font.pixelSize: 11
      editable: true

      from: 0
      to: 255
      value: parent.px

      onValueModified: field.commit((value & 0xFF) | (parent.py << 8))
    }

    Label {
      text: field.kind === "coords" ? qsTr("Y") : qsTr("↑↓")
      font.pixelSize: 10
      opacity: 0.6
    }

    SpinBox {
      id: yBox
      Layout.fillWidth: true
      Layout.minimumWidth: 0
      Layout.preferredHeight: 28
      font.pixelSize: 11
      editable: true

      from: 0
      to: 255
      value: parent.py

      onValueModified: field.commit((parent.px & 0xFF) | ((value & 0xFF) << 8))
    }
  }

  // ── enum: the combo IS the control ─────────────────────────────────────────────────────────
  //
  // Full width, and the popup is WIDER THAN THE COMBO where it has to be -- a 190px dock cannot show
  // "Jr. Trainer♀ — never used in-game" in 190px, and shrinking the text to fit is shrinking the
  // content to make room for the furniture. (Twilight, twice: "combo box text still cut off.")
  //
  // The options come SECTIONED: the ordinary values first, the flagged ones under their own heading.
  // A "!" on row 84 of a flat list of 140 items is a "!" nobody ever sees. (@see MapModel::sectioned)
  ComboBox {
    id: combo
    visible: field.kind === "enum"
    Layout.fillWidth: true
    Layout.bottomMargin: field.rawShown ? 0 : 6
    Layout.preferredHeight: 30
    font.pixelSize: 11

    model: field.options
    textRole: "name"
    valueRole: "value"

    // A value the game never names still has to READ correctly -- the combo says what it is rather
    // than silently snapping to the nearest thing it likes.
    displayText: field.known
                   ? currentText
                   : qsTr("Something else — $%1").arg(field.value.toString(16).toUpperCase())

    Component.onCompleted: currentIndex = indexOfValue(field.value)
    onActivated: field.commit(currentValue)

    Connections {
      target: field
      function onFieldDataChanged() { combo.currentIndex = combo.indexOfValue(field.value); }
    }

    // ⚠️ DO NOT TOUCH `popup.height`, AND DO NOT SIZE IT FROM `popup.contentItem`.
    //
    // I did exactly that -- `popup.height: Math.min(300, popup.contentItem.implicitHeight + 2)` --
    // and it is a **binding loop**: the popup's height depends on its ListView's implicit height,
    // which depends on the popup's height. Qt breaks the loop by dropping bindings, and what you get
    // is a popup whose rows are **squashed too short to show their text** and **do not respond to
    // clicks at all**. Twilight, and both symptoms in one sentence: *"combo box items render too
    // short and I don't see text sometimes... none of the combo boxes' buttons work."*
    //
    // The popup sizes itself. MapPicker -- the one that works, with 248 grouped maps in it -- does
    // not lay a finger on it, and neither do we. The delegate binds to the COMBO's width, never the
    // popup's.
    delegate: ItemDelegate {
      id: opt
      required property var modelData
      required property int index

      width: combo.width
      height: (opt.heading !== "" ? 20 : 0) + 28
      highlighted: combo.highlightedIndex === opt.index

      /// The section heading rides on the FIRST row of its section, and is empty on the rest.
      readonly property string heading: modelData.header || ""

      contentItem: ColumnLayout {
        spacing: 0

        Text {
          visible: opt.heading !== ""
          Layout.fillWidth: true
          text: opt.heading
          font.pixelSize: 10
          font.bold: true
          color: brg.settings.textColorMid
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 5

          Text {
            Layout.fillWidth: true
            text: opt.modelData.name
            font.pixelSize: 12
            color: brg.settings.textColorDark
            elide: Text.ElideRight
          }

          // The mark, on the row it belongs to -- and now it is in a SECTION of marked rows, so it
          // reads as "these ones", not as a needle in a haystack.
          Text {
            visible: opt.modelData.hack === true
            text: "!"
            font.pixelSize: 11
            font.bold: true
            color: "#c79100"
          }
        }
      }
    }
  }

  // ── The escape hatch ───────────────────────────────────────────────────────────────────────
  //
  // "Every value the save can hold is editable, including the hack ones." The combo covers the
  // values the game names; THIS covers the rest — and it stays out of the way until it is wanted.
  Label {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "enum" && !field.rawShown

    text: qsTr("Something else…")
    font.pixelSize: 10
    font.underline: linkHover.hovered
    color: "#56b4e9"

    HoverHandler { id: linkHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: field.rawOpen = true }
  }

  RowLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.rawShown
    spacing: 4

    SpinBox {
      Layout.fillWidth: true
      Layout.minimumWidth: 0
      Layout.preferredHeight: 28
      font.pixelSize: 11
      editable: true

      from: field.fieldData.min
      to: field.fieldData.max
      value: field.value

      onValueModified: field.commit(value)
    }

    // Closing it is ALWAYS the user's move, never the control's -- so this is always here. (Even on
    // a value the combo has no name for: fold it away and the combo still says what the value is.)
    MapRailButton {
      size: 22
      glyph: "⌃"
      tip: qsTr("Put the raw box away")
      onClicked: field.rawOpen = false
    }
  }

  // ── frames: a DURATION, drawn as one ───────────────────────────────────────────────────────
  //
  // ⚠️ Twilight: *"What is 'delay until next move'? What does that mean, how is it measured? Don't
  // tell them with text — tell them with a beautiful, polished, clean UI/UX."*
  //
  // So: a bar you can drag, with the answer written on it in both of the units that matter — the
  // frames the console counts, and the seconds you would feel. The Game Boy runs at 59.7275 Hz, and
  // that is where the seconds come from; it is not a round 60 and we do not pretend it is.
  ColumnLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "frames"
    spacing: 2

    RowLayout {
      Layout.fillWidth: true
      spacing: 6

      Label {
        text: field.value === 0
                ? qsTr("no wait")
                : qsTr("%1 frames").arg(field.value)
        font.pixelSize: 12
        font.bold: true
        color: brg.settings.textColorDark
      }

      Label {
        visible: field.value > 0
        text: qsTr("≈ %1 s").arg((field.value / 59.7275).toFixed(2))
        font.pixelSize: 11
        opacity: 0.55
      }

      Item { Layout.fillWidth: true }
    }

    Slider {
      Layout.fillWidth: true
      Layout.preferredHeight: 22
      from: 0
      to: 255
      stepSize: 1
      value: field.value

      onMoved: field.commit(Math.round(value))
    }
  }

  // ── team: which of a trainer class's rosters ───────────────────────────────────────────────
  SpinBox {
    visible: field.kind === "team"
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    Layout.preferredHeight: 28
    font.pixelSize: 11
    editable: true

    from: 0
    to: 255
    value: field.value

    onValueModified: field.commit(value)
  }

  // ── byte: the last resort ──────────────────────────────────────────────────────────────────
  SpinBox {
    visible: field.kind === "byte"
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    Layout.preferredHeight: 28
    font.pixelSize: 11
    editable: true

    from: field.fieldData.min
    to: field.fieldData.max
    value: field.value

    onValueModified: field.commit(value)
  }

  // (A "No real game holds this value here." line lived down here, under every flagged field.
  //  REMOVED 2026-07-13 -- Twilight: *"Don't have a 'no real game holds this value'; this is implied
  //  when using Something else."* And it is: the combo already reads "Something else — $37", and the
  //  flagged options already sit under their own heading with a "!" on them. Saying it a third time
  //  in a full sentence, under every one of them, is the panel talking to itself.)
}
