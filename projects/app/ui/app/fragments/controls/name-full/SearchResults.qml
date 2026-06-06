import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

// The character picker, redesigned as a wrapping grid of same-height,
// variable-width "pills" (Twilight's call) instead of a single-column list. Each
// pill is tinted by its category color, and hovering one pops a tooltip with a
// live in-game render of the tile plus its name and description.
Flickable {
  id: topz

  property string str: ""
  property var detailView: null

  clip: true
  contentWidth: width
  contentHeight: flow.height + 16
  ScrollBar.vertical: ScrollBar {}
  boundsBehavior: Flickable.StopAtBounds

  // There can only be one color and many fonts belong to more than one category.
  function determineColor(ind) {
    let font = brg.fonts.fontAt(ind);
    if(!font) return "red";

    if(font.normal)         return brg.settings.fontColorNormal;
    else if(font.control)   return brg.settings.fontColorControl;
    else if(font.picture)   return brg.settings.fontColorPicture;
    else if(font.singleChar)return brg.settings.fontColorSingle;
    else if(font.variable)  return brg.settings.fontColorVar;
    else if(font.multiChar) return brg.settings.fontColorMulti;

    return "red";
  }

  // Push this tile's details into the shared side pane.
  function showDetail(f, ind) {
    if(!topz.detailView || !f) return;
    topz.detailView.colorCodeEl.color = topz.determineColor(ind);
    topz.detailView.titleEl.text = (f.alias !== "") ? f.alias : f.name;
    topz.detailView.codeEl.text = (f.alias !== "") ? f.name : "";
    topz.detailView.descDividerEl.visible = f.tip !== "";
    topz.detailView.descEl.text = f.tip;
  }

  function clearDetail() {
    if(!topz.detailView) return;
    topz.detailView.colorCodeEl.color = "transparent";
    topz.detailView.titleEl.text = "";
    topz.detailView.codeEl.text = "";
    topz.detailView.descDividerEl.visible = false;
    topz.detailView.descEl.text = "";
  }

  Flow {
    id: flow
    x: 4
    y: 6
    width: topz.width - 16
    spacing: 2

    Repeater {
      model: brg.fontSearchModel

      delegate: Rectangle {
        id: pill

        required property int fontInd
        property var f: brg.fonts.fontAt(fontInd)
        property color cat: topz.determineColor(fontInd)

        height: 22
        width: Math.max(28, label.implicitWidth + 14)
        radius: height / 2

        color: mouse.containsMouse ? Qt.lighter(cat, 1.55) : Qt.lighter(cat, 1.88)
        border.width: 1
        border.color: mouse.containsMouse ? cat : Qt.lighter(cat, 1.45)

        Behavior on color { ColorAnimation { duration: 90 } }

        Label {
          id: label
          anchors.centerIn: parent
          text: pill.f ? ((pill.f.alias !== "") ? pill.f.alias : pill.f.name) : ""
          color: Qt.darker(pill.cat, 1.25)
          font.pixelSize: 12
        }

        MouseArea {
          id: mouse
          anchors.fill: parent
          hoverEnabled: true

          onClicked: {
            if(!pill.f) return;
            topz.str = topz.str.toString() + pill.f.name;
          }

          onContainsMouseChanged: {
            if(containsMouse) topz.showDetail(pill.f, pill.fontInd);
            else              topz.clearDetail();
          }
        }

        ToolTip {
          id: tip
          // Image-only tooltip. Control codes have no preview image, so they get
          // no tooltip at all.
          visible: mouse.containsMouse && pill.f && !pill.f.control
          delay: 250

          background: Rectangle {
            color: brg.settings.textColorLight
            radius: 8
            border.width: 1
            border.color: Qt.darker(brg.settings.textColorLight, 1.2)
          }

          // Live in-game tile render on a white "screen" so the dark GB glyph
          // pixels read clearly. tileName is only fed while the tooltip is up so
          // off-screen pills don't churn images.
          contentItem: Rectangle {
            color: "#ffffff"
            radius: 4
            border.width: 1
            border.color: Qt.darker(brg.settings.textColorLight, 1.25)
            implicitWidth: preview.width + 14
            implicitHeight: preview.height + 14

            TilePreview {
              id: preview
              anchors.centerIn: parent
              tileName: (tip.visible && pill.f) ? pill.f.name : ""
            }
          }
        }
      }
    }
  }
}

