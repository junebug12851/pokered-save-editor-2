import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty

import "../../general"
import "../../header"
import "./stats"

// StatsTab.qml -- the "DV/EV" tab of the Pokemon details editor.
//
// Brought up to the details style language (see OverviewTab.qml): one white
// grouped panel of zebra-striped rows, with connected "combo" action groups
// instead of loose buttons and ⋮ overflow menus.
//
//   Row 1  -- a segmented [DV | EV] mode switch (SegSel) on the left, and the
//             active kind's [Max | Re-Roll | Reset] icon combo (SegBtn) on the
//             right. The action combo retargets to DVs or EVs by statKind.
//   Row 2  -- the stat sliders for the active kind (Dv/EvStatGroup), zebra-tinted.
//   Row 3  -- Future Shiny as a segmented [Shiny | Non-Shiny] (SegSel) that is
//             *reactive* to boxData.isShiny: it reflects the live shiny state, so
//             dragging the DV bars into a (non-)shiny combination flips the
//             selection too. Clicking a segment forces that state; the dice button
//             beside it re-rolls a fresh combination within the current state.
//             (This replaces the old checkbox + ⋮ menu entirely.)
//
// SegSel's `active` is a binding to data (statKind / isShiny), not a checkable
// toggle, so the selection always mirrors the underlying value and never drifts.
Rectangle {
  id: top
  color: "transparent"

  property PokemonBox boxData: null
  property PokemonBox partyData: null

  // Which stat kind the sliders + action combo are editing.
  property string statKind: "DV"

  // Zebra tint laid over the white panel for alternate rows (matches OverviewTab).
  property color rowAlt: Qt.rgba(0, 0, 0, 0.04)

  // A selectable text segment of a connected "combo" group. Flat; fills with the
  // accent when `active`, otherwise a hover/press wash; hairline left divider
  // unless it's the first segment or it's active. `active` is bound to data (not a
  // checkable state), so the group always mirrors the underlying value.
  component SegSel: Button {
    id: ssel
    property bool active: false
    property bool first: false
    property bool last: false
    property string tip: ""
    flat: true
    display: AbstractButton.TextOnly
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    leftPadding: 18; rightPadding: 18; topPadding: 0; bottomPadding: 0
    Layout.fillHeight: true
    Layout.minimumHeight: 0
    font.pixelSize: 13
    contentItem: Text {
      text: ssel.text
      font: ssel.font
      color: ssel.active ? brg.settings.textColorLight : brg.settings.textColorMid
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
    }
    // Per-corner radius (Qt 6.7+) so the fill on an end segment follows the
    // group's rounded corners instead of showing a flat square edge.
    background: Rectangle {
      color: ssel.active ? brg.settings.accentColor
             : ssel.down ? Qt.rgba(0, 0, 0, 0.16)
             : ssel.hovered ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
      topLeftRadius: ssel.first ? 4 : 0
      bottomLeftRadius: ssel.first ? 4 : 0
      topRightRadius: ssel.last ? 4 : 0
      bottomRightRadius: ssel.last ? 4 : 0
      Rectangle {
        visible: !ssel.first && !ssel.active
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(0, 0, 0, 0.15)
      }
    }
    MainToolTip { text: ssel.tip }
  }
  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    ColumnLayout {
      // -16 reserves the Material scrollbar lane so the right-aligned action
      // groups stay clear of the overlay scrollbar (see OverviewTab).
      width: scroller.availableWidth - 16
      spacing: 0

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: rowsCol.implicitHeight
        color: brg.settings.textColorLight
        clip: true

        ColumnLayout {
          id: rowsCol
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          spacing: 0

          // ---- DV/EV mode switch + the active kind's Max/Re-Roll/Reset combo ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              spacing: 12

              // Segmented DV | EV.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 34
                implicitWidth: modeGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: modeGrp
                  anchors.fill: parent
                  spacing: 0
                  SegSel {
                    first: true
                    text: qsTr("DV")
                    active: top.statKind === "DV"
                    onClicked: top.statKind = "DV"
                    tip: qsTr("Determinant Values — set on capture and never change in-game; they help make each Pokémon unique. HP is a 5th DV computed from the other four.")
                  }
                  SegSel {
                    last: true
                    text: qsTr("EV")
                    active: top.statKind === "EV"
                    onClicked: top.statKind = "EV"
                    tip: qsTr("Stat Experience — zero on capture, rising after every battle until maxed; they slowly make each Pokémon unique.")
                  }
                }
              }

              Item { Layout.fillWidth: true }

              // Min | Re-Roll | Max for the active kind, ordered low→high:
              // |←  = reset all to 0,  dice = randomize,  →|  = max all. The combo
              // retargets to DVs or EVs by statKind, so EV behaves exactly like DV.
              Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 34
                implicitWidth: actGrp.implicitWidth
                radius: 4; color: "transparent"
                border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                clip: true
                RowLayout {
                  id: actGrp
                  anchors.fill: parent
                  spacing: 0
                  SegBtn {
                    first: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/arrow-left-to-line.svg"
                    enabled: (top.statKind === "DV") ? !boxData.isMinDVs : !boxData.isMinEvs
                    onClicked: (top.statKind === "DV") ? boxData.resetDVs() : boxData.resetEVs()
                    tip: (top.statKind === "DV") ? qsTr("Reset all DVs to 0 (the minimum).") : qsTr("Reset all EVs to 0 (the minimum).")
                  }
                  SegBtn {
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                    onClicked: (top.statKind === "DV") ? boxData.reRollDVs() : boxData.reRollEVs()
                    tip: (top.statKind === "DV") ? qsTr("Re-roll random DVs.") : qsTr("Re-roll random EVs.")
                  }
                  SegBtn {
                    last: true
                    icon.width: 18; icon.height: 18
                    icon.source: "qrc:/assets/icons/fontawesome/arrow-right-to-line.svg"
                    enabled: (top.statKind === "DV") ? !boxData.isMaxDVs : !boxData.isMaxEVs
                    onClicked: (top.statKind === "DV") ? boxData.maxDVs() : boxData.maxEVs()
                    tip: (top.statKind === "DV") ? qsTr("Max all DVs (the maximum).") : qsTr("Max all EVs (the maximum).")
                  }
                }
              }
            }
          }

          // ---- The stat sliders (zebra row; only the active kind is shown). The
          //      row sizes to the visible group's content height. ----
          Rectangle {
            Layout.fillWidth: true
            color: top.rowAlt
            Layout.preferredHeight: ((top.statKind === "DV") ? dvGroup.implicitHeight
                                                             : evGroup.implicitHeight) + 20

            DvStatGroup {
              id: dvGroup
              visible: top.statKind === "DV"
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.leftMargin: 7
              anchors.rightMargin: 7
              anchors.verticalCenter: parent.verticalCenter
            }
            EvStatGroup {
              id: evGroup
              visible: top.statKind === "EV"
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.leftMargin: 7
              anchors.rightMargin: 7
              anchors.verticalCenter: parent.verticalCenter
            }
          }

          // ---- Future Shiny (DV tab only — shininess is derived from DVs, so it
          //      has no meaning on the EV tab). The label sits ABOVE its row. ----
          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            color: "transparent"
            visible: top.statKind === "DV"

            ColumnLayout {
              anchors.fill: parent
              anchors.leftMargin: 12
              anchors.rightMargin: 12
              anchors.topMargin: 8
              anchors.bottomMargin: 8
              spacing: 5

              Text {
                text: qsTr("Future Shiny")
                color: brg.settings.textColorMid
                font.pixelSize: 13
              }

              RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Segmented Shiny | Non-Shiny. `active` binds to boxData.isShiny, so
                // dragging the DV bars into a (non-)shiny combo updates the selection
                // too; clicking a segment forces that state.
                Rectangle {
                  Layout.alignment: Qt.AlignVCenter
                  Layout.preferredHeight: 34
                  implicitWidth: shinyGrp.implicitWidth
                  radius: 4; color: "transparent"
                  border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                  clip: true
                  RowLayout {
                    id: shinyGrp
                    anchors.fill: parent
                    spacing: 0
                    SegSel {
                      first: true
                      text: qsTr("Shiny")
                      active: boxData.isShiny
                      onClicked: if(!boxData.isShiny) boxData.makeShiny();
                      tip: qsTr("Force a DV combination that reads as shiny (the Gen 2 / VC-era formula applied to Gen 1).")
                    }
                    SegSel {
                      last: true
                      text: qsTr("Non-Shiny")
                      active: !boxData.isShiny
                      onClicked: if(boxData.isShiny) boxData.unmakeShiny();
                      tip: qsTr("Force a DV combination that reads as non-shiny.")
                    }
                  }
                }

                Item { Layout.fillWidth: true }

                // Re-roll a fresh combination, keeping the current shiny state.
                Rectangle {
                  Layout.alignment: Qt.AlignVCenter
                  Layout.preferredHeight: 34
                  implicitWidth: shinyRollGrp.implicitWidth
                  radius: 4; color: "transparent"
                  border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
                  clip: true
                  RowLayout {
                    id: shinyRollGrp
                    anchors.fill: parent
                    spacing: 0
                    SegBtn {
                      first: true
                      last: true
                      icon.width: 18; icon.height: 18
                      icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
                      onClicked: boxData.isShiny ? boxData.rollShiny() : boxData.rollNonShiny();
                      tip: qsTr("Re-roll a new DV combination, keeping the current shiny / non-shiny state.")
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
