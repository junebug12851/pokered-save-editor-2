import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

Image {
  /*********************************************
   * Public properties with sensible defaults
   *********************************************/

  property int sizeMult: 2

  property string tileset: "overworld"
  property string type: "outdoor"
  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: true
  property bool is2Line: false
  property int chopLen: 0
  property color bgColor: (hasBox) ? "white" : "transparent"

  /*********************************************
   * Internal properties (Should never need to be changed)
   *********************************************/

  property int curFrame: 0
  property int tick: 1000 / 3
  property string hasBoxStr: (hasBox) ? "box" : "no-box"
  property string is2LineStr: (is2Line) ? "2-lines" : "1-line"

  // Error checks and corrects width and height to prevent blurring and to have
  // it display correctly. These, along with the width and height, should never
  // need to be changed.

  function adjustHeight() {
    if(hasBox)
      height = Qt.binding(function() {return 8 * 6 * sizeMult; });
    else
      height = Qt.binding(function() {
        return (is2Line) ?
            8 * 3 * sizeMult :
            8 * 1 * sizeMult;
      });
  }

  onHasBoxChanged: adjustHeight();
  onChopLenChanged: if(chopLen > 20) chopLen = 20;

  source: "image://font/" +
          tileset + "/" +
          type + "/" +
          curFrame + "/" +
          width + "/" +
          height + "/" +
          hasBoxStr + "/" +
          is2LineStr + "/" +
          chopLen + "/" +
          bgColor + "/" +
          "none/" +
          util.encodeBeforeUrl(placeholder) +"/" +
          util.encodeBeforeUrl(str)

  // To prevent a million images filling the cache, this should never have to be
  // changed
  cache: false

  // Width and height should never need to be changed
  width: (chopLen > 0 && !hasBox)
         ? 8 * chopLen * sizeMult
         : 8 * 20 * sizeMult

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

  Component.onCompleted: adjustHeight()
}
