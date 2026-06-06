import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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

  // Fired when an editing session FINISHES (the quick-edit popup closes, or the
  // full keyboard closes) with the final string. Consumers that drive an
  // expensive/byte-touching model write (player name → OT cascade, rival name)
  // should persist on THIS, not on every keystroke, so the write is atomic.
  // Cheap consumers (Pokémon nickname) can keep writing on str changes.
  signal committed(string val)

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

  // When the popup closes only to hand off to the full keyboard, skip its commit
  // so the value is committed exactly once (when the keyboard closes), keeping
  // the model write atomic.
  property bool suppressNextCommit: false

  // Example ("box") demo state — LOCAL to the quick-edit popup, never the row.
  // The regular name display (trainer card / rival / Pokémon) must only ever
  // show the name; the example lives only inside the editor. (The full keyboard
  // keeps its own separate example state — see FullKeyboard.qml.)
  property bool popupExample: false
  property string popupPlaceholder: "%%"

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

  // Toggle the popup's example demo (does NOT touch the row's hasBox).
  function toggleExample() {
    popupExample = !popupExample;
    reUpdateExample();
  }

  // Generate (or clear) the popup's example placeholder.
  function reUpdateExample() {
    if(disableAutoPlaceholder)
      return;

    if(!popupExample) {
      popupPlaceholder = "%%";
      return;
    }

    if(isPersonName && isPlayerName)
      popupPlaceholder = brg.randomExamplePlayer.randomExample();
    else if(isPersonName && !isPlayerName)
      popupPlaceholder = brg.randomExampleRival.randomExample();
    else
      popupPlaceholder = brg.randomExamplePokemon.randomExample();
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
    // Note: we intentionally do NOT pass placeholder/hasBox — the keyboard owns
    // its own example state so toggling an example there never affects the row.
    appRoot.push("qrc:/ui/app/screens/modal/FullKeyboard.qml", {
                   str: Qt.binding(function() {return img.str;}),
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

    // Receive the edited string. (Example toggles stay inside the keyboard now,
    // so there's nothing example-related to receive here.)
    function onStrChanged() { img.str = appRoot.currentItem.str; }

    // Keyboard is closing: commit the final value (atomic) then shut off signals.
    function onPreClose() {
      img.committed(img.str);
      fullKeyboardListener.target = null;
    }
  }

  // Signals to act accordingly
  onStrChanged: {
    reCalc();
    adjustChopLen();
  }

  onHasBoxChanged: adjustHeight();
  onChopLenChanged: if(chopLen > 20) chopLen = 20;

  // Open/close the centered editor popup when editorVisible flips. Seed the
  // field from the current name on open (a TextField's text: binding breaks once
  // the user types, so it can go stale after an edit elsewhere e.g. the keyboard).
  onEditorVisibleChanged: {
    if(editorVisible) {
      popupEdit.text = img.str;
      editorPopup.open();
    } else {
      editorPopup.close();
    }
  }

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

  // Stage 2: Quick Edit — a centered, dismissible popup rendered in the window
  // overlay so it's NEVER clipped by surrounding tabs/headers (it used to open
  // inline above the name and got cut off at the top of a pane). Shared by the
  // player name, rival, and Pokémon nickname editors.
  Popup {
    id: editorPopup

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay

    width: 450
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 20

    onClosed: {
      editorVisible = false;
      popupExample = false;   // start each edit session name-only
      // Commit the finished edit — unless we're closing only to open the full
      // keyboard, which will commit the final value itself.
      if(suppressNextCommit)
        suppressNextCommit = false;
      else
        img.committed(img.str);
    }

    background: Rectangle {
      color: brg.settings.textColorLight
      radius: 10
      border.width: 1
      border.color: Qt.darker(brg.settings.textColorLight, 1.15)
    }

    contentItem: ColumnLayout {
      spacing: 12

      // ---- Top bar: Simulated controls (left) + Name/Example toggle (right) ----
      RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Label {
          text: "Simulated"
          font.bold: true
          font.pixelSize: 13
          color: brg.settings.textColorDark
          Layout.alignment: Qt.AlignVCenter
        }

        ToolSeparator {
          Layout.fillHeight: false
          Layout.preferredHeight: 24
          Layout.alignment: Qt.AlignVCenter
        }

        FlatToggle {
          text: "Outdoor"
          active: brg.settings.previewOutdoor
          onClicked: brg.settings.previewOutdoor = !brg.settings.previewOutdoor;
          MainToolTip { text: "Render tiles as they'd look outdoors vs. indoors." }
        }

        SimulatedTilesetCombo {
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredWidth: 120
        }

        Item { Layout.fillWidth: true }   // push the Name/Example toggle right

        FlatToggle {
          text: popupExample ? "Example" : "Name"
          active: popupExample
          onClicked: img.toggleExample();
          MainToolTip { text: "Preview just the name, or inside an example sentence." }
        }

        IconButtonSquare {
          visible: popupExample
          Layout.alignment: Qt.AlignVCenter
          icon.width: 16
          icon.source: "qrc:/assets/icons/fontawesome/angle-double-right.svg"
          onClicked: img.reUpdateExample();
          MainToolTip { text: "Show a different random example" }
        }
      }

      // Live in-game preview. Normally it mirrors the row's source (just the
      // name); when the popup's example demo is on it renders its OWN box source
      // (str inside a random example sentence) — independent of the row.
      Image {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: popupExample ? 320 : Math.min(img.width, 300)
        Layout.preferredHeight: popupExample ? 96 : img.height
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        fillMode: Image.PreserveAspectFit
        cache: false
        source: popupExample
                ? ("image://font/" + tileset + "/" + isOutdoorStr + "/" + curFrame +
                   "/320/96/box/" + is2LineStr + "/" + chopLen + "/white/none/" +
                   brg.util.encodeBeforeUrl(popupPlaceholder) + "/" +
                   brg.util.encodeBeforeUrl(str))
                : img.source
      }

      NameEdit {
        id: popupEdit
        Layout.fillWidth: true

        // text is seeded on popup open (editorPopup.onOpened) and pushes UP from
        // here; we don't bind text:str (that binding breaks on first keystroke).
        onTextChanged: {
          if(brg.fonts.countSizeOf(text) <= 10)
            str = text;
        }

        isPersonName: img.isPersonName

        onAccepted: editorVisible = false
        onClose: editorVisible = false

        // Close this (modal) popup first, otherwise it blocks the full keyboard
        // that openFullKeyboard() pushes onto the app stack. Suppress the popup's
        // own commit — the keyboard commits the final value on close, so the
        // model is written exactly once.
        onToggleFullKeyboard: { suppressNextCommit = true; editorVisible = false; openFullKeyboard(); }
      }

      // ---- Byte-count feedback / warnings (June's original messages) ----
      Label {
        Layout.fillWidth: true
        visible: enableFeedback &&
                 (chopLen <= ((isPersonName) ? 7 : 10)) &&
                 (regCount <= ((isPersonName) ? 7 : 10)) &&
                 (expandedCount <= ((isPersonName) ? 7 : 10))
        font.pixelSize: 11
        color: feedbackColorNormal
        wrapMode: Text.WordWrap
        text: "Using " + regCount + " out of " + chopLen + " bytes."
      }

      Label {
        Layout.fillWidth: true
        visible: enableFeedback &&
                 isPersonName &&
                 expandedCount > 7 &&
                 expandedCount <= 10 &&
                 regCount < 10
        font.pixelSize: 11
        color: feedbackColorWarning
        wrapMode: Text.WordWrap
        text: "Warning: This name is meant to be a max of 7 characters"
      }

      Label {
        Layout.fillWidth: true
        visible: enableFeedback &&
                 expandedCount > 10 &&
                 regCount < 10
        font.pixelSize: 11
        color: feedbackColorWarning
        wrapMode: Text.WordWrap
        text: "Warning: This name expands out to be much longer on screen."
      }

      Label {
        Layout.fillWidth: true
        visible: enableFeedback &&
                 regCount >= 10
        font.pixelSize: 11
        color: feedbackColorWarning
        wrapMode: Text.WordWrap
        text: "Warning: You've used all 10 out of 10 bytes available."
      }
    }
  }

  // Inline feedback for read-only / embedded use (disableEditor — e.g. the full
  // keyboard preview). The interactive editor shows its feedback in the popup.
  Text {
    visible: disableEditor &&
             enableFeedback &&
             (chopLen <= ((isPersonName) ? 7 : 10)) &&
             (regCount <= ((isPersonName) ? 7 : 10)) &&
             (expandedCount <= ((isPersonName) ? 7 : 10))

    anchors.top:  parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorNormal
    horizontalAlignment: (centerFeedback) ? Text.AlignHCenter : undefined

    text: "Using " + regCount + " out of " + chopLen + " bytes."
  }

  Text {
    visible: disableEditor &&
             enableFeedback &&
             isPersonName &&
             expandedCount > 7 &&
             expandedCount <= 10 &&
             regCount < 10

    anchors.top: parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorWarning
    horizontalAlignment: (centerFeedback) ? Text.AlignHCenter : undefined

    text: "Warning: This name is meant to be a max of 7 characters"
  }

  Text {
    visible: disableEditor &&
             enableFeedback &&
             expandedCount > 10 &&
             regCount < 10

    anchors.top: parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorWarning
    horizontalAlignment: (centerFeedback) ? Text.AlignHCenter : undefined

    text: "Warning: This name expands out to be much longer on screen."
  }

  Text {
    visible: disableEditor &&
             enableFeedback &&
             regCount >= 10

    anchors.top: parent.bottom
    anchors.left: (centerFeedback) ? undefined : parent.left
    anchors.horizontalCenter: (centerFeedback) ? parent.horizontalCenter : undefined
    font.pixelSize: 11
    color: feedbackColorWarning
    horizontalAlignment: (centerFeedback) ? Text.AlignHCenter : undefined

    text: "Warning: You've used all 10 out of 10 bytes available."
  }

  // Initial ground-work
  Component.onCompleted: {
    reCalc()
    adjustHeight()
  }
}
