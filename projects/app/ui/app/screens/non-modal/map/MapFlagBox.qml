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
 * ONE flag box -- an outline around something on this map the save keeps a flag for, and the way into
 * Map Storage at that exact flag.
 *
 * The thing that makes this different from every other object on the canvas: **it is not drawn from
 * the save.** MapSprite draws the save's 16 loaded sprite slots; this is drawn from the ROM's cast
 * (`MapDBEntry::sprites`). So when a flag hides an object, there is no sprite to draw -- and the box
 * is still here, on the tile the thing would stand on, DASHED. That is the whole point of the
 * feature: you can see what belongs on a map and what your save has switched off.
 *
 * Not draggable, and deliberately so. A box is not a thing you move -- it is where the cartridge says
 * the object lives. It is a link, and it has exactly one gesture: click it, and Map Storage opens on
 * its switch.
 *
 * See notes/plans/map-screen.md -> Phase 16, and notes/reference/sprites.md for the missable system.
 */
import QtQuick
import QtQuick.Controls

Item {
  id: box

  /// The canvas, for the zoom.
  required property var canvas

  /// The WorldMissables bit index this box is a link to -- what a click hands to Map Storage.
  required property int missable

  /// Map tile coords (TILES -- `maps.json` sizes are in BLOCKS; one block is 2x2 of these).
  required property int tileX
  required property int tileY

  /// The missable's real name + description ("Pokeball 1" / "An item ball on Oaks Lab. Hidden =
  /// already picked up.").
  required property string name
  required property string desc

  /// Is the save currently HIDING it? (WorldMissables: **bit set = hidden**.) Dashes the box.
  required property bool hidden

  /// Does the game's own story ever move this bit? pret's X-marks say no for 121 of them, and that is
  /// worth knowing: it means the switch is yours alone.
  required property bool scriptToggled

  /// pret's known-issue note on this missable, or "".
  required property string oddity

  signal flagClicked(int missable)

  /// How far the box stands OFF the tile, in map pixels. This is not decoration -- it is what makes
  /// the box clickable at all, and it is why she said a box "around" it.
  ///
  /// A box is on the same 16x16 tile as the sprite it marks, and the sprite draws ON TOP (this item
  /// is z: 0). If the box were exactly tile-sized, every click on a visible object would land on the
  /// sprite -- which opens the Details panel -- and the box's own gesture would only ever work for
  /// objects that are hidden: precisely the opposite of the brief, which is about the Poké Balls you
  /// can see. Standing the ring off by 3px leaves a band that is outside the sprite and therefore
  /// clickable, WITHOUT stealing the sprite's own centre. So: the sprite still opens Details, the
  /// ring around it opens Map Storage, and a hidden object (no sprite at all) is clickable right
  /// across.
  readonly property int standoff: 3

  // A box sits ON its tile -- no 4-pixel lift. (The lift is an OAM fact about a drawn sprite; this is
  // a map coordinate, and the thing it points at may not even be drawn.)
  x: (box.canvas.mapBorderPx + box.tileX * 16) * box.canvas.zoom - box.standoff
  y: (box.canvas.mapBorderPx + box.tileY * 16) * box.canvas.zoom - box.standoff
  width: 16 * box.canvas.zoom + box.standoff * 2
  height: 16 * box.canvas.zoom + box.standoff * 2

  // Under the real objects: this annotates them, it must never cover them.
  z: 0

  // ── The outline ──────────────────────────────────────────────────────────────────────────
  //
  // Bluish green (#009e73, Okabe-Ito) -- the Flag boxes layer's own ink, and the same swatch the
  // Layers panel paints, so the row IS the legend. Distinct from the objects it sits on (#cc79a7),
  // the doors (#f0e442) and the signs (#e69f00), which is Fairy Fox's rule: a thing tied to a flag
  // does not look like a thing that isn't.
  Rectangle {
    id: outline
    anchors.fill: parent

    // NO FILL, ever. She asked for "a box around it" and "the outline" -- and the thing a box lands
    // on is a SPRITE, which has artwork worth looking at. A wash (what MapSign uses, because a sign
    // has no art of its own) tinted every Poké Ball in Oak's Lab green; the screenshot review caught
    // it. The box annotates the art, it does not recolour it.
    color: "transparent"
    border.width: Math.max(1, Math.round(box.canvas.zoom))
    border.color: "#009e73"
    radius: 2

    // SOLID = the flag shows it. DASHED = the save is hiding it, and the box is all that is left.
    // Qt has no dashed border on a Rectangle, so the dashes are drawn.
    Canvas {
      anchors.fill: parent
      visible: box.hidden
      onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const w = Math.max(1, Math.round(box.canvas.zoom));
        ctx.lineWidth = w;
        ctx.strokeStyle = "#009e73";
        ctx.setLineDash([Math.max(2, 3 * box.canvas.zoom), Math.max(2, 3 * box.canvas.zoom)]);
        ctx.strokeRect(w / 2, w / 2, width - w, height - w);
      }
      // The dashes must be repainted when the zoom changes them.
      Connections {
        target: box.canvas
        function onZoomChanged() { parent.requestPaint(); }
      }
    }

    // The hidden ones lose their solid edge -- the dashed Canvas above IS their edge.
    states: State {
      when: box.hidden
      PropertyChanges { outline.border.width: 0 }
    }
  }

  // ⚑ -- ONLY on an empty box, and that is the whole rule. When the object is there, its own artwork
  // is the thing worth seeing and a glyph over its face is just something in the way; the outline
  // already says "the game remembers this". When the flag hides it there IS no sprite, the tile is
  // bare, and the mark is what tells you something belongs here. So the glyph appears exactly where
  // it has nothing to obscure. (Vanishes when zoomed out too far to draw it legibly.)
  Text {
    anchors.centerIn: parent
    visible: box.hidden && box.canvas.zoom >= 1.5
    text: "⚑"
    font.pixelSize: Math.max(8, Math.round(9 * box.canvas.zoom))
    color: "#009e73"
  }

  // A MouseArea, never a PointerHandler -- a TapHandler fires THROUGH a floating panel, which cost a
  // whole review round once already. See notes/reference/qt-patterns.md (top of file).
  MouseArea {
    id: hit
    anchors.fill: parent
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    onClicked: box.flagClicked(box.missable)
  }

  ToolTip.visible: hit.containsMouse
  ToolTip.delay: 400
  ToolTip.text: {
    let t = box.name;
    t += box.hidden ? qsTr("\nYour save is hiding this.") : qsTr("\nYour save is showing this.");
    if (box.desc.length > 0)
      t += "\n\n" + box.desc;
    if (!box.scriptToggled)
      t += qsTr("\n\nThe game never changes this one by itself.");
    if (box.oddity.length > 0)
      t += "\n\n⚠ " + box.oddity;
    t += qsTr("\n\nClick to open it in Map Storage.");
    return t;
  }
}
