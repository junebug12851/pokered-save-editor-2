import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../screens/pseudo-modal"
import "../general"
import "../controls/name"
import "../../common/Style.js" as Style

/*
 * TODO: Add in tileset change
*/

Image {
  id: img

  /*********************************************
   * Public properties with sensible defaults
   *********************************************/

  // Essentially an image scale of sorts
  property real sizeMult: 2

  // Placeholder to contain example demonstration
  property string placeholder: "%%"

  // Actual name
  property string str: ""

  // Box enabled by default
  property bool hasBox: false

  // If box is disabled, render 2 lines or 1?
  property bool is2Line: false

  // A persons name? Used for error checking, auto sizing, and examples
  property bool isPersonName: false

  // Players name? Used for examples
  property bool isPlayerName: false

  /*********************************************
   * Internal properties (Should never need to be changed)
   *********************************************/

  // Is outdoor or not, used for tilemap building and processing
  // Wired up to app-wide property
  property bool isOutdoor: root.previewOutdoor

  // Tileset to reference for <tileXX> codes
  // Wired up to app-wide property
  property string tileset: root.previewTileset

  // Actual character count
  property int regCount: 0

  // Expanded character count
  property int expandedCount: 0

  // Whether the quick edit (Stage 2) is visible or not
  property bool editorVisible: false

  // Max tile width of image to chop down to (Ignored if box is open)
  property int chopLen: (isPersonName) ? 7 : 10

  // Current animation frame 0-7
  property int curFrame: 0

  // Animation speed
  property int tick: 1000 / 3

  // BG Color
  property color bgColor: (hasBox) ? "white" : "transparent"

  // Strings to send to the provider
  property string hasBoxStr: (hasBox) ? "box" : "no-box"
  property string is2LineStr: (is2Line) ? "2-lines" : "1-line"
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

  // Re-adjust chop-length as needed
  function adjustChopLen() {
    if(expandedCount > 7)
      chopLen = 10;
    else
      chopLen = Qt.binding(function() {return (isPersonName) ? 7 : 10; });
  }

  // Toggle example
  function toggleExample() {
    hasBox = !hasBox;
  }

  // Clear out or re-update example
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

  // Re-counts the tiles used
  function reCalc() {
    regCount = fonts.countSizeOf(str);
    expandedCount = fonts.countSizeOfExpanded(str);
  }

  // Signals to act accordingly
  onStrChanged: {
    reCalc();
    adjustChopLen();
  }

  onHasBoxChanged: {
    adjustHeight();
    reUpdateExample();
  }
  onChopLenChanged: if(chopLen > 20) chopLen = 20;

  // Actual data to send to provider
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

  // Allows moving to stage-2 (Quick Edit)
  IconBtnRound {
    icon.source: "qrc:/assets/icons/fontawesome/pen.svg"

    visible: !editorVisible
    anchors.top: parent.top
    anchors.topMargin: -16

    anchors.left: parent.right
    anchors.leftMargin: -9

    onClicked: editorVisible = !editorVisible;
  }

  // Stage 2: Quick Edit Field
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

    isPersonName: img.isPersonName
    hasBox: img.hasBox

    onChangeStr: img.str = val;
    onToggleExample: img.toggleExample();
    onReUpdateExample: img.reUpdateExample();
    onToggleFullKeyboard: root.openPseudoModal("./screens/pseudo-modal/NameEditFull.qml")
  }

  // Error & Informative messages

  // Notification on character count
  // Displays the current character count so long as it's the intended count
  // Considers regular and expanded counts
  // Person names are intedned to be 7 characters and Pokemon 10 characters
  Text {
    // Show this when the editor is visible and the name is an acceptable length
    visible: editorVisible &&
             (chopLen <= ((isPersonName) ? 7 : 10)) &&
             (regCount <= ((isPersonName) ? 7 : 10)) &&
             (expandedCount <= ((isPersonName) ? 7 : 10))

    anchors.top: parent.bottom
    anchors.left: parent.left
    font.pixelSize: 11
    color: "#757575"

    text: "Using " + regCount + " out of " + chopLen + " bytes."
  }

  // For person names only
  // Anyhting above 7 characters but below or equal to 10 can technically be
  // saved but your breaking the intended length
  Text {
    // Show this when the editor is visible, it's a persons name, and we've
    // gone over 7 characters
    visible: editorVisible &&
             isPersonName &&
             expandedCount > 7 &&
             expandedCount <= 10

    anchors.top: parent.bottom
    anchors.left: parent.left
    font.pixelSize: 11
    color: "#ef6c00"

    text: "Warning: This name is meant to be a max of 7 characters"
  }

  // Whenever the name unfolds much larger on the screen taking up significant
  // amount of tiles
  Text {
    // Show this when the editor is visible, it's a persons name, and we've
    // gone over 7 characters
    visible: editorVisible &&
             expandedCount > 10

    anchors.top: parent.bottom
    anchors.left: parent.left
    font.pixelSize: 11
    color: "#ef6c00"

    text: "Warning: This name expands out to be much longer on screen."
  }

  // Initial ground-work
  Component.onCompleted: {
    reCalc()
    adjustHeight()
    adjustChopLen()
    reUpdateExample()
  }
}
