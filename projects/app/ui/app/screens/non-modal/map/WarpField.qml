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
 * ONE warp field, drawn by its KIND -- the sibling of SpriteField.qml.
 *
 * It is its own file rather than another branch of SpriteField because a door's fields are a
 * different set of things: map pickers, flags, and the **two loaded guns**. It shares the visual
 * language exactly (same label row, same MapWarnIcon, same "the raw box appears only when the combo
 * cannot say the value" rule), and none of the sprite kinds.
 *
 *  | kind          | the control |
 *  |---------------|-------------|
 *  | `coords`      | X and Y **together** — one fact, one control. Packed `x \| (y << 8)`. |
 *  | `map`         | a real map picker: all 248, grouped, glitch ids labelled |
 *  | `flag`        | a switch |
 *  | `flyMap`      | 🔫 the **13** maps a Fly may legally name — full 248 behind a switch |
 *  | `dungeonMap`  | 🔫 the **7** maps a hole may drop you onto |
 *  | `dungeonHole` | 🔫 the legal hole numbers **for that map** (1-based; Victory Road 2F has no hole 1) |
 *  | `byte`        | the last resort |
 *
 * ## The three marks, and they are THREE DIFFERENT FACTS
 *
 * ⚠️ **wiped** — the console zeroes this byte on every save load. `wStatusFlags3` shares an address
 * with `wCableClubDestinationMap`, and `SpecialEnterMap` clears it on the Continue path. Verified on
 * the cartridge: wrote `$FF`, read back `$00`.
 *
 * 💀 **dead** — it survives a save *perfectly*, and **nothing in the game ever reads it**. Two writes,
 * zero reads, across the whole disassembly.
 *
 * 🔫 **gun** — the console's lookup table for this value has no bounds check and, for the fly table,
 * no terminator either. An illegal value makes it read arbitrary ROM as warp data.
 *
 * ⚠️ A wiped byte and an unread byte are **not the same thing**, and collapsing them into one grey
 * "unused" would be exactly the hand-wave this project doesn't do. The panel says which is which.
 *
 * @see MapModel::warpStateFields, notes/reference/warps.md
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
  id: field

  /// One entry of MapModel::warpFields / warpStateFields.
  required property var fieldData

  /// Which door this edits, or -1 for the map's warp STATE (which belongs to no door).
  required property int ind

  spacing: 2

  readonly property string kind: fieldData.kind || "byte"
  readonly property int value: fieldData.value || 0

  /// ⚠️ The console rewrites this on load.
  readonly property bool wiped: fieldData.mark === "wiped"

  /// 💀 It survives fine — and nothing in the game reads it.
  readonly property bool dead: fieldData.mark === "dead"

  /// 🔫 A value the console has no table entry for. `legal` says whether the CURRENT value is one.
  readonly property bool gun: fieldData.gun === true
  readonly property bool legal: fieldData.legal === true

  /// ⚠️ **Is anything going to READ this byte, as things stand?**
  ///
  /// The whole point of the distinction. `PrepareForSpecialWarp` only reaches these lookup tables
  /// when the flags above are set — so an out-of-table value that nothing is going to read is a fact,
  /// not a hazard, and firing a red "!" at it is crying wolf.
  ///
  /// And it *would* cry wolf on every save ever made: `whichDungeonWarp = 0` is the resting value the
  /// game itself writes whenever you are not standing on a hole. @see MapModel::warpStateFields.
  readonly property bool armed: fieldData.armed === true

  /// The red one: this value is out of the table **and** the game is going to use it.
  readonly property bool dangerous: field.gun && !field.legal && field.armed

  /// The escape hatch on a gun: show all 248 maps, not just the safe ones. Off by default -- the
  /// safe list is what you want 99 times in 100, and the full one is a click away for the 1.
  ///
  /// ⚠️ It is NEVER a refusal. Every byte the save can hold stays typeable, and a save that already
  /// holds an illegal value comes up SHOWING it. (map-screen.md §9.3.)
  property bool showAll: false

  // An illegal value is already in the save? Then open the full list at once -- otherwise the combo
  // would sit there showing nothing, and the user would have to guess why.
  Component.onCompleted: if (field.gun && !field.legal) field.showAll = true;

  function commit(v) {
    if (field.ind < 0)
      brg.map.setWarpStateField(field.fieldData.key, v);
    else
      brg.map.setWarpField(field.ind, field.fieldData.key, v);
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

    // ⚠️ WIPED. Console-verified, and the tooltip says so — because "trust me" is not an explanation.
    MapWarnIcon {
      visible: field.wiped
      text: qsTr("The game ZEROES this every single time it loads a save — this byte is shared with a "
                 + "cable-club field that gets cleared on the way in.\n\nSet it, save, load: it's "
                 + "gone. (Checked on a real cartridge: wrote 255, read back 0.)")
    }

    // 💀 DEAD. A different fact, and it gets a different mark — a grey skull, not an amber warning,
    // because nothing is going to go wrong. It just won't do anything.
    Label {
      visible: field.dead
      text: "💀"
      font.pixelSize: 11
      opacity: 0.75

      HoverHandler { id: deadHover }

      MapToolTip {
        shown: deadHover.hovered
        followGlobalSetting: false
        text: qsTr("The game writes this every time you use a warp — and then never reads it again.\n\n"
                   + "Nothing anywhere in the game looks at it. It survives being saved perfectly; it "
                   + "simply doesn't do anything. Change it freely, and change nothing.")
      }
    }

    // 🔫 THE RED ONE — and ONLY when the value is out of the table **and the game will read it**.
    //
    // ⚠️ Not "whenever it's out of the table". `whichDungeonWarp = 0` is the resting value the game
    // itself writes whenever you are not standing on a hole, so *every save ever made* carries one —
    // and flagging that would be crying wolf on every file anybody ever opened. It is precisely the
    // mistake the sprite "your cast has changed" notice made first time round.
    MapWarnIcon {
      visible: field.dangerous
      warn: true
      text: qsTr("No console has a table entry for this value — and this save is set up to USE it.\n\n"
                 + "The game looks it up in a list with no end marker and no bounds check, so it will "
                 + "run off the end, read whatever cartridge bytes come next as if they were warp "
                 + "data, and drop the player somewhere undefined.\n\nIt's still yours to set. It is "
                 + "not refused, and it will not be rewritten behind your back.")
    }
  }

  // The quiet version: the value IS out of the table, but nothing is going to read it. That is a
  // true and mildly interesting fact, and it gets a grey line — not an alarm.
  Label {
    Layout.fillWidth: true
    visible: field.gun && !field.legal && !field.armed
    text: qsTr("Not a value the console has an answer for — but nothing on this save is going to "
               + "read it, so it sits there harmlessly.")
    wrapMode: Text.Wrap
    font.pixelSize: 10
    opacity: 0.5
  }

  // ── coords: X and Y are ONE fact, so they are ONE control ─────────────────────────────────
  RowLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "coords"
    spacing: 6

    // Packed: low byte X, high byte Y. @see MapModel::setWarpField.
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

  // ── flag: a switch ─────────────────────────────────────────────────────────────────────────
  Switch {
    Layout.bottomMargin: 2
    visible: field.kind === "flag"

    checked: field.value !== 0
    onToggled: field.commit(checked ? 1 : 0)

    // The label is already above it; a switch that repeats its own name is chrome.
    text: checked ? qsTr("On") : qsTr("Off")
    font.pixelSize: 11
  }

  // ── map: a real map picker — all 248, grouped, glitch ids labelled ────────────────────────
  //
  // Plus the one value that is not a map at all: `back outside` ($FF). It goes at the TOP, because on
  // a door it is not an exotic option — it is the ordinary case.
  ComboBox {
    id: mapCombo
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    Layout.preferredHeight: 30
    visible: field.kind === "map"
    font.pixelSize: 11

    readonly property var maps: {
      const base = brg.map.mapList();
      if (field.fieldData.key !== "destMap")
        return base;

      // "Back outside" is the sane default and the commonest destination in the game. It leads the
      // list, and it says what it MEANS — resolved through "Outside is…", live.
      const out = [{ ind: 255,
                     name: qsTr("← Back outside (%1)").arg(brg.map.lastMapName),
                     group: qsTr("The usual"), isCopy: false, copyOf: "" }];
      return out.concat(base);
    }

    model: mapCombo.maps
    textRole: "name"
    valueRole: "ind"

    currentIndex: {
      const list = mapCombo.maps;
      for (let i = 0; i < list.length; i++)
        if (list[i].ind === field.value)
          return i;
      return -1;
    }

    onActivated: field.commit(currentValue)

    delegate: ItemDelegate {
      required property var modelData
      required property int index

      width: mapCombo.width
      height: (modelData.group !== "" ? 20 : 0) + 24
      highlighted: mapCombo.highlightedIndex === index

      contentItem: ColumnLayout {
        spacing: 0

        Text {
          visible: modelData.group !== ""
          Layout.fillWidth: true
          text: modelData.group
          font.pixelSize: 10
          font.bold: true
          color: brg.settings.textColorMid
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 6

          Text {
            text: modelData.ind
            font.pixelSize: 10
            font.family: "monospace"
            color: brg.settings.textColorMid
            Layout.minimumWidth: 22
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
  }

  // ── 🔫 flyMap / dungeonMap / dungeonHole: the LEGAL values, with the full range one click away ──
  ColumnLayout {
    Layout.fillWidth: true
    Layout.bottomMargin: 6
    visible: field.kind === "flyMap" || field.kind === "dungeonMap" || field.kind === "dungeonHole"
    spacing: 3

    readonly property var safe: {
      if (field.kind === "flyMap")
        return brg.map.flyWarpMapList();

      if (field.kind === "dungeonMap")
        return brg.map.dungeonWarpMapList();

      // The holes legal for the map that is currently picked. It is a per-MAP question, not a range:
      // Victory Road 2F has a hole 2 and no hole 1.
      return brg.map.dungeonHoleList(brg.map.warpDungeonMap);
    }

    // The safe list. This is the control you want, and it is the one you get.
    ComboBox {
      id: safeCombo
      Layout.fillWidth: true
      Layout.preferredHeight: 30
      visible: !field.showAll
      font.pixelSize: 11

      model: parent.safe
      textRole: "name"
      valueRole: "value"

      currentIndex: {
        const list = model;
        for (let i = 0; i < list.length; i++)
          if (list[i].value === field.value)
            return i;
        return -1;
      }

      onActivated: field.commit(currentValue)

      // A hole list that is empty means the picked map has no holes -- which is a real thing to be
      // told, not an empty combo to stare at.
      displayText: (model && model.length > 0)
                   ? (currentIndex >= 0 ? currentText : qsTr("— not one of the legal values —"))
                   : qsTr("— this map has no holes —")
    }

    // The full byte range. Never hidden, never refused -- one click away, and it says what it is.
    SpinBox {
      Layout.fillWidth: true
      Layout.preferredHeight: 28
      visible: field.showAll
      font.pixelSize: 11
      editable: true
      from: 0
      to: 255
      value: field.value
      onValueModified: field.commit(value)
    }

    // ⚠️ Not a checkbox saying "unsafe mode". A plain, quiet link — because the full range is not
    // forbidden, it is just not what you usually want. (The doctrine: hack values are first-class.)
    Label {
      Layout.fillWidth: true
      text: field.showAll
            ? qsTr("Showing every value. ⟲ Back to the safe ones")
            : (field.kind === "dungeonHole"
               ? qsTr("Showing the legal holes. Show every value…")
               : qsTr("Showing the maps the console has an answer for. Show all 248…"))
      font.pixelSize: 10
      color: brg.settings.accentColor
      wrapMode: Text.Wrap

      HoverHandler { id: allHover; cursorShape: Qt.PointingHandCursor }
      TapHandler { onTapped: field.showAll = !field.showAll }
    }
  }

  // ── byte: the last resort ─────────────────────────────────────────────────────────────────
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
