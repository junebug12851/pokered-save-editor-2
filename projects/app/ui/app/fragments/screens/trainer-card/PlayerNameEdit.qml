import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

NameDisplay {
  id: top

  isPersonName: true
  isPlayerName: true

  // The save model object, or null before a file is open. Always go through this
  // (never dereference the chain blind) — the old code crashed when the chain
  // was momentarily null during load/reset.
  function basics() {
    return brg.file.data.dataExpanded
         ? brg.file.data.dataExpanded.player.basics
         : null;
  }

  // Persist ONLY when an edit session finishes (atomic), not per keystroke.
  // Writing playerName runs fullSetPlayerName, which rescans all storage and
  // updates owned mons' OT — doing that every character hung the editor and
  // could even corrupt a traded mon's OT if an intermediate value collided with
  // it. One atomic commit avoids both.
  onCommitted: (finalStr) => {
    let b = basics();
    if(b && b.playerName !== finalStr)
      b.playerName = finalStr;
  }

  // Reflect external changes (e.g. randomize) back into the field.
  Connections {
    target: brg.file.data.dataExpanded
          ? brg.file.data.dataExpanded.player.basics
          : null
    function onPlayerNameChanged() {
      let b = top.basics();
      if(b) top.str = b.playerName;
    }
  }

  Component.onCompleted: {
    let b = basics();
    if(b) top.str = b.playerName;
  }
}
