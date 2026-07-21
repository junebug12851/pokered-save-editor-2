/*
  * Copyright 2026 Fairy Fox
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/*
  MapRailGroup.qml -- a GROUP of rail buttons, collapsed to ONE.

  Project leadership, 2026-07-14: *"collapse the left toolbar button groups into a single button each — I count
  three-or-more groups, so reduce it down to three buttons."*

  So the left rail is three groups: the tools (select / pan / zoom), the makers (place a door / place a
  person), and the panels (layers / characters / details). Each is now ONE button that shows the
  group's ACTIVE member (or the last one you picked from it), wears a small corner ◢ to say "there is
  more in here", and flies the members out to the right when you click it. Pick one and it collapses.

  The face is reactive -- exactly the preference from the top-bar pass: it shows what is ACTIVE, not a
  generic group glyph. So the tools button shows ↖ or ✥ or ⌕ depending on which tool is in hand, and
  the panels button shows the icon of whatever panel is open.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: grp

  /// The members, in flyout order: `[{ id, glyph, tip, shortcut? }]`.
  property var members: []

  /// The id of the member that is currently ACTIVE — the tool in hand, the panel that is open — or ""
  /// if nothing in this group is active. The face shows it, lit; otherwise the face shows @ref
  /// lastPicked. The caller binds this (e.g. `activeId: mapScreen.tool` for the tools group).
  property string activeId: ""

  /// The tooltip on the collapsed face button.
  property string tip: ""

  /// Extra content rendered in the flyout AFTER the member buttons — the tools group hands it the
  /// zoom ▾ (ZoomMenu), so the zoom slider + "Go to…" still have exactly one home, inside the group.
  default property alias flyoutExtra: extra.data

  /// Emitted when a member is chosen from the flyout. The caller does the thing (set the tool, open
  /// the panel).
  signal chosen(string id)

  /// Whether the flyout is open. Exposed (with an objectName) so the screenshot harness can open it.
  property bool expanded: false

  /// What the face shows when nothing is active: the last member picked, defaulting to the first.
  property string lastPicked: members.length > 0 ? members[0].id : ""

  readonly property string shownId: grp.activeId !== "" ? grp.activeId : grp.lastPicked

  implicitWidth: 32
  implicitHeight: 32

  function glyphOf(id) {
    for (let i = 0; i < grp.members.length; i++)
      if (grp.members[i].id === id)
        return grp.members[i].glyph;
    return grp.members.length > 0 ? grp.members[0].glyph : "";
  }

  // ── The collapsed face ─────────────────────────────────────────────────────────────────────
  MapRailButton {
    id: face
    anchors.fill: parent

    glyph: grp.glyphOf(grp.shownId)
    active: grp.activeId !== "" || grp.expanded
    tip: grp.tip

    onClicked: grp.expanded = !grp.expanded
  }

  // The "there's a flyout here" mark — a small filled corner, the convention every tool group uses.
  Text {
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 2
    anchors.bottomMargin: 1

    text: "◢"
    font.pixelSize: 7
    color: face.active ? brg.settings.textColorLight : brg.settings.textColorMid
    opacity: 0.8
  }

  // ── The flyout ─────────────────────────────────────────────────────────────────────────────
  //
  // Pops to the RIGHT, over the canvas, so the rail stays narrow. Click a member to pick it; click
  // away (or Esc) to dismiss without choosing.
  Popup {
    id: fly

    visible: grp.expanded
    onClosed: grp.expanded = false

    x: face.width + 5
    y: -3
    padding: 4

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
      color: "#ffffff"
      radius: 8
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    contentItem: RowLayout {
      spacing: 3

      Repeater {
        model: grp.members

        MapRailButton {
          required property var modelData

          objectName: "toolBtn_" + modelData.id   // the DEBUG harness still finds tools by name
          size: 30
          glyph: modelData.glyph
          tip: modelData.tip
          shortcut: modelData.shortcut !== undefined ? modelData.shortcut : ""
          active: grp.activeId === modelData.id

          onClicked: {
            grp.lastPicked = modelData.id;
            grp.chosen(modelData.id);
            grp.expanded = false;
          }
        }
      }

      // The zoom ▾, when the caller supplies one. Sized to its child; invisible (and space-free) when
      // empty, so the makers/panels flyouts don't grow a phantom gap.
      Item {
        id: extra
        Layout.preferredWidth: childrenRect.width
        Layout.preferredHeight: 30
        Layout.alignment: Qt.AlignVCenter
        visible: childrenRect.width > 0
      }
    }
  }
}
