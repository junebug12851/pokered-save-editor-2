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

  /// The user has asked for the raw box on a value the combo COULD have said. Sticky, so it does not
  /// vanish under them mid-edit.
  property bool rawOpen: false

  /// The raw box is up when the value needs it, or when it was asked for.
  readonly property bool rawShown: field.kind === "enum" && (!field.known || field.rawOpen)

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
  // Full width. No number box beside it stealing 74px and cutting its text off.
  ComboBox {
    id: combo
    visible: field.kind === "enum"
    Layout.fillWidth: true
    Layout.bottomMargin: field.rawShown || field.pickedHack ? 0 : 6
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

    // Only offered when the value is one the combo COULD have said -- otherwise there is nothing to
    // fold back into.
    MapRailButton {
      visible: field.known
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

  // ── The hack flag, in words ────────────────────────────────────────────────────────────────
  //
  // Not a refusal, not a correction. A sentence.
  Label {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "enum" && (!field.known || field.pickedHack)

    text: field.known
            ? qsTr("No real game uses this one.")
            : qsTr("No real game holds this value here.")
    font.pixelSize: 10
    color: "#c79100"
    wrapMode: Text.Wrap
  }
}
