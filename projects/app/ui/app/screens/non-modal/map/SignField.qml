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

/**
 * ONE sign field, drawn by its KIND -- the sibling of WarpField.qml / SpriteField.qml.
 *
 * A sign has only two things to edit, but the second one is the whole point of the feature:
 *
 *  | kind     | the control |
 *  |----------|-------------|
 *  | `coords` | X and Y **together** — one fact, one control. Packed `x \| (y << 8)`. |
 *  | `enum`   | the map's TEXT, grouped (Signs / People / Other), showing the real words. A raw id box |
 *  |          | appears when the value is one no entry names — the hack ids past the map's table. |
 *
 * The grouped combo is the "select from the text on the map" project leadership asked for: every text id a
 * sign can reference, grouped by what it is meant for, each row its real words. Selection commits;
 * nothing happens on hover. @see MapModel::signTextList, notes/reference/signs.md.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  id: field

  /// One entry of MapModel::signFields.
  required property var fieldData

  /// Which sign this edits.
  required property int ind

  spacing: 2

  readonly property string kind: fieldData.kind || "byte"
  readonly property int value: fieldData.value || 0
  readonly property var options: fieldData.options || []

  /// Is the current value one the grouped list names? If not, it is a hack id past the map's text,
  /// and the raw box is what shows it -- never refused, never hidden.
  readonly property bool valueNamed: {
    for (let i = 0; i < field.options.length; i++)
      if (field.options[i].value === field.value)
        return true;
    return false;
  }

  /// Type any id 0..255. On automatically when the value is one no entry names, so the control is
  /// never sitting there showing nothing.
  property bool showRaw: false
  Component.onCompleted: if (field.kind === "enum" && !field.valueNamed) field.showRaw = true;

  function commit(v) {
    brg.map.setSignField(field.ind, field.fieldData.key, v);
  }

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

      MapToolTip {
        shown: labelHover.hovered && (field.fieldData.blurb || "") !== ""
        text: field.fieldData.blurb || ""
      }
    }
  }

  // ── coords: X and Y are ONE fact, so they are ONE control ─────────────────────────────────
  RowLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "coords"
    spacing: 6

    readonly property int px: field.value & 0xFF
    readonly property int py: (field.value >> 8) & 0xFF

    Label { text: qsTr("X"); font.pixelSize: 10; opacity: 0.6 }

    SpinBox {
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

    Label { text: qsTr("Y"); font.pixelSize: 10; opacity: 0.6 }

    SpinBox {
      Layout.fillWidth: true
      Layout.minimumWidth: 0
      Layout.preferredHeight: 28
      font.pixelSize: 11
      editable: true
      from: 0
      to: 255
      value: parent.py
      onValueModified: field.commit(parent.px | ((value & 0xFF) << 8))
    }
  }

  // ── enum: the map's TEXT, grouped, with the real words ────────────────────────────────────
  ColumnLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "enum"
    spacing: 3

    // The grouped picker. Headers ride on the first row of each section (Signs / People / Other),
    // exactly as the map picker draws its groups.
    ComboBox {
      id: textCombo
      Layout.fillWidth: true
      Layout.preferredHeight: 30
      visible: !field.showRaw
      font.pixelSize: 11

      model: field.options
      textRole: "name"
      valueRole: "value"

      currentIndex: {
        const list = field.options;
        for (let i = 0; i < list.length; i++)
          if (list[i].value === field.value)
            return i;
        return -1;
      }

      onActivated: field.commit(currentValue)

      displayText: currentIndex >= 0 ? currentText : qsTr("— id %1 (not this map's text) —").arg(field.value)

      delegate: ItemDelegate {
        required property var modelData
        required property int index

        width: textCombo.width
        height: ((modelData.header || "") !== "" ? 20 : 0) + 26
        highlighted: textCombo.highlightedIndex === index

        contentItem: ColumnLayout {
          spacing: 0

          Text {
            visible: (modelData.header || "") !== ""
            Layout.fillWidth: true
            text: modelData.header || ""
            font.pixelSize: 10
            font.bold: true
            color: brg.settings.textColorMid
          }

          Text {
            Layout.fillWidth: true
            text: modelData.name
            font.pixelSize: 11
            color: brg.settings.textColorDark
            elide: Text.ElideRight
          }
        }
      }
    }

    // The raw id. Never hidden, never refused -- one click away, and it says what it is. On by
    // default when the sign already holds an id no entry names (a hack value past the map's table).
    RowLayout {
      Layout.fillWidth: true
      visible: field.showRaw
      spacing: 6

      Label { text: qsTr("Text id"); font.pixelSize: 10; opacity: 0.6 }

      SpinBox {
        Layout.fillWidth: true
        Layout.preferredHeight: 28
        font.pixelSize: 11
        editable: true
        from: 0
        to: 255
        value: field.value
        onValueModified: field.commit(value)
      }
    }

    // A quiet link, not a checkbox: the raw range is not forbidden, it is just not what you usually
    // want. (The doctrine: hack values are first-class.)
    Label {
      Layout.fillWidth: true
      text: field.showRaw ? qsTr("Typing a raw id. ⟲ Back to this map's text")
                          : qsTr("Point it at any id (past this map's text)…")
      font.pixelSize: 10
      color: brg.settings.accentColor
      wrapMode: Text.Wrap

      HoverHandler { id: rawHover; cursorShape: Qt.PointingHandCursor }
      TapHandler { onTapped: field.showRaw = !field.showRaw }
    }
  }

  // ── byte: the last resort (signs never use it today, but the kit is the kit) ──────────────
  SpinBox {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    Layout.preferredHeight: 28
    visible: field.kind === "byte"
    font.pixelSize: 11
    editable: true
    from: field.fieldData.min !== undefined ? field.fieldData.min : 0
    to: field.fieldData.max !== undefined ? field.fieldData.max : 255
    value: field.value
    onValueModified: field.commit(value)
  }
}
