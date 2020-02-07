import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../controls/name"
import "../../screens/modal"

/*
 * So... yeah, this sort of become pretty complicated lol. But name editing
 * is something I've really put a lot of thought and work into from the very
 * beginning. Save Editor 1 made a good attempt but I felt it was largely
 * lacking and that was mainly due to the technologies I was using then. This is
 * more along the lines of what I was wanting.
*/

Image {
  id: img

  /*********************************************
   * Public properties with sensible defaults
   *********************************************/

  // Essentially an image scale of sorts
  property real sizeMult: 2

  // Placeholder to contain example demonstration
  // If controlling this from outside, make sure to disable internal generation
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

  // Disable Editor Button
  property bool disableEditor: false

  // Disable Auto Placeholder
  // If this is true, placeholder relies entirely on the placeholder from
  // outside. Otherwise, placeholder ignores placeholder text and generates it's
  // own internal placeholder text.
  property bool disableAutoPlaceholder: false

  // Enable Feedback messages
  property bool enableFeedback: true

  // Center Feedback
  property bool centerFeedback: false

  // Feedback Colors
  property color feedbackColorNormal: "#757575"
  property color feedbackColorWarning: "#ef6c00"

  /*********************************************
   * Internal properties (Should never need to be changed)
   *********************************************/

  // Is outdoor or not, used for tilemap building and processing
  // Wired up to app-wide property
  property bool isOutdoor: brg.settings.previewOutdoor

  // Tileset to reference for <tileXX> codes
  // Wired up to app-wide property
  property string tileset: brg.settings.previewTileset

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
    // If auto placeholder, it means the placeholder is coming in from the
    // outside only
    if(disableAutoPlaceholder)
      return;

    if(!hasBox) {
      placeholder = "%%"
      return;
    }

    if(isPersonName && isPlayerName)
      placeholder = brg.randomExamplePlayer.randomExample();
    else if(isPersonName && !isPlayerName)
      placeholder = brg.randomExampleRival.randomExample();
    else
      placeholder = brg.randomExamplePokemon.randomExample();
  }

  // Re-counts the tiles used
  function reCalc() {
    regCount = brg.fonts.countSizeOf(str);
    expandedCount = brg.fonts.countSizeOfExpanded(str);
  }

  // Because this is somewhat complicated
  function openFullKeyboard() {
    // We manually open outside of the router the full keyboard beacuse
    // we want to pass parameters to it
    appRoot.push("qrc:/ui/app/screens/modal/FullKeyboard.qml", {
                   placeholder: Qt.binding(function() {return img.placeholder;}),
                   str: Qt.binding(function() {return img.str;}),
                   hasBox: Qt.binding(function() {return img.hasBox;}),
                   is2Line: Qt.binding(function() {return img.is2Line;}),
                   isPersonName: Qt.binding(function() {return img.isPersonName;}),
                   isPlayerName: Qt.binding(function() {return img.isPlayerName;}),
                 });

    // We then tell the router of what we've done
    brg.router.manualStackPush("fullKeyboard");

    // And then we open incomming signals so that we can receive input
    fullKeyboardListener.target = appRoot.currentItem;
  }

  // Incomming signals from full-keyboard
  Connections {
    id: fullKeyboardListener

    // Initially set to no incomming signals
    target: null
    ignoreUnknownSignals: true

    // Various signals to receive
    onStrChanged: img.str = appRoot.currentItem.str;
    onToggleExample: img.toggleExample();
    onReUpdateExample: img.reUpdateExample();

    // Notification keyboard is going to close, re-shutoff incomming signals
    onPreClose: fullKeyboardListener.target = null
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
          brg.util.encodeBeforeUrl(placeholder) +"/" +
          brg.util.encodeBeforeUrl(str)

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
  IconButtonRound {
    icon.source: "qrc:/assets/icons/fontawesome/pen.svg"

    visible: !editorVisible && !disableEditor
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
      if(brg.fonts.countSizeOf(text) <= 10)
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
    onToggleFullKeyboard: openFullKeyboard();
  }

  // Error & Informative messages

  // Notification on character count
  // Displays the current character count so long as it's the intended count
  // Considers regular and expanded counts
  // Person names are intedned to be 7 characters and Pokemon 10 characters
  Text {
    // Show this when the editor is visible and the name is an acceptable length
    visible: (editorVisible || disableEditor) &&
             enableFeedback &&
             (chopLen <= ((isPersonName) ? 7 : 10)) &&
             (regCount <= ((isPersonName) ? 7 : 10)) &&
             (expandedCount <= ((isPersonName) ? 7 : 10))

    anchors.top:  parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorNormal

    text: "Using " + regCount + " out of " + chopLen + " bytes."
  }

  // For person names only
  // Anyhting above 7 characters but below or equal to 10 can technically be
  // saved but your breaking the intended length
  Text {
    // Show this when the editor is visible, it's a persons name, and we've
    // gone over 7 characters
    visible: (editorVisible || disableEditor) &&
             enableFeedback &&
             isPersonName &&
             expandedCount > 7 &&
             expandedCount <= 10

    anchors.top: parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorWarning

    text: "Warning: This name is meant to be a max of 7 characters"
  }

  // Whenever the name unfolds much larger on the screen taking up significant
  // amount of tiles
  Text {
    // Show this when the editor is visible, it's a persons name, and we've
    // gone over 7 characters
    visible: (editorVisible || disableEditor) &&
             enableFeedback &&
             expandedCount > 10

    anchors.top: parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorWarning

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
