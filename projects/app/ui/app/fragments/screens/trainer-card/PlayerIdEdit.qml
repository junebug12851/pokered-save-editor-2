import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"
import "../../controls/menu"

MouseArea {
  id: mouseArea
  hoverEnabled: true
  width: child.implicitWidth
  height: child.implicitHeight

  // The save model object, or null before a file is open.
  function basics() {
    return brg.file.data.dataExpanded
         ? brg.file.data.dataExpanded.player.basics
         : null;
  }

  DefTextEdit {
    id: child
    anchors.fill: parent
    labelEl.text: "ID"
    maximumLength: 4

    // Commit on edit-finish (Enter / focus-out), NOT per keystroke. Writing
    // playerID runs fullSetPlayerId, which rescans all storage and updates owned
    // mons' OT IDs — doing that on every digit hung the editor and could rewrite
    // a traded mon's OT ID if an intermediate value collided with it. One atomic
    // commit avoids both. Invalid/partial input reverts to the stored value.
    onEditingFinished: {
      let b = mouseArea.basics();
      if(!b)
        return;

      var idDec = parseInt(text, 16);

      if(text === "" || isNaN(idDec) || idDec < 0 || idDec > 0xFFFF) {
        child.text = b.playerID.toString(16).toUpperCase();
        return;
      }

      if(b.playerID !== idDec)
        b.playerID = idDec;
    }

    MainToolTip {
      text: "Determines if your Pokemon are traded or not. 4 Characters each 0-9 or A-F."
    }

    Connections {
      target: brg.file.data.dataExpanded
            ? brg.file.data.dataExpanded.player.basics
            : null
      function onPlayerIDChanged() {
        let b = mouseArea.basics();
        if(b) child.text = b.playerID.toString(16).toUpperCase();
      }
    }

    Component.onCompleted: {
      let b = mouseArea.basics();
      if(b) child.text = b.playerID.toString(16).toUpperCase();
    }

    RandomMenu {
      id: menuBtn
      visible: mouseArea.containsMouse
      onRandomize: {
        let b = mouseArea.basics();
        if(b) b.randomizeID();
      }
    }
  }
}
