import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../general"
import "../controls/name"
import "../../common/Style.js" as Style

/*
 * TODO: Add in tileset change
*/

Image {
  /*********************************************
   * Public properties with sensible defaults
   *********************************************/
  property real sizeMult: 2
  property bool isOutdoor: true
  property string tileset: "overworld"
  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  /*********************************************
   * Internal properties (Should never need to be changed)
   *********************************************/

  property bool editorVisible: false
  property int chopLen: (isPersonName) ? 7 : 10
  property int curFrame: 0
  property int tick: 1000 / 3
  property string hasBoxStr: (hasBox) ? "box" : "no-box"
  property string is2LineStr: (is2Line) ? "2-lines" : "1-line"
  property color bgColor: (hasBox) ? "white" : "transparent"
  property string isOutdoorStr: (isOutdoor) ? "outdoor" : "indoor"

  // Error checks and corrects width and height to prevent blurring and to have
  // it display correctly. These, along with the width and height, should never
  // need to be changed.

  function adjustHeight() {
    if(hasBox)
      height = Qt.binding(function() {return Math.trunc(8 * 6 * sizeMult); });
    else
      height = Qt.binding(function() {
        return (is2Line) ?
            Math.trunc(8 * 3 * sizeMult) :
            Math.trunc(8 * 1 * sizeMult);
      });
  }

  function adjustChopLen() {
    if(fonts.countSizeOf(str) > 7)
      chopLen = 10;
    else
      chopLen = Qt.binding(function() {return (isPersonName) ? 7 : 10; });
  }

  function toggleExample() {
    hasBox = !hasBox;
  }

  function reUpdateExample() {
    if(!hasBox) {
      placeholder = "%%"
      return;
    }

    if(isPersonName && isPlayerName)
      placeholder = randomExamplePlayer.randomExample();
    else if(isPersonName && !isPlayerName)
      placeholder = randomExampleRival.randomExample();
    else
      placeholder = randomExamplePokemon.randomExample();
  }

  onStrChanged: adjustChopLen();
  onHasBoxChanged: {
    adjustHeight();
    reUpdateExample();
  }
  onChopLenChanged: if(chopLen > 20) chopLen = 20;

  source: "image://font/" +
          tileset + "/" +
          isOutdoorStr + "/" +
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
         ? Math.trunc(8 * chopLen * sizeMult)
         : Math.trunc(8 * 20 * sizeMult)

  // This is a placeholder to prevent it occasionally asking for height 0
  // on startup. Functions above will quickly correct it to the proper value
  // during startup
  height: 1

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

  MenuButton {
    id: menuBtn
    visible: !editorVisible
    anchors.top: parent.top
    anchors.topMargin: -14

    anchors.left: parent.right
    anchors.leftMargin: -7
    onClicked: menu.open();

    Menu {
      id: menu

      MenuItem {
        text: "Randomize Name";
        onTriggered: {
          if(isPersonName)
            str = randomPlayerName.randomName();
          else
            str = randomPokemonName.randomName();
        }
      }

      MenuItem {
        text: "Toggle Example"
        onTriggered: toggleExample();
      }

      MenuItem {
        enabled: hasBox
        text: "Randomize Example"
        onTriggered: reUpdateExample();
      }

      MenuItem {
        text: "Close"
        onTriggered: menu.close();
      }
    }
  }

  EditButton {
    visible: !editorVisible
    anchors.top: menuBtn.top
    anchors.topMargin: -1

    anchors.left: menuBtn.right
    anchors.leftMargin: -11

    onClicked: editorVisible = !editorVisible;
  }

  NameEdit {
    text: str
    onTextChanged: {
      if(fonts.countSizeOf(text) <= 10)
        str = text;
    }

    visible: editorVisible
    onAccepted: editorVisible = !editorVisible
    onClose: editorVisible = !editorVisible

    width: parent.width
    anchors.bottom: parent.top
    anchors.left: parent.left
    anchors.bottomMargin: 5
  }

  Component.onCompleted: {
    adjustHeight()
    adjustChopLen()
  }
}
