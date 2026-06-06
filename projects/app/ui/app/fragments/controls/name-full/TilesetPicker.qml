import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

Item {
  id: top

  property string str: ""
  property var detailView: null

  TilesetDisplay {
    id: tilesetImg

    // Center tileset
    anchors.centerIn: parent

    // Scale tileset to fit within boundraries minus page indicator
    sizeMult: (Math.min(parent.width, parent.height) - 40) / 128

    MouseArea {
      anchors.fill: parent
      hoverEnabled: true

      property int lastTileID: 0;

      // Determine color for details preview
      function determineColor(ind) {
        let font = brg.fonts.fontAt(ind);

        if(font.normal)
          return brg.settings.fontColorNormal;
        else if(font.control)
          return brg.settings.fontColorControl;
        else if(font.picture)
          return brg.settings.fontColorPicture;
        else if(font.singleChar)
          return brg.settings.fontColorSingle;
        else if(font.variable)
          return brg.settings.fontColorVar;
        else if(font.multiChar)
          return brg.settings.fontColorMulti;

        // Error
        return "red";
      }

      // Re-process mouse change
      function process() {
        var tileX = Math.trunc(mouseX / (8 * tilesetImg.sizeMult));
        var tileY = Math.trunc(mouseY / (8 * tilesetImg.sizeMult));
        var tileId = tileY * 16 + tileX;

        lastTileID = tileId;

        // We never want a crash!!! Stop here if tileID is invalid
        if(lastTileID <= 0 || lastTileID > brg.fonts.fontCount())
          return noMouse();

        let dv = top.detailView;
        if(!dv) return;
        let f = brg.fonts.fontAt(lastTileID);
        if(!f) return;
        dv.colorCodeEl.color = determineColor(lastTileID);
        dv.titleEl.text = (f.alias !== "") ? f.alias : f.name;
        dv.codeEl.text = (f.alias !== "") ? f.name : "";
        dv.descDividerEl.visible = f.tip !== "";
        dv.descEl.text = f.tip;
      }

      // No mouse
      function noMouse() {
        let dv = top.detailView;
        if(!dv) return;
        dv.colorCodeEl.color = "transparent";
        dv.titleEl.text = "";
        dv.codeEl.text = "";
        dv.descDividerEl.visible = false;
        dv.descEl.text = "";
      }

      onHoveredChanged: {
        if(!hovered) {
          noMouse();
          return;
        }

        process();
      }

      onContainsMouseChanged: {
        if(!containsMouse) {
          noMouse();
          return;
        }
      }

      onMouseXChanged: process();
      onMouseYChanged: process();

      onClicked: {
        // Our highest priority is that we cannot have a crash. A crash is
        // by far the quickest way to destroy a programs reputation in only
        // one single time. If it comes down to not pulling anything up on
        // a click versus crashing, I'll choose the former.
        if(lastTileID <= 0 || lastTileID > brg.fonts.fontCount())
          return;

        var f = brg.fonts.fontAt(lastTileID);
        if(f === null || f === undefined)
          return;

        top.str = top.str + f.name
      }
    }
  }
}
