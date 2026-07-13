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
 * ONE editable byte of a sprite, in the Details panel.
 *
 * Two shapes, from the field's `kind`:
 *
 *   * **enum** -- a combo of the values the game actually names, and a **number box beside it**.
 *     The combo is a convenience, never a cage: the byte takes its **full range**, and a value the
 *     game has no name for is *shown* ("Hack value $37"), never refused, never rewritten. That is
 *     the whole doctrine — every value the save can hold is editable, including the ones no real
 *     game would.
 *   * **byte** -- a number box.
 *
 * The blurb under each row says what the byte *does*. Nobody should have to already know what an
 * "image base offset" is to be allowed to look at it.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  id: field

  required property var fieldData
  required property int slot

  spacing: 1

  readonly property bool isEnum: fieldData.kind === "enum"
  readonly property var options: fieldData.options || []

  /// Is the current value one the game has a name for?
  readonly property bool known: {
    if (!field.isEnum)
      return true;
    for (let i = 0; i < field.options.length; i++)
      if (field.options[i].value === field.fieldData.value)
        return true;
    return false;
  }

  // ⚠️ The label goes ABOVE the controls, not beside them.
  //
  // The first version put label | combo | number all on one row, and the number box fell off the
  // right edge of the dock -- a panel is ~190px and that row wants 250. Shrinking the controls to
  // fit would have been shrinking the CONTENT to make room for the furniture, which is the wrong
  // way round. So the row became two.
  Label {
    Layout.fillWidth: true
    text: field.fieldData.label
    font.pixelSize: 11
    wrapMode: Text.Wrap

    HoverHandler { id: labelHover }
    ToolTip.visible: brg.settings.infoBtnPressed && (labelHover.hovered && field.fieldData.blurb !== "")
    ToolTip.delay: 400
    ToolTip.text: field.fieldData.blurb
  }

  RowLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 4
    spacing: 4

    ComboBox {
      id: combo
      visible: field.isEnum
      Layout.fillWidth: true
      Layout.minimumWidth: 0
      Layout.preferredHeight: 30
      font.pixelSize: 11

      model: field.options
      textRole: "name"
      valueRole: "value"

      // A value the game never names still has to READ correctly -- so when it is unknown the
      // combo shows what it is rather than silently snapping to the nearest thing it likes.
      displayText: field.known
                     ? currentText
                     : qsTr("Hack value $%1").arg(field.fieldData.value.toString(16).toUpperCase())

      Component.onCompleted: currentIndex = indexOfValue(field.fieldData.value)

      onActivated: brg.map.setNpcField(field.slot, field.fieldData.key, currentValue)

      Connections {
        target: field
        function onFieldDataChanged() {
          combo.currentIndex = combo.indexOfValue(field.fieldData.value);
        }
      }
    }

    SpinBox {
      id: spin
      Layout.fillWidth: !field.isEnum
      Layout.preferredWidth: field.isEnum ? 74 : 0
      Layout.preferredHeight: 30
      font.pixelSize: 11

      from: field.fieldData.min
      to: field.fieldData.max
      value: field.fieldData.value
      editable: true

      // Commit on the value settling, not on every keystroke -- typing "12" into a byte box should
      // not briefly write 1.
      onValueModified: brg.map.setNpcField(field.slot, field.fieldData.key, value)
    }
  }

  // The hack flag, in words. Not a refusal, not a correction -- a sentence.
  Label {
    Layout.fillWidth: true
    Layout.bottomMargin: 4
    visible: field.isEnum && !field.known
    text: qsTr("No real game holds this value here.")
    font.pixelSize: 10
    color: "#c79100"
    wrapMode: Text.Wrap
  }
}
