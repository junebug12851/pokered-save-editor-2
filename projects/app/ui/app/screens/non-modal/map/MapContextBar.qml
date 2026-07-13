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
  MapContextBar.qml -- the options for whatever you are currently holding.

  Straight out of Aseprite ("the context bar shows specific options for the active tool; it also
  changes depending on the state of the active document") and Photoshop's options bar. It is the
  answer to "where do all these controls go?": they go where they apply, and they are not there
  when they don't.

  Today it holds the tool's options and the map's PALETTE. The palette lives here because it is the
  one control that is always about what you are looking at -- and because a Material SpinBox in the
  footer once shoved the zoom controls clean off the screen (ui-patterns.md: "Material controls
  fight small heights"), which is what taught us to hand-roll these.

  As the phases land, the selection's own fast-lane fields (a warp's X/Y, its destination...) arrive
  here too -- one bar, whatever is relevant, never everything at once.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../../fragments/general"

Rectangle {
  id: bar

  /// The active tool (from the rail).
  property string tool: "select"

  /// The canvas, so the zoom segments can drive it.
  property var canvas: null

  implicitHeight: 32
  color: "#ffffff"

  Rectangle {
    anchors.bottom: parent.bottom
    width: parent.width
    height: 1
    color: brg.settings.dividerColor
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 10
    spacing: 8

    // ── Zoom (the Zoom tool's options) ───────────────────────────────────────────────────────
    Text {
      visible: bar.tool === "zoom"
      text: qsTr("Zoom")
      font.pixelSize: 11
      font.bold: true
      color: brg.settings.textColorMid
    }

    Repeater {
      model: bar.tool === "zoom" ? [1, 2, 3, 4, 6, 8] : []

      MapRailButton {
        required property var modelData
        size: 24
        glyph: modelData + "×"
        tip: qsTr("Draw the map at %1 screen pixels per Game Boy pixel").arg(modelData)
        active: bar.canvas && bar.canvas.zoom === modelData
        onClicked: if (bar.canvas) bar.canvas.userZoom = modelData
      }
    }

    MapRailButton {
      visible: bar.tool === "zoom"
      size: 24
      glyph: "⤢"
      tip: qsTr("Fit the whole map in the window")
      active: bar.canvas && bar.canvas.userZoom === 0
      onClicked: if (bar.canvas) bar.canvas.userZoom = 0
    }

    // ── Select / Pan: nothing to configure YET (the phases fill this in) ─────────────────────
    Text {
      visible: bar.tool === "select"
      text: qsTr("Click a block to inspect it")
      font.pixelSize: 11
      color: brg.settings.textColorMid
    }

    Text {
      visible: bar.tool === "pan"
      text: qsTr("Drag the map — or hold Space with any tool")
      font.pixelSize: 11
      color: brg.settings.textColorMid
    }

    Item { Layout.fillWidth: true }

    // ── The palette (wMapPalOffset) ──────────────────────────────────────────────────────────
    //
    // NOT a brightness slider. The game SUBTRACTS this byte from a pointer into its fade-palette
    // table, so 0/3/6/9 land on real entries (the four levels) and everything else reads across the
    // seam between two of them -- the six glitch palettes. The map is drawn THROUGH whichever byte
    // that produces, so a glitch palette renders as the genuine article. (reference/palettes.md)
    Text {
      text: qsTr("Palette")
      font.pixelSize: 11
      font.bold: true
      color: brg.settings.textColorMid
    }

    ContrastStep {
      text: "−"
      enabled: brg.map.contrast > 0
      onClicked: brg.map.contrast = brg.map.contrast - 1
    }

    Text {
      text: brg.map.contrast
      font.pixelSize: 12
      font.bold: true
      color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorDark
      horizontalAlignment: Text.AlignHCenter
      Layout.minimumWidth: 14
    }

    ContrastStep {
      text: "+"
      enabled: brg.map.contrast < brg.map.contrastMax
      onClicked: brg.map.contrast = brg.map.contrast + 1
    }

    Text {
      text: brg.map.contrastName
      font.pixelSize: 11
      color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorMid
      elide: Text.ElideRight
      Layout.maximumWidth: 230
    }
  }
}
