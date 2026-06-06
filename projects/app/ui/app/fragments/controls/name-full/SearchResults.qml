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
    y: 4
    width: topz.width - 16
    spacing: 2

    Repeater {
      model: brg.fontSea