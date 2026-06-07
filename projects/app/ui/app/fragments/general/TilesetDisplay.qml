// TilesetDisplay.qml -- a live preview of the whole current tileset image.
//
// Renders the full 128x128 tileset (scaled by sizeMult) via the
// "image://tileset/..." provider, animated across 8 frames, following the
// app-wide preview tileset / outdoor settings. Read-only display used by the name
// tools so you can see the character/tile sheet.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Image {
  id: img

  /*********************************************
   * Public properties with sensible defaults
   *********************************************/

  // Essentially an image scale of sorts
  property real sizeMult: 2

  /*********************************************
   * Internal properties (Should never need to be changed)
   *********************************************/

  // Is outdoor or not, used for tilemap building and processing
  // Wired up to app-wide property
  property bool isOutdoor: brg.settings.previewOutdoor

  // Tileset to reference for <tileXX> codes
  // Wired up to app-wide property
  property string tileset: brg.settings.previewTileset

  // Current animation frame 0-7
  property int curFrame: 0

  // Animation speed
  property int tick: 1000 / 3

  // Strings to send to the provider
  property string isOutdoorStr: (isOutdoor) ? "outdoor" : "indoor"

  width: ((128 * sizeMult) < 1)
         ? 1
         : Math.trunc(128 * sizeMult)

  height: ((128 * sizeMult) < 1)
          ? 1
          : Math.trunc(128 * sizeMult)

  source: "image://tileset/" +
          tileset + "/" +
          isOutdoorStr + "/" +
          "font/" +
          curFrame + "/" +
          "whole/" +
          width + "/" +
          height

  // To prevent a million images filling the cache, this should never have to be
  // changed
  cache: false

  // Always running timer, should never need to be changed
  Timer {
    interval: tick;
    running: true;
    repeat: true
    onTriggered: {
      if((curFrame + 1) >= 8)
        curFrame = 0;
      else
        curFrame++;
    }
  }
}
