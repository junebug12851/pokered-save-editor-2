import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"

Item {
  id: top

  property string str: ""

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
        if(lastTileID <= 0 || lastTileID >= brg.fonts.fontCount())
          return noMouse();

        detailView.colorCodeEl.color = determineColor(lastTileID);
        detailView.titleEl.text = (brg.fonts.fontAt(lastTileID).alias !== "")
            ? brg.fonts.fontAt(lastTileID).alias
            : brg.fonts.fontAt(lastTileID).name;
        detailView.codeEl.text = (brg.fonts.fontAt(lastTileID).alias !== "")
            ? brg.fonts.fontAt(lastTileID).name
            : "";
        detailView.descDividerEl.visible = brg.fonts.fontAt(lastTileID).tip !== ""
        detailView.descEl.text = brg.fonts.fontAt(lastTileID).tip;
      }

      // No mouse
      function noMouse() {
        detailView.colorCodeEl.color = "transparent"
        detailView.titleEl.text = ""
        detailView.codeEl.text = ""
        detailView.descDividerEl.visible = false
        detailView.descEl.text = ""
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
        if(lastTileID <= 0 || lastTileID >= brg.fonts.fontCount())
          return;

        top.str = top.str + brg.fonts.fontAt(lastTileID).name
      }
    }
  }
}
