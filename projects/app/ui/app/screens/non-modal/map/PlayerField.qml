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
 * ONE player-state field, drawn by its KIND -- the sibling of WarpField.qml and SpriteField.qml.
 *
 * The player's bytes are a different set again: facing directions, a getting-around mode, and a pile
 * of flags. It shares the visual language exactly (same label row, same MapWarnIcon, same "the raw
 * box appears only when the combo cannot say the value" rule).
 *
 *  | kind   | the control |
 *  |--------|-------------|
 *  | `enum` | a combo over `options` ({value, name}); a raw box steps in for a value none of them names |
 *  | `flag` | a switch |
 *  | `byte` | the last resort — a spin box across `min`..`max` |
 *
 * ## The two marks — and they are DIFFERENT FACTS (@see notes/reference/player-state.md)
 *
 * ⚠️ **reload** — the console rewrites this the instant it loads your save. Unlike the warp block
 * (where one wipe of `wStatusFlags3` does the lot), the player's bytes are rewritten by DIFFERENT
 * routines — his facing is *forced to DOWN*, the battle bits are *zeroed*, STRENGTH is *reset*, the
 * door bits are *cleared* — so each field carries its OWN `note` saying which, verified on the
 * cartridge. The amber "!" shows that note, not a shared one.
 *
 * 💀 **dead** — it survives a save perfectly, and nothing in the game ever reads it.
 *
 * Neither is hidden because it is unimportant; both are hidden (behind the toolbar's "Reloaded
 * values" switch, filtered in the MODEL) because they are clutter above the fields that do something.
 * On, they appear, each wearing its mark. Nothing is ever refused, and nothing is rewritten behind
 * your back.
 *
 * @see MapModel::playerFields
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  id: field

  /// One entry of MapModel::playerFields.
  required property var fieldData

  spacing: 2

  readonly property string kind: fieldData.kind || "byte"
  readonly property int value: fieldData.value || 0
  readonly property var options: fieldData.options || []

  /// ⚠️ The console rewrites this on load. `note` says exactly how.
  readonly property bool reload: fieldData.mark === "reload"

  /// 💀 It survives fine — and nothing in the game reads it.
  readonly property bool dead: fieldData.mark === "dead"

  /// The per-field explanation the amber "!" / the skull carries. Never a shared "trust me".
  readonly property string note: fieldData.note || ""

  /// Does the current value match one of the enum options? When it doesn't, the combo steps aside
  /// for a raw box — the same doctrine as the warp guns: a hack value is first-class, never refused.
  readonly property bool valueIsNamed: {
    for (let i = 0; i < field.options.length; i++)
      if (field.options[i].value === field.value)
        return true;
    return false;
  }

  /// The escape hatch: type any byte, not just the named ones. Auto-open when the save already holds
  /// an unnamed value, so the combo never sits there showing nothing.
  property bool showRaw: false
  Component.onCompleted: if (field.kind === "enum" && !field.valueIsNamed) field.showRaw = true;

  function commit(v) {
    brg.map.setPlayerField(field.fieldData.key, v);
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

    // ⚠️ REWRITTEN ON LOAD. The tooltip is the field's OWN note — because these bytes are rewritten
    // for different reasons, and "trust me" is not an explanation.
    MapWarnIcon {
      visible: field.reload
      text: field.note
    }

    // 💀 DEAD. A different fact, a different mark — a grey skull, not an amber warning, because
    // nothing goes wrong. It just won't do anything.
    Label {
      visible: field.dead
      text: "💀"
      font.pixelSize: 11
      opacity: 0.75

      HoverHandler { id: deadHover }

      MapToolTip {
        shown: deadHover.hovered
        followGlobalSetting: false
        text: field.note
      }
    }
  }

  // ── enum: a combo over the named values ─────────────────────────────────────────────────────
  ComboBox {
    id: enumCombo
    Layout.fillWidth: true
    Layout.bottomMargin: field.showRaw ? 0 : 6
    Layout.preferredHeight: 30
    visible: field.kind === "enum" && !field.showRaw
    font.pixelSize: 11

    model: field.options
    textRole: "name"
    valueRole: "value"

    currentIndex: {
      for (let i = 0; i < field.options.length; i++)
        if (field.options[i].value === field.value)
          return i;
      return -1;
    }

    onActivated: field.commit(currentValue)

    displayText: currentIndex >= 0 ? currentText : qsTr("— %1 —").arg(field.value)
  }

  // The raw byte behind an enum — never hidden, never refused, one quiet link away.
  SpinBox {
    Layout.fillWidth: true
    Layout.preferredHeight: 28
    visible: field.kind === "enum" && field.showRaw
    font.pixelSize: 11
    editable: true
    from: 0
    to: 255
    value: field.value
    onValueModified: field.commit(value)
  }

  Label {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "enum"
    text: field.showRaw ? qsTr("Showing the raw value. ⟲ Back to the named ones")
                        : qsTr("Set a raw value…")
    font.pixelSize: 10
    color: brg.settings.accentColor
    wrapMode: Text.Wrap

    HoverHandler { cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: field.showRaw = !field.showRaw }
  }

  // ── flag: a switch ───────────────────────────────────────────────────────────────────────────
  Switch {
    Layout.bottomMargin: 2
    visible: field.kind === "flag"

    checked: field.value !== 0
    onToggled: field.commit(checked ? 1 : 0)

    // The name is already above it; a switch that repeats its own name is chrome.
    text: checked ? qsTr("On") : qsTr("Off")
    font.pixelSize: 11
  }

  // ── byte: the last resort ────────────────────────────────────────────────────────────────────
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
